from __future__ import annotations

from typing import Optional

from PySide6.QtCore import QObject, Signal

from upload_job import UploadJob


class PicocSession(QObject):
    send_requested = Signal(str)
    console_text = Signal(str)
    mode_changed = Signal(str)
    status_changed = Signal(str)
    upload_finished = Signal(bool, str)
    upload_state_changed = Signal(bool)

    def __init__(self, parent: QObject | None = None) -> None:
        super().__init__(parent)
        self._connected = False
        self._mode = "DISCONNECTED"
        self._observed_mode = "UNKNOWN"
        self._prompt_window = ""
        self._last_ready_marker_index = -1
        self._upload_job: Optional[UploadJob] = None

    @property
    def mode(self) -> str:
        return self._mode

    def set_connected(self, connected: bool) -> None:
        self._connected = connected
        self._prompt_window = ""
        self._last_ready_marker_index = -1
        self._observed_mode = "UNKNOWN" if connected else "DISCONNECTED"
        self._set_mode("UNKNOWN" if connected else "DISCONNECTED")
        if connected:
            self.status_changed.emit("已连接，等待 PicoC 提示符...")
        else:
            self.status_changed.emit("已断开连接。")
            self._cancel_upload("上传过程中连接已断开。")

    def handle_incoming_text(self, text: str) -> None:
        self.console_text.emit(text)
        self._prompt_window = (self._prompt_window + text)[-512:]
        ready_marker = "ready for next file"
        ready_index = self._prompt_window.lower().rfind(ready_marker)
        if ready_index >= 0 and ready_index != self._last_ready_marker_index:
            self._last_ready_marker_index = ready_index
            self.status_changed.emit("设备已准备好接收下一个文件。")
            if self._upload_job is not None:
                self._upload_job.observe_ready_for_next_file()

        detected_mode: Optional[str] = None
        last_picoc = self._prompt_window.rfind("picoc> ")
        last_load = self._prompt_window.rfind("load> ")
        if last_picoc >= 0 or last_load >= 0:
            detected_mode = "REPL" if last_picoc > last_load else "LOAD"

        if detected_mode is not None:
            self._observed_mode = detected_mode
            if self._upload_job is None:
                self._set_mode(detected_mode)
            else:
                self._upload_job.observe_mode(detected_mode)

    def send_manual(self, text: str) -> bool:
        if not self._connected:
            self.status_changed.emit("当前未连接。")
            return False
        if self._upload_job is not None:
            self.status_changed.emit("正在上传文件，已禁止手工发送。")
            return False

        payload = text.replace("\r\n", "\n").replace("\r", "\n")
        if not payload.endswith("\n"):
            payload += "\n"
        self.send_requested.emit(payload.replace("\n", "\r\n"))
        self.status_changed.emit("手工命令已发送。")
        return True

    def start_upload(self, source_text: str) -> bool:
        if not self._connected:
            self.status_changed.emit("当前未连接。")
            return False
        if self._upload_job is not None:
            self.status_changed.emit("已有上传任务正在运行。")
            return False

        job = UploadJob(source_text, self)
        self._upload_job = job
        job.send_requested.connect(self.send_requested)
        job.progress_changed.connect(self.status_changed)
        job.finished.connect(self._handle_upload_finished)
        self.upload_state_changed.emit(True)
        self._set_mode("BUSY")
        job.start(self._observed_mode)
        return True

    def abort(self) -> bool:
        if not self._connected:
            self.status_changed.emit("当前未连接。")
            return False

        self.send_requested.emit(":abort\r\n")
        self.status_changed.emit("已发送中止命令。")
        if self._upload_job is not None:
            self._cancel_upload("用户已中止上传。")
        return True

    def _handle_upload_finished(self, success: bool, message: str) -> None:
        self._upload_job = None
        self.upload_state_changed.emit(False)
        self._set_mode(self._observed_mode)
        self.status_changed.emit(message)
        self.upload_finished.emit(success, message)

    def _cancel_upload(self, message: str) -> None:
        if self._upload_job is not None:
            job = self._upload_job
            self._upload_job = None
            job.finished.disconnect(self._handle_upload_finished)
            self.upload_state_changed.emit(False)
            job.cancel()
        self._set_mode(self._observed_mode if self._connected else "DISCONNECTED")
        self.upload_finished.emit(False, message)

    def _set_mode(self, mode: str) -> None:
        if self._mode != mode:
            self._mode = mode
            self.mode_changed.emit(mode)
