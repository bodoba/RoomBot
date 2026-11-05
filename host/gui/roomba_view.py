"""Animated top view of the Roomba.

Coordinates: the origin is the centre of the robot, the front points UP.
One unit is about 1 mm (the Roomba has a diameter of 340 mm).
"""

import math

from PySide6.QtCore import QPointF, QRectF, Qt, QTimer
from PySide6.QtGui import QBrush, QColor, QFont, QPainter, QPen, QPolygonF
from PySide6.QtWidgets import QWidget

from models import SensorState

BODY_RADIUS = 170.0                     # mm
FRAME_MS = 33                           # ~30 frames per second
EASING = 0.30                           # 0..1, higher = faster reaction

# Cliff sensor positions: angle from the front in degrees (clockwise).
CLIFF_ANGLES = (-65.0, -22.0, 22.0, 65.0)   # left, front-left, front-right, right
WALL_ANGLE = 70.0                       # wall sensor, front right side

COLOR_OK = QColor(70, 200, 110)
COLOR_ALARM = QColor(235, 60, 60)
COLOR_BODY = QColor(55, 58, 64)
COLOR_IDLE = QColor(120, 125, 135)


def lerp_color(a: QColor, b: QColor, t: float) -> QColor:
    """Blend colour a to colour b. t=0 gives a, t=1 gives b."""
    t = max(0.0, min(1.0, t))
    return QColor(
        int(a.red() + (b.red() - a.red()) * t),
        int(a.green() + (b.green() - a.green()) * t),
        int(a.blue() + (b.blue() - a.blue()) * t),
    )


def polar(radius: float, angle_deg: float) -> QPointF:
    """Point at 'radius' and 'angle_deg' measured clockwise from the front."""
    a = math.radians(angle_deg)
    return QPointF(radius * math.sin(a), -radius * math.cos(a))


