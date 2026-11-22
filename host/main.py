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
"""Entry point of the Roomba console."""

import logging
import sys

from PySide6.QtWidgets import QApplication

from gui.main_window import MainWindow
from mqtt_client import RoombaMqtt


def main() -> int:
    logging.basicConfig(level=logging.INFO,
                        format="%(asctime)s %(name)s %(levelname)s %(message)s")
    app = QApplication(sys.argv)

    mqtt = RoombaMqtt()
    window = MainWindow(mqtt)
    window.show()

    # Connect at startup. This happens AFTER the window has connected its
    # slots to the MQTT signals, so the first status change is not missed.
    window.connect_to_broker()

    # Stop the video thread and the MQTT thread cleanly when the window closes.
    app.aboutToQuit.connect(window.shutdown)
    app.aboutToQuit.connect(mqtt.stop)
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())