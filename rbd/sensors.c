/* *********************************************************************************** */
/*                                                                                     */
/*  Copyright (c) 2026 by Bodo Bauer <bb@bb-zone.com>                                  */
/*                                                                                     */
/*  This program is free software: you can redistribute it and/or modify               */
/*  it under the terms of the GNU General Public License as published by               */
/*  the Free Software Foundation, either version 3 of the License, or                  */
/*  (at your option) any later version.                                                */
/*                                                                                     */
/*  This program is distributed in the hope that it will be useful,                    */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                     */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                      */
/*  GNU General Public License for more details.                                       */
/*                                                                                     */
/*  You should have received a copy of the GNU General Public License                  */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.              */
/* *********************************************************************************** */
#define _GNU_SOURCE                                     /* enable pthread np functions */

#include <errno.h>
#include <inttypes.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>

#include "mqtt.h"
#include "sensors.h"
#include "serial.h"

/** Packet IDs requested from the Roomba stream. */
// static const uint8_t pktIds[] = {7, 8, 9, 10, 11, 12, 22, 23, 24, 25, 26, 35};

/** Decoded sensor snapshot. */
typedef struct {
	uint8_t  bumpsDrops;    /**< Packet 7: bumps and wheel drops */
	uint8_t  wall;          /**< Packet 8 */
	uint8_t  cliffL, cliffFL, cliffFR, cliffR;     /**< Packets 9-12 */
	uint16_t voltage_mV;    /**< Packet 22 */
	int16_t  current_mA;    /**< Packet 23 */
	int8_t   temp_C;        /**< Packet 24 */
	uint16_t charge_mAh;    /**< Packet 25 */
	uint16_t capacity_mAh;  /**< Packet 26 */
	uint8_t  oiMode;        /**< Packet 35 */
} SensorData;

/** Stream frame parser state. */
typedef struct {
	enum { S_SYNC, S_LEN, S_DATA } state;
	uint8_t buf[256];       /**< Payload followed by checksum */
	size_t  n;              /**< Expected payload length */
	size_t  pos;            /**< Bytes received in buf */
} Parser;

/** Data length in bytes per OI packet ID (0 = unsupported here). */
static const uint8_t pktLen[36] = {
	[7]  = 1, [8]  = 1, [9]  = 1, [10] = 1, [11] = 1, [12] = 1,
	[22] = 2, [23] = 2, [24] = 1, [25] = 2, [26] = 2, [35] = 1
};

static _Atomic uint8_t lastMode = 0xff;         /**< 0xff = no data yet */

/**
 * @brief Monotonic time in milliseconds.
 * @return Milliseconds since an arbitrary start point.
 */
