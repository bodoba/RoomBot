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
"""Widget that shows video frames and keeps the aspect ratio."""

from PySide6.QtCore import Qt
from PySide6.QtGui import QImage, QPixmap
from PySide6.QtWidgets import QLabel, QSizePolicy


class VideoView(QLabel):
    """Displays the most recent frame, scaled to the widget size.

    It only DISPLAYS images. All processing happens before, in VideoWorker.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAlignment(Qt.AlignCenter)
        self.setMinimumSize(320, 240)
        # "Ignored" lets the user shrink the dock, even if the frame is big.
        self.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Ignored)
        self.setStyleSheet("background-color: black; color: #aaaaaa;")

        # Keep the original image so we can rescale it when the widget resizes.
        self._image = None
        self.show_message("Camera off")

    def show_message(self, text: str) -> None:
        """Show a text instead of video (e.g. 'Camera off')."""
        self._image = None
        self.clear()                    # Remove the old pixmap.
        self.setText(text)

    def show_image(self, image: QImage) -> None:
        """Show a new frame (called in the GUI thread through a signal)."""
        self._image = image
        self._update_pixmap()

    def resizeEvent(self, event) -> None:
        """Rescale the current frame when the widget size changes."""
        super().resizeEvent(event)
        self._update_pixmap()

    def _update_pixmap(self) -> None:
        """Scale the stored image to the widget size, keeping its aspect ratio."""
        if self._image is None:
            return
        scaled = QPixmap.fromImage(self._image).scaled(
            self.size(), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        self.setPixmap(scaled)