"""Central place for all settings and MQTT topic names."""

# Address of the MQTT broker (usually running on the Orange Pi).
BROKER_HOST = "192.168.100.26"
BROKER_PORT = 1883

# All topics of the daemon start with this prefix.
PREFIX = "roombot"

# --- Topics published by the daemon (we subscribe) ---
TOPIC_STATE = f"{PREFIX}/sensor/state"          # JSON, ~10 Hz
TOPIC_HEARTBEAT = f"{PREFIX}/sensor/heartbeat"  # text, 1 Hz

# --- Topics the daemon will subscribe to (we publish) ---
# NOTE: proposal, not yet implemented in the daemon.
TOPIC_CMD_DRIVE = f"{PREFIX}/cmd/drive"         # {"velocity": mm/s, "radius": mm}
TOPIC_CMD_DISPLAY = f"{PREFIX}/cmd/display"     # {"text": "ABCD"}
TOPIC_CMD_START = f"{PREFIX}/cmd/start"         # {}
TOPIC_CMD_POWER_DOWN = f"{PREFIX}/cmd/power_down"  # {}
TOPIC_CMD_CAMERA = f"{PREFIX}/cmd/camera"       # {"enabled": true|false}

# If no state message arrives for this long, the GUI shows "NO DATA".
STALE_AFTER_S = 1.0

# Drive speed limits offered by the GUI (mm/s). The daemon must limit too.
SPEED_MIN = 50
SPEED_MAX = 300
SPEED_DEFAULT = 100

# --- Video (go2rtc server) ---
# stream.html is only the browser player page. For frame processing we need
# the raw stream, so we use go2rtc's MJPEG endpoint on the same server.
VIDEO_PAGE_URL = "http://192.168.100.115:1984/stream.html?src=webcam"
VIDEO_STREAM_URL = "rtsp://192.168.100.115:8554/webcam"

# Seconds to wait before the worker retries a failed stream connection.
VIDEO_RETRY_S = 2.0
# Timeout in ms for opening / reading the stream (avoids hanging threads).
VIDEO_TIMEOUT_MS = 10000