static uint64_t nowMs(void) {
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

/**
 * @brief Decode the payload of one verified stream frame.
 *
 * @param p   Payload (sequence of id, data...).
 * @param n   Payload length in bytes.
 * @param out Snapshot to update.
 * @return 0 on success, -1 on malformed payload.
 */
static int decodePayload(const uint8_t *p, size_t n, SensorData *out) {
	size_t i = 0;

	while (i < n) {
		uint8_t id = p[i++];
		uint8_t len = (id < sizeof(pktLen)) ? pktLen[id] : 0;

		if (len == 0 || i + len > n)
			return -1;
		switch (id) {
            case 7:  out->bumpsDrops = p[i]; break;
            case 8:  out->wall = p[i]; break;
            case 9:  out->cliffL = p[i]; break;
            case 10: out->cliffFL = p[i]; break;
            case 11: out->cliffFR = p[i]; break;
            case 12: out->cliffR = p[i]; break;
            case 22: out->voltage_mV = (uint16_t)(p[i] << 8 | p[i + 1]); break;
            case 23: out->current_mA = (int16_t)(p[i] << 8 | p[i + 1]); break;
            case 24: out->temp_C = (int8_t)p[i]; break;
            case 25: out->charge_mAh = (uint16_t)(p[i] << 8 | p[i + 1]); break;
            case 26: out->capacity_mAh = (uint16_t)(p[i] << 8 | p[i + 1]); break;
            case 35: out->oiMode = p[i]; break;
		} 
		i += len;
	}
	return 0;
}

/**
 * @brief Feed one byte into the frame parser.
 *
 * @param ps  Parser state.
 * @param c   Received byte.
 * @param out Snapshot updated when a valid frame completes.
 * @return 1 if a valid frame was decoded, 0 otherwise.
 */
static int parserFeed(Parser *ps, uint8_t c, SensorData *out) {
	switch (ps->state) {
	case S_SYNC:
		if (c == OI_STREAM_HEADER)
			ps->state = S_LEN;
		return 0;
	case S_LEN:
		ps->n = c;
		ps->pos = 0;
		ps->state = (c > 0) ? S_DATA : S_SYNC;
		return 0;
	case S_DATA:
		ps->buf[ps->pos++] = c;
		if (ps->pos < ps->n + 1)
			return 0;
		ps->state = S_SYNC;
		{
			unsigned sum = OI_STREAM_HEADER + ps->n;
			size_t i;

			for (i = 0; i <= ps->n; i++)
				sum += ps->buf[i];
			if ((sum & 0xff) != 0)
				return 0;       /* checksum error, resync */
		}
		return decodePayload(ps->buf, ps->n, out) == 0;
	}
	return 0;
}

/**
 * @brief Format a snapshot as one JSON object.
 *
 * @param d    Snapshot.
 * @param seq  Message sequence number.
 * @param buf  Output buffer.
 * @param size Size of the output buffer.
 * @return Number of characters written (see snprintf).
 */
static int toJson(const SensorData *d, uint64_t seq, char *buf, size_t size)
{
	return snprintf(buf, size,
	    "{\"seq\":%" PRIu64 ",\"t\":%" PRIu64 ",\"mode\":%u,"
	    "\"bump\":%u,\"wall\":%u,"
	    "\"cliff\":[%u,%u,%u,%u],"
	    "\"batt\":{\"mV\":%u,\"mA\":%d,\"C\":%d,\"mAh\":%u,\"cap\":%u}}",
	    seq, nowMs(), d->oiMode, d->bumpsDrops, d->wall,
	    d->cliffL, d->cliffFL, d->cliffFR, d->cliffR,
	    d->voltage_mV, d->current_mA, d->temp_C,
	    d->charge_mAh, d->capacity_mAh);
}

/**
 * @brief Sensor thread: reads the OI stream and publishes JSON via MQTT.
 *
 * @param arg Unused.
 * @return Never returns.
 */
void *threadSensors(void*) {
    SensorData cur, lastPub;
	Parser ps;
	struct pollfd pfd;
	uint64_t seq = 0, lastMs = 0, lastHb = 0;
	uint8_t rx[128];
	char json[JSON_MAX];

    #ifdef __APPLE__                             /* avoid warning on host build system */
        pthread_setname_np("rbd_sensors");
    #else
        pthread_setname_np(pthread_self(), "rbd_sensors");
    #endif
	memset(&cur, 0, sizeof(cur));
	memset(&lastPub, 0, sizeof(lastPub));
	memset(&ps, 0, sizeof(ps));
	pfd.fd = serialFd();                /* assumed: configured UART fd */
	pfd.events = POLLIN;

	for (;;) {
		int dirty = 0;
		ssize_t r;
		ssize_t i;
		uint64_t t;

		if (poll(&pfd, 1, 200) > 0 && (pfd.revents & POLLIN)) {
			r = read(pfd.fd, rx, sizeof(rx));
			for (i = 0; i < r; i++)
				dirty |= parserFeed(&ps, rx[i], &cur);
		}
		t = nowMs();
		atomic_store(&lastMode, cur.oiMode);
		
		/* Publish on safety-relevant change, otherwise at 10 Hz. */
		if (dirty && ((
                        (cur.bumpsDrops != lastPub.bumpsDrops) ||
		                (cur.cliffL     != lastPub.cliffL)     ||
		                (cur.cliffFL    != lastPub.cliffFL)    ||
		                (cur.cliffFR    != lastPub.cliffFR)    ||
		                (cur.cliffR     != lastPub.cliffR)     ||
		                (cur.oiMode     != lastPub.oiMode)
                    ) || (dirty && t - lastMs >= PUBLISH_PERIOD_MS))) {
			    toJson(&cur, seq++, json, sizeof(json));
			    mqttPublish("roombot/sensor/state", "%s", json);
			    lastPub = cur;
			    lastMs = t;
		}
		if (t - lastHb >= 1000) {       /* keep your 1 s heartbeat */
			mqttPublish("roombot/sensor/heartbeat","0x%010" PRIx64, (uint64_t)seq);
			lastHb = t;
		}
	}
	return NULL;
}

int sensorsGetMode(void) {
	uint8_t m = atomic_load(&lastMode);

	return (m == 0xff) ? -1 : (int)m;
}