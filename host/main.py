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

    # Stop the MQTT thread cleanly when the window closes.
    app.aboutToQuit.connect(mqtt.stop)
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())