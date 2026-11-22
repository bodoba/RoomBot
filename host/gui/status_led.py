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