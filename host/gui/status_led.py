"""Small round indicator light (LED) for the connection status."""

from PySide6.QtCore import Qt
from PySide6.QtGui import QBrush, QColor, QPainter, QPen
from PySide6.QtWidgets import QWidget


class StatusLed(QWidget):
    """A round LED. Green = connected, red = failed, orange = connecting."""

    # Map each state name to its colour.
    COLORS = {
        "connected": QColor(60, 200, 90),
        "failed": QColor(230, 60, 60),
        "connecting": QColor(240, 170, 40),
    }

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(16, 16)       # The LED has a fixed size.
        self._state = "connecting"      # Initial state until we know more.

    def set_state(self, state: str) -> None:
        """Set 'connected', 'failed' or 'connecting' and repaint."""
        self._state = state
        self.update()                   # Schedules paintEvent().

    def paintEvent(self, event) -> None:
        """Draw a filled circle in the colour of the current state."""
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)   # Smooth edges.
        p.setPen(QPen(QColor(30, 30, 30), 1))    # Thin dark outline.
        p.setBrush(QBrush(self.COLORS[self._state]))
        p.drawEllipse(1, 1, self.width() - 2, self.height() - 2)
        p.end()