"""Windows-only ctypes adapters; operations are restricted to owned processes."""
import ctypes
from contextlib import contextmanager
import os
from pathlib import Path
import time
from verification_common import require


class Windows:
    def __init__(self):
        require(os.name == "nt", "Owned window verification requires Windows.")
        from ctypes import wintypes as w
        self.w = w
        self.user = ctypes.WinDLL("user32", use_last_error=True)
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        self.callback = ctypes.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
        class Rect(ctypes.Structure):
            _fields_ = [(n, w.LONG) for n in ("left", "top", "right", "bottom")]
        class Entry(ctypes.Structure):
            _fields_ = [("size", w.DWORD), ("usage", w.DWORD), ("pid", w.DWORD), ("heap", ctypes.c_size_t),
                        ("module", w.DWORD), ("threads", w.DWORD), ("parent", w.DWORD), ("priority", w.LONG), ("flags", w.DWORD), ("exe", w.WCHAR * 260)]
        self.rect, self.entry = Rect, Entry
        signatures = {
            "EnumWindows": ([self.callback, w.LPARAM], w.BOOL), "GetWindowTextW": ([w.HWND, w.LPWSTR, ctypes.c_int], ctypes.c_int),
            "GetClassNameW": ([w.HWND, w.LPWSTR, ctypes.c_int], ctypes.c_int), "GetWindowThreadProcessId": ([w.HWND, ctypes.POINTER(w.DWORD)], w.DWORD),
            "IsWindowVisible": ([w.HWND], w.BOOL), "IsIconic": ([w.HWND], w.BOOL), "ShowWindow": ([w.HWND, ctypes.c_int], w.BOOL),
            "GetForegroundWindow": ([], w.HWND), "SetForegroundWindow": ([w.HWND], w.BOOL), "DestroyWindow": ([w.HWND], w.BOOL),
            "PostMessageW": ([w.HWND, w.UINT, w.WPARAM, w.LPARAM], w.BOOL), "GetDlgItem": ([w.HWND, ctypes.c_int], w.HWND),
            "SendMessageTimeoutW": ([w.HWND, w.UINT, w.WPARAM, w.LPARAM, w.UINT, w.UINT, ctypes.POINTER(ctypes.c_size_t)], w.LPARAM),
            "GetClientRect": ([w.HWND, ctypes.POINTER(Rect)], w.BOOL), "SetThreadDpiAwarenessContext": ([ctypes.c_void_p], ctypes.c_void_p),
            "SetWindowPos": ([w.HWND, w.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, w.UINT], w.BOOL),
            "CreateWindowExW": ([w.DWORD, w.LPCWSTR, w.LPCWSTR, w.DWORD, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, w.HWND, w.HMENU, w.HINSTANCE, ctypes.c_void_p], w.HWND)}
        for name, (arguments, result) in signatures.items():
            function = getattr(self.user, name)
            function.argtypes, function.restype = arguments, result
        signatures = {
            "SetErrorMode": ([w.UINT], w.UINT), "CreateToolhelp32Snapshot": ([w.DWORD, w.DWORD], w.HANDLE),
            "Process32FirstW": ([w.HANDLE, ctypes.POINTER(Entry)], w.BOOL), "Process32NextW": ([w.HANDLE, ctypes.POINTER(Entry)], w.BOOL),
            "CloseHandle": ([w.HANDLE], w.BOOL), "OpenProcess": ([w.DWORD, w.BOOL, w.DWORD], w.HANDLE),
            "QueryFullProcessImageNameW": ([w.HANDLE, w.DWORD, w.LPWSTR, ctypes.POINTER(w.DWORD)], w.BOOL),
            "WaitForSingleObject": ([w.HANDLE, w.DWORD], w.DWORD), "TerminateProcess": ([w.HANDLE, w.UINT], w.BOOL),
            "GetExitCodeProcess": ([w.HANDLE, ctypes.POINTER(w.DWORD)], w.BOOL),
            "CreateFileW": ([w.LPCWSTR, w.DWORD, w.DWORD, ctypes.c_void_p, w.DWORD, w.DWORD, w.HANDLE], w.HANDLE)}
        for name, (arguments, result) in signatures.items():
            function = getattr(self.kernel, name)
            function.argtypes, function.restype = arguments, result

    def owner(self, hwnd):
        owner = self.w.DWORD()
        self.user.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        return owner.value

    def find(self, pid, title="SymoCraft", klass=None):
        found = []
        @self.callback
        def visit(hwnd, unused):
            if self.owner(hwnd) == pid:
                text, classname = ctypes.create_unicode_buffer(256), ctypes.create_unicode_buffer(256)
                self.user.GetWindowTextW(hwnd, text, 256)
                self.user.GetClassNameW(hwnd, classname, 256)
                if text.value == title and (klass is None or classname.value == klass):
                    found.append(hwnd)
            return True
        require(self.user.EnumWindows(visit, 0), "Window enumeration failed.")
        return found[0] if found else None

    def owned(self, hwnd, pid):
        require(hwnd and self.owner(hwnd) == pid, "Window is no longer owned by the launched process.")

    def close(self, hwnd, pid):
        self.owned(hwnd, pid)
        require(self.user.PostMessageW(hwnd, 0x10, 0, 0), "Owned WM_CLOSE failed.")

    @contextmanager
    def dpi(self):
        prior = self.user.SetThreadDpiAwarenessContext(ctypes.c_void_p(-4))
        require(prior, "Cannot set probe DPI context.")
        try:
            yield
        finally:
            require(self.user.SetThreadDpiAwarenessContext(prior), "Cannot restore probe DPI context.")

    def pixels(self, hwnd, pid):
        self.owned(hwnd, pid)
        with self.dpi():
            rectangle = self.rect()
            require(self.user.GetClientRect(hwnd, ctypes.byref(rectangle)), "GetClientRect failed.")
            return [rectangle.right - rectangle.left, rectangle.bottom - rectangle.top]

    def resize(self, hwnd, pid):
        self.owned(hwnd, pid)
        with self.dpi():
            require(self.user.SetWindowPos(hwnd, None, 0, 0, 1280, 720, 0x416), "Owned resize failed.")

    def focus_sink(self):
        hwnd = self.user.CreateWindowExW(0, "STATIC", "SymoCraft owned focus fixture", 0x10CF0000, 40, 40, 280, 140, None, None, None, None)
        require(hwnd, "Cannot create owned focus fixture.")
        return hwnd

    def runner_control(self, hwnd, pid, identifier):
        require(self.find(pid, "SymoCraft Benchmark", "SymoCraftBenchmarkRunner") == hwnd, "Runner identity no longer matches.")
        control = self.user.GetDlgItem(hwnd, identifier)
        self.owned(control, pid)
        return control

    def send(self, hwnd, message, wparam=0, lparam=0):
        result = ctypes.c_size_t()
        require(self.user.SendMessageTimeoutW(hwnd, message, wparam, lparam, 2, 1000, ctypes.byref(result)), "Owned runner message failed.")
        return result.value

    def text(self, hwnd, pid, identifier, text):
        control = self.runner_control(hwnd, pid, identifier)
        buffer = ctypes.create_unicode_buffer(text)
        require(self.send(control, 0xc, 0, ctypes.cast(buffer, ctypes.c_void_p).value), "Owned runner text update failed.")

    def choice(self, hwnd, pid, identifier, index):
        require(self.send(self.runner_control(hwnd, pid, identifier), 0x14e, index) == index, "Owned runner choice failed.")

    def start(self, hwnd, pid):
        self.send(hwnd, 0x111, 110, self.runner_control(hwnd, pid, 110))

    def children(self, pid):
        snapshot = self.kernel.CreateToolhelp32Snapshot(2, 0)
        require(snapshot != ctypes.c_void_p(-1).value, "Process snapshot failed.")
        try:
            entry = self.entry()
            entry.size = ctypes.sizeof(entry)
            require(self.kernel.Process32FirstW(snapshot, ctypes.byref(entry)), "Process enumeration failed.")
            result = []
            while True:
                if entry.parent == pid:
                    result.append(entry.pid)
                if not self.kernel.Process32NextW(snapshot, ctypes.byref(entry)):
                    break
            return result
        finally:
            self.kernel.CloseHandle(snapshot)

    def child(self, parent, executable):
        for pid in self.children(parent):
            handle = self.kernel.OpenProcess(0x100000 | 0x1000 | 1, False, pid)
            if not handle:
                continue
            text, length = ctypes.create_unicode_buffer(32768), self.w.DWORD(32768)
            if self.kernel.QueryFullProcessImageNameW(handle, 0, text, ctypes.byref(length)) and Path(text.value).resolve() == Path(executable).resolve():
                return Child(self, pid, handle)
            self.kernel.CloseHandle(handle)
        return None

    def exclusive_lock_check(self, path):
        handle = self.kernel.CreateFileW(str(path), 0xc0000000, 0, None, 3, 0, None)
        require(handle != ctypes.c_void_p(-1).value, "Session lock remains owned by runner.")
        self.kernel.CloseHandle(handle)


