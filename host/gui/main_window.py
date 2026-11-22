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
"""Main window: robot view in the centre, panels left and right."""

import time
from functools import partial

from PySide6.QtCore import Qt, QTimer
from PySide6.QtGui import QImage
from PySide6.QtWidgets import (QDockWidget, QFormLayout, QGridLayout,
                               QGroupBox, QHBoxLayout, QLabel, QLineEdit,
                               QMainWindow, QPushButton, QSlider, QVBoxLayout,
                               QWidget)

import config
from gui.roomba_view import RoombaView
from gui.status_led import StatusLed
from gui.video_view import VideoView
from gui.video_worker import VideoWorker
from mqtt_client import RoombaMqtt
from models import SensorState


class MainWindow(QMainWindow):
    """Connects the MQTT class and the video worker to the widgets."""

    def __init__(self, mqtt: RoombaMqtt):
        super().__init__()
        self._mqtt = mqtt
        self._connected = False         # Is the broker connection up right now?
        self._wanted = False            # Does the user want a connection?
        self._last_rx = 0.0             # Time of the last state message

        self.setWindowTitle("Roomba Console")
        self.resize(1150, 720)

        # The robot view is the central widget.
        self._view = RoombaView()
        self.setCentralWidget(self._view)

        # Video pipeline: the worker thread reads and processes the frames,
        # the VideoView only displays the finished result.
        self._video = VideoWorker(parent=self)
        # Example for later: self._video.add_processor(my_detector)

        # Drive command that is repeated while a button is held (None = idle).
        # The daemon watchdog stops the robot if the messages stop arriving.
        self._drive_cmd = None
        self._drive_timer = QTimer(self)
        self._drive_timer.setInterval(100)          # 10 Hz
        self._drive_timer.timeout.connect(self._resend_drive)

        self._build_control_dock()
        self._build_video_dock()        # Added before telemetry: shown on top.
        self._build_telemetry_dock()
        self.statusBar().showMessage("Disconnected")

        # Connect MQTT signals to our slots (methods below).
        mqtt.connectionChanged.connect(self._on_connection)
        mqtt.stateReceived.connect(self._on_state)
        mqtt.packetsLost.connect(self._on_lost)

        # Connect video signals to our slots.
        self._video.frameReady.connect(self._on_frame)
        self._video.streamStateChanged.connect(self._on_stream_state)

        # Every 250 ms check whether data stopped arriving.
        self._stale_timer = QTimer(self)
        self._stale_timer.timeout.connect(self._check_stale)
        self._stale_timer.start(250)

    # ------------------------------------------------------------------
    # Public interface (called from main.py)
    # ------------------------------------------------------------------
    def connect_to_broker(self) -> None:
        """Start connecting to the broker given in the host field."""
        self._wanted = True
        self._led.set_state("connecting")           # Orange until we know more.
        self._conn_label.setText("Connecting ...")
        self._connect_btn.setText("Disconnect")     # Allows aborting retries.
        self.statusBar().showMessage("Connecting ...")
        self._mqtt.start(self._host_edit.text().strip())

    def shutdown(self) -> None:
        """Stop background threads (connected to QApplication.aboutToQuit)."""
        self._video.stop_stream()

    # ------------------------------------------------------------------
    # Building the panels
    # ------------------------------------------------------------------
    def _build_control_dock(self) -> None:
        panel = QWidget()
        layout = QVBoxLayout(panel)

        # --- Connection: host, connect button and status LED ---
        box = QGroupBox("Connection")
        form = QVBoxLayout(box)
        self._host_edit = QLineEdit(config.BROKER_HOST)
        self._connect_btn = QPushButton("Connect")
        self._connect_btn.clicked.connect(self._toggle_connection)
        form.addWidget(self._host_edit)
        form.addWidget(self._connect_btn)

        # Status row: LED (green = connected, red = failed) and a text.
        status_row = QHBoxLayout()
        self._led = StatusLed()
        self._led.set_state("failed")               # Red until connected.
        self._conn_label = QLabel("Disconnected")
        status_row.addWidget(self._led)
        status_row.addWidget(self._conn_label, 1)
        form.addLayout(status_row)
        layout.addWidget(box)

        # --- Camera switch: sends an MQTT message and starts/stops the video ---
        box = QGroupBox("Camera")
        v = QVBoxLayout(box)
        self._camera_btn = QPushButton("Camera OFF")
        self._camera_btn.setCheckable(True)         # Stays pressed = switch.
        self._camera_btn.setEnabled(False)          # Needs an MQTT connection.
        self._camera_btn.setStyleSheet(
            "QPushButton:checked { background-color: #3cb371; color: white; }")
        self._camera_btn.toggled.connect(self._on_camera_toggled)
        v.addWidget(self._camera_btn)
        layout.addWidget(box)

        # --- Drive pad: moves while a button is held, stops on release ---
        box = QGroupBox("Drive")
        grid = QGridLayout(box)
        self._speed = QSlider(Qt.Horizontal)
        self._speed.setRange(config.SPEED_MIN, config.SPEED_MAX)
        self._speed.setValue(config.SPEED_DEFAULT)
        self._speed_label = QLabel()
        self._speed.valueChanged.connect(
            lambda v: self._speed_label.setText(f"{v} mm/s"))
        self._speed_label.setText(f"{self._speed.value()} mm/s")

        # (text, row, column, velocity factor, radius)
        pad = (("▲", 0, 1, 1, 32768), ("◀", 1, 0, 1, 1),
               ("▶", 1, 2, 1, -1), ("▼", 2, 1, -1, 32768))
        for text, row, col, factor, radius in pad:
            btn = QPushButton(text)
            btn.setMinimumSize(50, 50)
            # pressed/released fire when the mouse button goes down/up.
            btn.pressed.connect(partial(self._drive, factor, radius))
            btn.released.connect(self._drive_stop)
            grid.addWidget(btn, row, col)
        stop_btn = QPushButton("■")
        stop_btn.setMinimumSize(50, 50)
        stop_btn.clicked.connect(self._drive_stop)
        grid.addWidget(stop_btn, 1, 1)
        grid.addWidget(self._speed, 3, 0, 1, 3)
        grid.addWidget(self._speed_label, 4, 0, 1, 3)
        layout.addWidget(box)

        # --- Robot commands ---
        box = QGroupBox("Robot")
        v = QVBoxLayout(box)
        start_btn = QPushButton("Start (Safe mode)")
        start_btn.clicked.connect(self._mqtt.start_robot)
        power_btn = QPushButton("Power down")
        power_btn.clicked.connect(self._mqtt.power_down)
        v.addWidget(start_btn)
        v.addWidget(power_btn)

        row = QHBoxLayout()
        self._display_edit = QLineEdit()
        self._display_edit.setMaxLength(4)
        self._display_edit.setPlaceholderText("4 chars")
        send_btn = QPushButton("Display")
        send_btn.clicked.connect(
            lambda: self._mqtt.display(self._display_edit.text()))
        row.addWidget(self._display_edit)
        row.addWidget(send_btn)
        v.addLayout(row)
        layout.addWidget(box)

        layout.addStretch(1)

        dock = QDockWidget("Control", self)
        dock.setWidget(panel)
        self.addDockWidget(Qt.LeftDockWidgetArea, dock)

    def _build_video_dock(self) -> None:
        """Dock with the video display (right side, above the telemetry)."""
        self._video_view = VideoView()
        dock = QDockWidget("Camera", self)
        dock.setWidget(self._video_view)
        self.addDockWidget(Qt.RightDockWidgetArea, dock)

    def _build_telemetry_dock(self) -> None:
        panel = QWidget()
        form = QFormLayout(panel)

        # One label per value, kept in a dict so we can update them by name.
        self._labels = {}
        for key, title in (("mode", "Mode"), ("battery", "Battery"),
                           ("voltage", "Voltage"), ("current", "Current"),
                           ("temp", "Temperature"), ("seq", "Sequence"),
                           ("lost", "Lost messages")):
            label = QLabel("-")
            self._labels[key] = label
            form.addRow(title, label)

        dock = QDockWidget("Telemetry", self)
        dock.setWidget(panel)
        self.addDockWidget(Qt.RightDockWidgetArea, dock)

    # ------------------------------------------------------------------
    # Slots: driving
    # ------------------------------------------------------------------
    def _drive(self, factor: int, radius: int) -> None:
        """Start driving and keep repeating the command while the button is held."""
        self._drive_cmd = (factor * self._speed.value(), radius)
        self._mqtt.drive(*self._drive_cmd)      # send the first message at once
        self._drive_timer.start()               # then repeat every 100 ms

    def _resend_drive(self) -> None:
        """Timer slot: repeat the current drive command (keeps the watchdog fed)."""
        if self._drive_cmd is not None:
            self._mqtt.drive(*self._drive_cmd)

    def _clear_drive(self) -> None:
        """Stop repeating the drive command locally (sends nothing)."""
        self._drive_timer.stop()
        self._drive_cmd = None

    def _drive_stop(self) -> None:
        """Stop repeating and tell the robot to stop."""
        self._clear_drive()
        self._mqtt.stop_driving()

    # ------------------------------------------------------------------
    # Slots: connection
    # ------------------------------------------------------------------
    def _toggle_connection(self) -> None:
        """Connect button: connect, or disconnect / abort the retries."""
        if self._wanted:
            self._wanted = False
            self._mqtt.stop()
            self._connect_btn.setText("Connect")
        else:
            self.connect_to_broker()

    def _on_connection(self, connected: bool) -> None:
        """MQTT connection changed: update LED, texts and enabled controls."""
        self._connected = connected

        if connected:
            self._led.set_state("connected")                # Green
            text = "Connected"
        else:
            self._led.set_state("failed")                   # Red
            # While the user wants a connection, paho keeps retrying.
            text = ("Connection failed, retrying ..." if self._wanted
                    else "Disconnected")
        self._conn_label.setText(text)
        self.statusBar().showMessage(text)
        self._connect_btn.setText("Disconnect" if self._wanted else "Connect")

        # The camera switch needs a working MQTT link.
        self._camera_btn.setEnabled(connected)

        # Never keep a drive command alive across a connection change.
        self._clear_drive()
        if connected:
            self._mqtt.stop_driving()       # Make sure the robot stands still.
        else:
            self._view.set_state(None)
            self._set_camera_off()          # The camera state is now unknown.

    # ------------------------------------------------------------------
    # Slots: camera and video
    # ------------------------------------------------------------------
    def _on_camera_toggled(self, checked: bool) -> None:
        """Camera switch clicked: send the MQTT message and start/stop video."""
        self._camera_btn.setText("Camera ON" if checked else "Camera OFF")
        self._mqtt.camera(checked)
        if checked:
            self._video_view.show_message("Waiting for stream ...")
            # The worker retries until the server delivers the stream.
            self._video.start_stream()
        else:
            self._video.stop_stream()
            self._video_view.show_message("Camera off")

    def _set_camera_off(self) -> None:
        """Switch the camera UI off WITHOUT sending an MQTT message."""
        # blockSignals prevents toggled() from publishing a message.
        self._camera_btn.blockSignals(True)
        self._camera_btn.setChecked(False)
        self._camera_btn.setText("Camera OFF")
        self._camera_btn.blockSignals(False)
        self._video.stop_stream()
        self._video_view.show_message("Camera off")

    def _on_frame(self, image: QImage) -> None:
        """A processed frame arrived from the worker thread."""
        try:
            # Late frames still queued after switching off must not be shown.
            if self._camera_btn.isChecked():
                self._video_view.show_image(image)
        finally:
            self._video.frame_consumed()

    def _on_stream_state(self, state: str) -> None:
        """Worker reports its state: show a hint while no video is running."""
        if state == "connecting" and self._camera_btn.isChecked():
            self._video_view.show_message("Connecting to stream ...")

    # ------------------------------------------------------------------
    # Slots: sensor data
    # ------------------------------------------------------------------
    def _on_state(self, state: SensorState) -> None:
        self._last_rx = time.monotonic()
        self._view.set_state(state)
        self._labels["mode"].setText(state.mode_name)
        self._labels["battery"].setText(f"{state.battery_percent:.0f} %")
        self._labels["voltage"].setText(f"{state.voltage_mv / 1000:.2f} V")
        self._labels["current"].setText(f"{state.current_ma} mA")
        self._labels["temp"].setText(f"{state.temp_c} °C")
        self._labels["seq"].setText(str(state.seq))

    def _on_lost(self, total: int) -> None:
        self._labels["lost"].setText(str(total))

    def _check_stale(self) -> None:
        """Show 'NO DATA' if the daemon stopped sending."""
        if (self._connected and self._last_rx > 0
                and time.monotonic() - self._last_rx > config.STALE_AFTER_S):
            self._view.set_state(None)

            