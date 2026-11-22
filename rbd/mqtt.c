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
#include <errno.h>
#include <mosquitto.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "logging.h"
#include "mqtt.h"

/* *********************************************************************************** *
 * Module global variables
 * *********************************************************************************** */
static pthread_mutex_t   mosqLock = PTHREAD_MUTEX_INITIALIZER; /**< Guards mosq and   */
                                                               /**< subscriptionList  */
static struct mosquitto *mosq = NULL;             /**< Handle to MQTT broker           */
static mqttIncoming_t   *subscriptionList = NULL; /**< List of topics to subscribe to  */
static bool              loopRunning = false;     /**< Network thread is running       */
static bool              subscriptionError = false; /**< Subscribe failed after connect*/

/**
 * @brief Dispatch incomming MQTT message
 *
 * Select and call the callback function that matches the topic.
 *
 * @param mos       Mosquitto client handle (unused)
 * @param userData  User data of the client handle (unused)
 * @param message   Incoming message; payload is not NUL-terminated
 */
static void dispatchMessage(struct mosquitto *mos, void *userData, const struct mosquitto_message *message) {
    int idx = 0;

    (void)mos;
    (void)userData;

    if (subscriptionList == NULL || message == NULL || message->topic == NULL) {
        writeLog(LOG_WARNING, "Dropped MQTT message without subscription list or topic");
    } else {
        /* identify callback function by matching topic */
        while (subscriptionList[idx].topic != NULL) {
            if (strcmp(message->topic, subscriptionList[idx].topic) == 0) {
                if (subscriptionList[idx].handler != NULL) {
                    (subscriptionList[idx].handler)(message->payload,
                                                    message->payloadlen,
                                                    message->topic,
                                                    subscriptionList[idx].user_data);
                } else {
                    writeLog(LOG_WARNING, "No handler for MQTT topic '%s'", message->topic);
                }
                break;
            }
            idx++;
        }
    }
}

/**
 * @brief Subscribe to all topics in the subscription list
 *
 * Sets subscriptionError if at least one subscription is rejected.
 *
 * @param m  Mosquitto client handle
 */
static void subscribeAll(struct mosquitto *m) {
    int idx = 0;

    while (subscriptionList != NULL && subscriptionList[idx].topic != NULL) {
        int qos = subscriptionList[idx].qos;

        if (qos < 0 || qos > 2) {
            writeLog(LOG_ERR, "Invalid QoS %d for MQTT topic '%s'", qos,
                     subscriptionList[idx].topic);
            subscriptionError = true;
        } else {
            int rc = mosquitto_subscribe(m, NULL, subscriptionList[idx].topic, qos);

            if (rc != MOSQ_ERR_SUCCESS) {
                writeLog(LOG_ERR, "Cannot subscribe to MQTT topic '%s': %s",
                         subscriptionList[idx].topic, mosquitto_strerror(rc));
                subscriptionError = true;
            } else {
                writeLog(LOG_DEBUG, "Subscribed to MQTT topic '%s' (QoS %d)",
                         subscriptionList[idx].topic, qos);
            }
        }
        idx++;
    }
}

/**
 * @brief Connect callback, re-subscribes after every (re)connect
 *
 * @param m         Mosquitto client handle
 * @param userData  User data of the client handle (unused)
 * @param rc        CONNACK result code
 */
static void handleConnect(struct mosquitto *m, void *userData, int rc) {
    (void)userData;

    if (rc != 0) {
        writeLog(LOG_ERR, "MQTT connection refused: %s", mosquitto_connack_string(rc));
        subscriptionError = true;
    } else {
        writeLog(LOG_INFO, "MQTT connection established");
        subscriptionError = false;
        subscribeAll(m);
    }
}

/**
 * @brief Disconnect callback, reports lost connections
 *
 * @param m         Mosquitto client handle (unused)
 * @param userData  User data of the client handle (unused)
 * @param rc        Disconnect reason
 */
static void handleDisconnect(struct mosquitto *m, void *userData, int rc) {
    (void)m;
    (void)userData;

    if (rc != 0) {
        writeLog(LOG_WARNING, "MQTT connection lost: %s (reconnecting)",
                 mosquitto_strerror(rc));
    } else {
        writeLog(LOG_INFO, "MQTT connection closed");
    }
}

/* *********************************************************************************** *
 * P R I V A T E  H E L P E R S
 * *********************************************************************************** */

/**
 * @brief Destroy the client and stop the network thread
 *
 * Caller must hold mosqLock.
 */
static void destroyClientLocked(void) {
    if (mosq != NULL) {
        if (loopRunning) {
            mosquitto_loop_stop(mosq, true);
            loopRunning = false;
        }
        mosquitto_destroy(mosq);
        mosq = NULL;
    }
    subscriptionList = NULL;
}

