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