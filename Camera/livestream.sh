#!/bin/bash
# *********************************************************************************** #
#                                                                                     #
#  Copyright (c) 2026 by Bodo Bauer <bb@bb-zone.com>                                  #
#                                                                                     #
#  This program is free software: you can redistribute it and/or modify               #
#  it under the terms of the GNU General Public License as published by               #
#  the Free Software Foundation, either version 3 of the License, or                  #
#  (at your option) any later version.                                                #
#                                                                                     #
#  This program is distributed in the hope that it will be useful,                    #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of                     #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                      #
#  GNU General Public License for more details.                                       #
#                                                                                     #
#  You should have received a copy of the GNU General Public License                  #
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.              #
# *********************************************************************************** #
TAG="CAMERA"

PIDDIR="/run"
GO2RTC_PID="${PIDDIR}/go2rtc.pid"
FFMPEG_PID="${PIDDIR}/ffmpeg-stream.pid"
GO2RTC_CONF="/etc/go2rtc.yaml"

#RES="320x240"
RES="640x480"

FRAMERATE=10

# Get the device name of the webcam
DEV=`v4l2-ctl --list-devices | grep -A1 "GENERAL WEBCAM:" | tail -1`

FFMPEG_CALL="ffmpeg -nostats -loglevel warning -fflags +discardcorrupt -f v4l2 -input_format mjpeg -video_size $RES -framerate $FRAMERATE -i $DEV -c:v libx264 -preset ultrafast -tune zerolatency -g 15 -keyint_min 15 -x264-params repeat-headers=1 -b:v 400k -maxrate 400k -bufsize 400k -f rtsp -rtsp_flags prefer_tcp -rtsp_transport tcp rtsp://127.0.0.1:8554/webcam" 

# echo $FFMPEG_CALL

is_running()
{
    local pidfile="$1"
    local pid=""

    if [ -f "$pidfile" ]
    then
        pid=$(cat "$pidfile")

        if kill -0 "$pid" 2>/dev/null
        then
            return 0
        fi
    fi

    return 1
}

start_stream()
{
    if is_running "$FFMPEG_PID"
    then
        logger -t "$TAG" "Stream bereits gestartet"
    else
        logger -t "$TAG" "Starte go2rtc"

        go2rtc --config "$GO2RTC_CONF" --daemon 2>&1 > /dev/null

        sleep 2

        pgrep -x go2rtc > "$GO2RTC_PID"

        logger -t "$TAG" "Starte ffmpeg"

        $FFMPEG_CALL 2>&1 | logger -t "$TAG" &

        echo $! > "$FFMPEG_PID"

        logger -t "$TAG" "Stream gestartet"
    fi
}

stop_stream()
{
    local pid=""

    if is_running "$FFMPEG_PID"
    then
        pid=$(cat "$FFMPEG_PID")

        logger -t "$TAG" "Stoppe ffmpeg PID=$pid"

        kill -2 "$pid"

        rm -f "$FFMPEG_PID"
    fi

    if is_running "$GO2RTC_PID"
    then
        pid=$(cat "$GO2RTC_PID")

        logger -t "$TAG" "Stoppe go2rtc PID=$pid"

        kill "$pid"

        rm -f "$GO2RTC_PID"
    fi
}

status_stream()
{
    if is_running "$GO2RTC_PID"
    then
        echo "go2rtc: RUNNING"
    else
        echo "go2rtc: STOPPED"
    fi

    if is_running "$FFMPEG_PID"
    then
        echo "ffmpeg : RUNNING"
    else
        echo "ffmpeg : STOPPED"
    fi
}

case "$1" in
    start)
        start_stream
        ;;
    stop)
        stop_stream
        ;;
    restart)
        stop_stream
        sleep 1
        start_stream
        ;;
    status)
        status_stream
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac

exit 0
