from __future__ import annotations

from enum import Enum, auto

from PySide6.QtCore import QObject, QTimer, Signal


class UploadPhase(Enum):
    IDLE = auto()
    WAIT_LOAD_PROMPT = auto()
    SENDING_LINES = auto()
    WAIT_EXEC_COMPLETE = auto()
    FINISHED = auto()


class UploadJob(QObject):
    send_requested = Signal(str)
    progress_changed = Signal(str)
    finished = Signal(bool, str)

    def __init__(self, source_text: str, parent: QObject | None = None) -> None:
        super().__init__(parent)
        normalized = source_text.replace("\r\n", "\n").replace("\r", "\n")
        self._lines = normalized.split("\n")
        self._line_index = 0
        self._phase = UploadPhase.IDLE
        self._timeout_timer = QTimer(self)
        self._timeout_timer.setSingleShot(True)
        self._timeout_timer.timeout.connect(self._handle_timeout)
        self._send_timer = QTimer(self)
        self._send_timer.setSingleShot(True)
        self._send_timer.timeout.connect(self._send_next_line)

    @property
    def phase(self) -> UploadPhase:
        return self._phase

    def start(self, current_mode: str) -> None:
        if current_mode == "LOAD":
            self._progress("正在发送文件...")
            self._phase = UploadPhase.SENDING_LINES
            self._send_timer.start(0)
        else:
            self._progress("正在请求进入文件模式...")
            self._phase = UploadPhase.WAIT_LOAD_PROMPT
            self.send_requested.emit(":load\r\n")
            self._timeout_timer.start(4000)

    def observe_mode(self, mode: str) -> None:
        if self._phase == UploadPhase.WAIT_LOAD_PROMPT and mode == "LOAD":
            self._timeout_timer.stop()
            self._progress("文件模式就绪，开始发送文件...")
            self._phase = UploadPhase.SENDING_LINES
            self._send_timer.start(0)
            return

        if self._phase == UploadPhase.WAIT_EXEC_COMPLETE and mode == "REPL":
            self._finish(True, "上传完成。")

    def observe_ready_for_next_file(self) -> None:
        if self._phase == UploadPhase.WAIT_EXEC_COMPLETE:
            self._finish(True, "上传完成。")

    def cancel(self) -> None:
        if self._phase != UploadPhase.FINISHED:
            self._finish(False, "上传已取消。")

    def _send_next_line(self) -> None:
        if self._phase != UploadPhase.SENDING_LINES:
            return

        if self._line_index < len(self._lines):
            self.send_requested.emit(self._lines[self._line_index] + "\r\n")
            self._line_index += 1
            self._send_timer.start(5)
            return

        self._phase = UploadPhase.WAIT_EXEC_COMPLETE
        self._progress("正在执行文件...")
        self.send_requested.emit(":end\r\n")
        self._timeout_timer.start(15000)

    def _handle_timeout(self) -> None:
        self._finish(False, "操作超时。")

    def _finish(self, success: bool, message: str) -> None:
        self._timeout_timer.stop()
        self._send_timer.stop()
        self._phase = UploadPhase.FINISHED
        self.finished.emit(success, message)

    def _progress(self, message: str) -> None:
        self.progress_changed.emit(message)