class Child:
    def __init__(self, api, pid, handle):
        self.api, self.pid, self.handle = api, pid, handle

    def wait(self, seconds):
        state = self.api.kernel.WaitForSingleObject(self.handle, int(seconds * 1000))
        require(state in (0, 258), "Owned child wait failed.")
        return state == 0

    def exit_code(self):
        code = self.api.w.DWORD()
        require(self.api.kernel.GetExitCodeProcess(self.handle, ctypes.byref(code)), "Owned child exit query failed.")
        return code.value

    def dispose(self):
        forced = not self.wait(0)
        try:
            if forced:
                require(self.api.kernel.TerminateProcess(self.handle, 1) and self.wait(10), "Owned child cleanup failed.")
        finally:
            self.api.kernel.CloseHandle(self.handle)
        return forced


def wait_ready(process, api, log=None, timeout=40, title="SymoCraft", klass=None):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        hwnd = api.find(process.pid, title, klass)
        if hwnd:
            if not api.user.IsWindowVisible(hwnd):
                api.owned(hwnd, process.pid)
                api.user.ShowWindow(hwnd, 5)
            if log is None or (Path(log).exists() and "[runtime] ready;" in Path(log).read_text(encoding="utf-8-sig", errors="replace")):
                return hwnd
        require(process.poll() is None, "Owned process exited before its ready window.")
        time.sleep(.02)
    raise ValueError("Owned ready window timed out.")