class RoombaView(QWidget):
    """Custom widget that paints the robot and animates the sensors."""

    # Names of all animated values.
    _NAMES = ("bump_l", "bump_r", "drop_l", "drop_r", "wall",
              "cliff0", "cliff1", "cliff2", "cliff3")

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(420, 420)

        self._state = None                              # latest SensorState or None
        self._target = {n: 0.0 for n in self._NAMES}    # where values should go
        self._level = {n: 0.0 for n in self._NAMES}     # what is drawn now
        self._phase = 0.0                               # drives pulses and waves

        # The animation timer calls _tick() about 30 times per second.
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._tick)
        self._timer.start(FRAME_MS)

    # ------------------------------------------------------------------
    # Public interface
    # ------------------------------------------------------------------
    def set_state(self, state) -> None:
        """Set the newest sensor data, or None if no data is available."""
        self._state = state
        if state is None:
            for n in self._NAMES:
                self._target[n] = 0.0
            return
        self._target["bump_l"] = 1.0 if state.bump_left else 0.0
        self._target["bump_r"] = 1.0 if state.bump_right else 0.0
        self._target["drop_l"] = 1.0 if state.wheeldrop_left else 0.0
        self._target["drop_r"] = 1.0 if state.wheeldrop_right else 0.0
        self._target["wall"] = 1.0 if state.wall else 0.0
        for i in range(4):
            self._target[f"cliff{i}"] = 1.0 if state.cliff[i] else 0.0

    # ------------------------------------------------------------------
    # Animation
    # ------------------------------------------------------------------
    def _tick(self) -> None:
        """Move each displayed level a bit closer to its target, then repaint."""
        for n in self._NAMES:
            self._level[n] += (self._target[n] - self._level[n]) * EASING
        self._phase += 0.15
        self.update()                   # schedules paintEvent()

    # ------------------------------------------------------------------
    # Painting
    # ------------------------------------------------------------------
    def paintEvent(self, event) -> None:
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)

        # Move the origin to the centre and scale so the robot always fits.
        side = min(self.width(), self.height())
        p.translate(self.width() / 2, self.height() / 2)
        p.scale(side / 440.0, side / 440.0)

        self._draw_body(p)
        self._draw_wheels(p)
        self._draw_bumpers(p)
        self._draw_cliff_sensors(p)
        self._draw_wall_sensor(p)
        self._draw_center_text(p)
        p.end()

    def _draw_body(self, p: QPainter) -> None:
        p.setPen(QPen(QColor(25, 25, 28), 3))
        p.setBrush(QBrush(COLOR_BODY))
        p.drawEllipse(QPointF(0, 0), BODY_RADIUS, BODY_RADIUS)

        # Small triangle on the front shows the driving direction.
        p.setPen(Qt.NoPen)
        p.setBrush(QBrush(COLOR_IDLE))
        p.drawPolygon(QPolygonF([QPointF(0, -120), QPointF(-18, -90),
                                 QPointF(18, -90)]))

    def _draw_wheels(self, p: QPainter) -> None:
        # A wheel turns red when its wheel-drop sensor is active.
        for side, name in ((-1, "drop_l"), (1, "drop_r")):
            color = lerp_color(QColor(30, 30, 32), COLOR_ALARM, self._level[name])
            p.setPen(Qt.NoPen)
            p.setBrush(QBrush(color))
            p.drawRoundedRect(QRectF(side * 158 - 9, -10, 18, 70), 4, 4)

    def _draw_bumpers(self, p: QPainter) -> None:
        # drawArc uses 1/16 degrees, 0 = 3 o'clock, counter-clockwise.
        rect = QRectF(-BODY_RADIUS, -BODY_RADIUS, 2 * BODY_RADIUS, 2 * BODY_RADIUS)
        for name, start_deg in (("bump_l", 92), ("bump_r", 38)):
            color = lerp_color(COLOR_IDLE, COLOR_ALARM, self._level[name])
            p.setPen(QPen(color, 14, Qt.SolidLine, Qt.FlatCap))
            p.drawArc(rect, int(start_deg * 16), int(50 * 16))

    def _draw_cliff_sensors(self, p: QPainter) -> None:
        for i, angle in enumerate(CLIFF_ANGLES):
            level = self._level[f"cliff{i}"]
            pos = polar(148, angle)
            color = lerp_color(COLOR_OK, COLOR_ALARM, level)

            # A pulsing ring appears while the cliff alarm is active.
            if level > 0.05:
                pulse = 0.5 + 0.5 * math.sin(self._phase * 2.0)
                ring = QColor(color)
                ring.setAlphaF(level * (0.4 + 0.6 * pulse))
                p.setPen(QPen(ring, 3))
                p.setBrush(Qt.NoBrush)
                p.drawEllipse(pos, 12 + 10 * pulse * level, 12 + 10 * pulse * level)

            p.setPen(QPen(QColor(20, 20, 20), 2))
            p.setBrush(QBrush(color))
            p.drawEllipse(pos, 9, 9)

    def _draw_wall_sensor(self, p: QPainter) -> None:
        level = self._level["wall"]
        origin = polar(BODY_RADIUS - 4, WALL_ANGLE)

        # Three expanding circles ("waves") while a wall is detected.
        if level > 0.05:
            for k in range(3):
                grow = (self._phase * 8.0 + k * 14.0) % 42.0   # 0..42
                color = QColor(90, 170, 255)
                color.setAlphaF(level * (1.0 - grow / 42.0))
                p.setPen(QPen(color, 3))
                p.setBrush(Qt.NoBrush)
                p.drawEllipse(origin, 8 + grow, 8 + grow)

        color = lerp_color(COLOR_IDLE, QColor(90, 170, 255), level)
        p.setPen(QPen(QColor(20, 20, 20), 2))
        p.setBrush(QBrush(color))
        p.drawEllipse(origin, 7, 7)

    def _draw_center_text(self, p: QPainter) -> None:
        p.setPen(QColor(230, 230, 235))
        if self._state is None:
            p.setFont(QFont("Helvetica", 16, QFont.Bold))
            p.drawText(QRectF(-100, -20, 200, 40), Qt.AlignCenter, "NO DATA")
            return
        p.setFont(QFont("Helvetica", 18, QFont.Bold))
        p.drawText(QRectF(-100, -30, 200, 30), Qt.AlignCenter,
                   self._state.mode_name)
        p.setFont(QFont("Helvetica", 12))
        p.drawText(QRectF(-100, 0, 200, 24), Qt.AlignCenter,
                   f"{self._state.battery_percent:.0f} %")