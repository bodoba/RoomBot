"""Main window: robot view in the centre, panels left and right."""

import time
from functools import partial

from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import (QDockWidget, QFormLayout, QGridLayout,
                               QGroupBox, QHBoxLayout, QLabel, QLineEdit,
                               QMainWindow, QPushButton, QSlider, QVBoxLayout,
                               QWidget)

import config
from gui.roomba_view import RoombaView
from mqtt_client import RoombaMqtt
from models import SensorState


class MainWindow(QMainWindow):
    """Connects the MQTT class to the widgets."""

    def __init__(self, mqtt: RoombaMqtt):
        super().__init__()
        self._mqtt = mqtt
        self._connected = False
        self._last_rx = 0.0             # time of the last state message

        self.setWindowTitle("Roomba Console")
        self.resize(1150, 720)

        # The robot view is the central widget.
        self._view = RoombaView()
        self.setCentralWidget(self._view)

        self._build_control_dock()
        self._build_telemetry_dock()
        self.statusBar().showMessage("Disconnected")

        # Connect MQTT signals to our slots (methods below).
        mqtt.connectionChanged.connect(self._on_connection)
        mqtt.stateReceived.connect(self._on_state)
        mqtt.packetsLost.connect(self._on_lost)

        # Every 250 ms check whether data stopped arriving.
        self._stale_timer = QTimer(self)
        self._stale_timer.timeout.connect(self._check_stale)
        self._stale_timer.start(250)

    # ------------------------------------------------------------------
    # Building the panels
    # ------------------------------------------------------------------
    def _build_control_dock(self) -> None:
        panel = QWidget()
        layout = QVBoxLayout(panel)

        # --- Connection ---
        box = QGroupBox("Connection")
        form = QVBoxLayout(box)
        self._host_edit = QLineEdit(config.BROKER_HOST)
        self._connect_btn = QPushButton("Connect")
        self._connect_btn.clicked.connect(self._toggle_connection)
        form.addWidget(self._host_edit)
        form.addWidget(self._connect_btn)
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
            btn.released.connect(self._mqtt.stop_driving)
            grid.addWidget(btn, row, col)
        stop_btn = QPushButton("■")
        stop_btn.setMinimumSize(50, 50)
        stop_btn.clicked.connect(self._mqtt.stop_driving)
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
    # Slots: reactions to button clicks and MQTT signals
    # ------------------------------------------------------------------
    def _drive(self, factor: int, radius: int) -> None:
        """Send a drive command using the speed from the slider."""
        self._mqtt.drive(factor * self._speed.value(), radius)

    def _toggle_connection(self) -> None:
        if self._connected:
            self._mqtt.stop()
        else:
            self._mqtt.start(self._host_edit.text().strip())
            self.statusBar().showMessage("Connecting ...")

    def _on_connection(self, connected: bool) -> None:
        self._connected = connected
        self._connect_btn.setText("Disconnect" if connected else "Connect")
        self.statusBar().showMessage("Connected" if connected else "Disconnected")
        if not connected:
            self._view.set_state(None)

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