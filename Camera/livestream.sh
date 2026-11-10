#!/bin/bash

TAG="roomcam"

PIDDIR="/run"
GO2RTC_PID="${PIDDIR}/go2rtc.pid"
FFMPEG_PID="${PIDDIR}/ffmpeg-stream.pid"

RES="320x240"
FRAMERATE=10

DEV=`v4l2-ctl --list-devices | grep -A1 "GENERAL WEBCAM:" | tail -1`

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

        go2rtc --daemon 2>&1 > /dev/null

        sleep 2

        pgrep -x go2rtc > "$GO2RTC_PID"

        logger -t "$TAG" "Starte ffmpeg"

        ffmpeg \
            -nostats \
            -loglevel warning \
            -f v4l2 \
            -input_format mjpeg \
            -video_size "$RES" \
            -framerate "$FRAMERATE" \
            -i $DEV \
            -c:v libx264 \
            -preset ultrafast \
            -tune zerolatency \
            -g 15 \
            -keyint_min 15 \
            -x264-params repeat-headers=1 \
            -b:v 400k \
            -maxrate 400k \
            -bufsize 400k \
            -f rtsp \
            -rtsp_transport tcp \
            rtsp://127.0.0.1:8554/webcam \
            2>&1 | logger -t "$TAG" &

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

        kill "$pid"

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