/**
 * @brief Publish a raw payload, mosqLock must be held by the caller
 *
 * @param topic    Topic of MQTT message
 * @param payload  Message payload
 * @param len      Payload length
 * @param qos      0, 1 or 2 indicating the Quality of Service
 * @param retain   Set to true to make the message retained
 *
 * @return true on success, false on error
 */
static bool publishLocked(const char *topic, const void *payload, int len, int qos, bool retain) {
    bool success = false;

    if (mosq == NULL) {
        writeLog(LOG_DEBUG, "Publish to '%s' dropped: not connected to broker", topic);
    } else {
        int rc = mosquitto_publish(mosq, NULL, topic, len, payload, qos, retain);

        if (rc != MOSQ_ERR_SUCCESS) {
            writeLog(LOG_ERR, "Publish to '%s' failed: %s", topic, mosquitto_strerror(rc));
        } else {
            success = true;
        }
    }
    return success;
}

/* *********************************************************************************** *
 * P U B L I C  I N T E R F A C E
 * *********************************************************************************** */

bool mqttPublish(const char *topic, int qos, const char *format, ...) {
    char    message[MAX_MQTT_MESSAGE];
    va_list valist;
    int     len;
    bool    success = false;

    if (topic == NULL || format == NULL || qos < 0 || qos > 2) {
        writeLog(LOG_ERR, "mqttPublish(): invalid arguments");
    } else {
        message[0] = '\0';

        va_start(valist, format);
        len = vsnprintf(message, sizeof message, format, valist);
        va_end(valist);

        if (len < 0) {
            writeLog(LOG_ERR, "mqttPublish(): cannot format message for '%s'", topic);
        } else if ((size_t)len >= sizeof message) {
            writeLog(LOG_WARNING, "mqttPublish(): message for '%s' truncated to %zu bytes",
                     topic, sizeof message - 1);
        } else {
            pthread_mutex_lock(&mosqLock);
            success = publishLocked(topic, message, len, qos, false);
            pthread_mutex_unlock(&mosqLock);
        }
    }
    return success;
}

bool mqttPublishRaw(const char *topic, const char *payload, size_t len, int qos, bool retain) {
    bool success = false;

    if (topic == NULL || payload == NULL || len < 0 || qos < 0 || qos > 2) {
        writeLog(LOG_ERR, "mqttPublishRaw(): invalid arguments");
    } else {
        pthread_mutex_lock(&mosqLock);
        success = publishLocked(topic, payload, len, qos, retain);
        pthread_mutex_unlock(&mosqLock);
    }
    return success;
}

bool mqttInit(const char *broker, int port, int keepalive, mqttIncoming_t *subscriptions) {
    bool success = false;
    int  rc;

    if (broker == NULL || subscriptions == NULL || port <= 0 || port > 65535 || keepalive <= 0) {
        writeLog(LOG_ERR, "mqttInit(): invalid arguments");
    } else {
        pthread_mutex_lock(&mosqLock);

        if (mosq != NULL) {
            writeLog(LOG_WARNING, "mqttInit(): MQTT already initialized");
        } else if ((rc = mosquitto_lib_init()) != MOSQ_ERR_SUCCESS) {
            writeLog(LOG_ERR, "mqttInit(): cannot initialize mosquitto: %s",
                     mosquitto_strerror(rc));
        } else {
            mosq = mosquitto_new(NULL, true, NULL);

            if (mosq == NULL) {
                writeLog(LOG_ERR, "mqttInit(): cannot create MQTT client: %s", strerror(errno));
                mosquitto_lib_cleanup();
            } else {
                /* Callbacks and state must be in place before the network thread runs */
                subscriptionList = subscriptions;
                mosquitto_message_callback_set(mosq, &dispatchMessage);
                mosquitto_connect_callback_set(mosq, &handleConnect);
                mosquitto_disconnect_callback_set(mosq, &handleDisconnect);

                rc = mosquitto_connect(mosq, broker, port, keepalive);

                if (rc != MOSQ_ERR_SUCCESS) {
                    writeLog(LOG_ERR, "mqttInit(): connect to %s:%d failed: %s",
                             broker, port, mosquitto_strerror(rc));
                } else if ((rc = mosquitto_loop_start(mosq)) != MOSQ_ERR_SUCCESS) {
                    writeLog(LOG_ERR, "mqttInit(): cannot start network loop: %s",
                             mosquitto_strerror(rc));
                } else {
                    loopRunning = true;
                    success = !subscriptionError;
                    if (!success) {
                        writeLog(LOG_ERR, "mqttInit(): at least one subscription failed");
                    }
                }

                if (!success) {
                    destroyClientLocked();
                    mosquitto_lib_cleanup();
                }
            }
        }
        pthread_mutex_unlock(&mosqLock);
    }
    return success;
}

void mqttEnd(void) {
    pthread_mutex_lock(&mosqLock);
    destroyClientLocked();
    mosquitto_lib_cleanup();
    pthread_mutex_unlock(&mosqLock);
}
