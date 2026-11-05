"""MQTT connection to the Roomba daemon.

paho-mqtt calls our callbacks from its own network thread. A Qt GUI must only
be touched from the GUI thread. Therefore this class converts every event
into a Qt signal. Qt delivers signals across threads safely (queued).
"""

import json
import logging

import paho.mqtt.client as mqtt
from PySide6.QtCore import QObject, Signal

import config
from models import SensorState

log = logging.getLogger("roomba.mqtt")


class RoombaMqtt(QObject):
    """Receives sensor data and sends commands via MQTT."""

    # Emitted with True/False when the broker connection changes.
    connectionChanged = Signal(bool)
    # Emitted for each valid state message (argument: SensorState).
    stateReceived = Signal(object)
    # Emitted for each heartbeat message (argument: text).
    heartbeatReceived = Signal(str)
    # Emitted with the total number of lost state messages so far.
    packetsLost = Signal(int)

    def __init__(self, parent=None):
        super().__init__(parent)

        # State used to detect lost messages (QoS 0 may drop messages).
        self._last_seq = None
        self._lost_total = 0

        # paho-mqtt 2.x needs the callback API version as first argument.
        self._client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2, client_id="roomba-console"
        )
        # Reconnect automatically, wait 1 s ... 10 s between attempts.
        self._client.reconnect_delay_set(min_delay=1, max_delay=10)

        # Register the callbacks BEFORE connecting, so nothing is missed.
        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message = self._on_message

    # ------------------------------------------------------------------
    # Connection management
    # ------------------------------------------------------------------
    def start(self, host: str = config.BROKER_HOST,
              port: int = config.BROKER_PORT) -> None:
        """Start connecting in the background (does not block the GUI)."""
        self._last_seq = None
        self._lost_total = 0
        # connect_async + loop_start: paho connects and retries in its thread.
        self._client.connect_async(host, port, keepalive=30)
        self._client.loop_start()

    def stop(self) -> None:
        """Disconnect and stop the network thread."""
        self._client.disconnect()
        self._client.loop_stop()
        self.connectionChanged.emit(False)

    # ------------------------------------------------------------------
    # paho callbacks (run in the paho network thread!)
    # ------------------------------------------------------------------
    def _on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            log.error("MQTT connect failed: %s", reason_code)
            return
        log.info("MQTT connected")
        # Subscribe here (not in start) so it is repeated after a reconnect.
        client.subscribe([(config.TOPIC_STATE, 0), (config.TOPIC_HEARTBEAT, 0)])
        self.connectionChanged.emit(True)

    def _on_disconnect(self, client, userdata, disconnect_flags,
                       reason_code, properties):
        log.warning("MQTT disconnected: %s", reason_code)
        self.connectionChanged.emit(False)

    def _on_message(self, client, userdata, msg):
        # Payload arrives as bytes, decode it to text first.
        text = msg.payload.decode("utf-8", errors="replace")

        if msg.topic == config.TOPIC_STATE:
            try:
                state = SensorState.from_json(text)
            except ValueError as exc:
                log.warning("Ignoring message: %s", exc)
                return
            self._track_loss(state.seq)
            self.stateReceived.emit(state)
        elif msg.topic == config.TOPIC_HEARTBEAT:
            self.heartbeatReceived.emit(text)

    def _track_loss(self, seq: int) -> None:
        """Count gaps in the sequence numbers."""
        if self._last_seq is not None:
            if seq > self._last_seq + 1:
                self._lost_total += seq - self._last_seq - 1
                self.packetsLost.emit(self._lost_total)
            # If seq went backwards, the daemon restarted: no loss to count.
        self._last_seq = seq

    # ------------------------------------------------------------------
    # Commands (called from the GUI thread)
    # ------------------------------------------------------------------
    def _publish(self, topic: str, payload: dict) -> None:
        """Publish a JSON payload with QoS 1 (delivery is confirmed)."""
        self._client.publish(topic, json.dumps(payload), qos=1)

    def drive(self, velocity: int, radius: int = 32768) -> None:
        """Drive the Roomba.

        velocity: mm/s, negative = backwards.
        radius:   mm. 32768 = straight, 1 = turn left on the spot,
                  -1 = turn right on the spot.
        """
        self._publish(config.TOPIC_CMD_DRIVE,
                      {"velocity": int(velocity), "radius": int(radius)})

    def stop_driving(self) -> None:
        self.drive(0)

    def display(self, text: str) -> None:
        """Show up to 4 characters on the Roomba display."""
        self._publish(config.TOPIC_CMD_DISPLAY, {"text": text[:4]})

    def start_robot(self) -> None:
        self._publish(config.TOPIC_CMD_START, {})

    def power_down(self) -> None:
        self._publish(config.TOPIC_CMD_POWER_DOWN, {})