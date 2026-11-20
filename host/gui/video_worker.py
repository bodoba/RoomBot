"""Background thread: read the stream, process each frame, emit a QImage.

The processing pipeline is a list of functions (add_processor). Each one
receives a BGR numpy frame and must return a BGR numpy frame. This is the
place for object detection, drawing boxes, etc. later.
"""

import logging
from typing import Callable, List

import cv2
import numpy as np
from PySide6.QtCore import QThread, Signal
from PySide6.QtGui import QImage

import config

log = logging.getLogger("roomba.video")

# Type of a processing step: frame in, frame out (both BGR numpy arrays).
FrameProcessor = Callable[[np.ndarray], np.ndarray]


class VideoWorker(QThread):
    """Reads frames from the stream URL without blocking the GUI."""

    # Finished image for the GUI thread (Qt delivers it thread-safely).
    frameReady = Signal(QImage)
    # "connecting", "streaming" or "stopped": for status messages in the GUI.
    streamStateChanged = Signal(str)

    def __init__(self, url: str = config.VIDEO_STREAM_URL, parent=None):
        super().__init__(parent)
        self._url = url
        self._running = False                       # Flag to end the loop.
        self._processors: List[FrameProcessor] = []

    # ------------------------------------------------------------------
    # Public interface (called from the GUI thread)
    # ------------------------------------------------------------------
    def add_processor(self, func: FrameProcessor) -> None:
        """Append a processing step, e.g. an object detector."""
        self._processors.append(func)

    def start_stream(self) -> None:
        """Start reading (does nothing if the thread already runs)."""
        self._running = True
        if not self.isRunning():
            self.start()                # Calls run() in the new thread.

    def stop_stream(self) -> None:
        """Ask the loop to end and wait until the thread has finished."""
        self._running = False
        # The wait is limited, because a blocked read ends after its timeout.
        self.wait(config.VIDEO_TIMEOUT_MS + 2000)

    # ------------------------------------------------------------------
    # Thread code
    # ------------------------------------------------------------------
    def run(self) -> None:
        """Outer loop: (re)connect. Inner loop: read and process frames."""
        while self._running:
            self.streamStateChanged.emit("connecting")
            cap = self._open()
            if cap is None:
                self._sleep(config.VIDEO_RETRY_S)   # Wait, then try again.
                continue

            self.streamStateChanged.emit("streaming")
            while self._running:
                ok, frame = cap.read()      # Blocks until a frame arrives.
                if not ok:
                    log.warning("Stream lost, reconnecting")
                    break                   # Back to the outer loop.
                frame = self._apply_processors(frame)
                self.frameReady.emit(self._to_qimage(frame))
            cap.release()

            if self._running:               # Avoid a hot reconnect loop.
                self._sleep(config.VIDEO_RETRY_S)

        self.streamStateChanged.emit("stopped")

    def _open(self):
        """Open the stream. Returns a VideoCapture or None on failure."""
        # Timeouts prevent read() from blocking forever if the server dies.
        cap = cv2.VideoCapture(self._url, cv2.CAP_FFMPEG, [
            cv2.CAP_PROP_OPEN_TIMEOUT_MSEC, config.VIDEO_TIMEOUT_MS,
            cv2.CAP_PROP_READ_TIMEOUT_MSEC, config.VIDEO_TIMEOUT_MS,
        ])
        if cap.isOpened():
            return cap
        cap.release()
        log.warning("Cannot open stream %s", self._url)
        return None

    def _apply_processors(self, frame: np.ndarray) -> np.ndarray:
        """Run all processing steps in order and return the result."""
        for func in list(self._processors):         # Copy: list may change.
            frame = func(frame)
        return frame

    @staticmethod
    def _to_qimage(frame: np.ndarray) -> QImage:
        """Convert an OpenCV BGR frame into an independent QImage."""
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)    # Qt expects RGB.
        h, w, _ = rgb.shape
        # copy() detaches the QImage from the numpy buffer, which is freed.
        return QImage(rgb.data, w, h, 3 * w, QImage.Format_RGB888).copy()

    def _sleep(self, seconds: float) -> None:
        """Sleep in small steps, so stop_stream() reacts quickly."""
        remaining = int(seconds * 1000)
        while self._running and remaining > 0:
            self.msleep(100)
            remaining -= 100