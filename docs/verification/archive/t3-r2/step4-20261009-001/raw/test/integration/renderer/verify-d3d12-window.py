"""Bounded, PID-scoped lifecycle verification of the real complete D3D12 game."""

import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
from datetime import datetime, timezone
import uuid


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def identity(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return {"path": str(path), "bytes": path.stat().st_size, "sha256": digest.hexdigest()}


def checked_path(value, allowed):
    original = Path(os.path.abspath(value))
    resolved = original.resolve()
    try:
        relative = resolved.relative_to(allowed)
    except ValueError as error:
        raise ValueError("Verification paths must be inside repository out/m3-t3: " + str(resolved)) from error
    if not relative.parts:
        raise ValueError("The output root itself is not a candidate or run directory.")
    for part in (original,) + tuple(original.parents):
        if part.exists() and (part.is_symlink() or getattr(part.lstat(), "st_file_attributes", 0) & 0x400):
            raise ValueError("Linked/reparse paths are not supported: " + str(part))
    return resolved


def package_files(package):
    pending = [package]
    files = []
    while pending:
        for item in pending.pop().iterdir():
            if item.is_symlink() or getattr(item.lstat(), "st_file_attributes", 0) & 0x400:
                raise ValueError("Linked package entry: " + str(item))
            if item.is_dir():
                pending.append(item)
            elif item.is_file():
                files.append(item)
            else:
                raise ValueError("Unexpected package entry: " + str(item))
    return sorted(files)


class NativeWindow:
    def __init__(self):
        from ctypes import wintypes

        self.user32 = ctypes.WinDLL("user32", use_last_error=True)
        self.callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

        class Rect(ctypes.Structure):
            _fields_ = [(name, wintypes.LONG) for name in ("left", "top", "right", "bottom")]

        self.rect_type = Rect
        signatures = {
            "EnumWindows": ([self.callback_type, wintypes.LPARAM], wintypes.BOOL),
            "GetWindowTextW": ([wintypes.HWND, wintypes.LPWSTR, ctypes.c_int], ctypes.c_int),
            "GetWindowThreadProcessId": ([wintypes.HWND, ctypes.POINTER(wintypes.DWORD)], wintypes.DWORD),
            "IsWindow": ([wintypes.HWND], wintypes.BOOL),
            "IsWindowVisible": ([wintypes.HWND], wintypes.BOOL),
            "IsIconic": ([wintypes.HWND], wintypes.BOOL),
            "GetForegroundWindow": ([], wintypes.HWND),
            "SetForegroundWindow": ([wintypes.HWND], wintypes.BOOL),
            "ShowWindowAsync": ([wintypes.HWND, ctypes.c_int], wintypes.BOOL),
            "GetClientRect": ([wintypes.HWND, ctypes.POINTER(Rect)], wintypes.BOOL),
            "GetWindowRect": ([wintypes.HWND, ctypes.POINTER(Rect)], wintypes.BOOL),
            "SetWindowPos": ([wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int,
                              ctypes.c_int, ctypes.c_int, wintypes.UINT], wintypes.BOOL),
            "PostMessageW": ([wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM], wintypes.BOOL),
        }
        for name, (arguments, result) in signatures.items():
            function = getattr(self.user32, name)
            function.argtypes = arguments
            function.restype = result
        self.dword = wintypes.DWORD

    def owner(self, hwnd):
        owner = self.dword()
        self.user32.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        return owner.value

    def find(self, process_id):
        handles = []

        @self.callback_type
        def visit(hwnd, unused):
            if self.owner(hwnd) == process_id:
                title = ctypes.create_unicode_buffer(256)
                self.user32.GetWindowTextW(hwnd, title, len(title))
                if title.value == "SymoCraft":
                    handles.append(hwnd)
            return True

        self.require(self.user32.EnumWindows(visit, 0), "EnumWindows")
        return handles

    @staticmethod
    def require(result, operation):
        if not result:
            raise OSError(ctypes.get_last_error(), operation + " failed")

    def rectangle(self, hwnd, client=False):
        rectangle = self.rect_type()
        operation = "GetClientRect" if client else "GetWindowRect"
        self.require(getattr(self.user32, operation)(hwnd, ctypes.byref(rectangle)), operation)
        return rectangle.right - rectangle.left, rectangle.bottom - rectangle.top


def verify(options):
    root = Path(__file__).resolve().parents[3]
    allowed = (root / "out" / "m3-t3").resolve()
    package = checked_path(options.package_directory, allowed)
    output = checked_path(options.output_directory, allowed)
    try:
        output.relative_to(package)
    except ValueError:
        pass
    else:
        raise ValueError("Output must be outside the candidate package to keep its contents pristine.")
    if output.exists():
        raise ValueError("Output directory must be new; prior evidence is never overwritten.")
    exe = package / "SymoCraft.exe"
    required = [exe, package / "assets/configs/blockFormats.yaml", package / "assets/textures/texture_atlas.png"]
    required += [package / "assets/shaders/d3d12" / name for name in
                 ("blocks.vs.dxil", "blocks.ps.dxil", "selection.vs.dxil", "selection.ps.dxil")]
    for item in required:
        if not item.is_file():
            raise ValueError("Missing candidate input: " + str(item))
    inputs = [identity(item) for item in package_files(package)]
    output.mkdir(parents=True)
    workdir = output / "unrelated-cwd"
    workdir.mkdir()
    stdout = output / "game.stdout.log"
    stderr = output / "game.stderr.log"
    arguments = ["--renderer", "d3d12", "--msaa", "4", "--validation", "1", "--seed", "424242",
                 "--scene", "regression", "--checkpoint", "four-chunk"]
    manifest = {
        "schema": "symocraft.d3d12-production-window-verification.v1", "run_id": str(uuid.uuid4()),
        "started_utc": utc_now(), "finished_utc": None, "success": False, "failure": None,
        "package_directory": str(package), "executable": str(exe), "working_directory": str(workdir),
        "arguments": arguments, "package_identity": inputs, "runner_identity": identity(Path(__file__).resolve()),
        "process_id": None, "window_handle": None, "actual_renderer": None, "requested_close": "Window",
        "requested_resize": options.resize, "requested_outer_size": [options.resize_width, options.resize_height],
        "ready_observed": False, "exit_code": None,
        "forced_termination": False, "continuously_minimized": False, "minimize_duration_ms": None,
        "world_summary_observed": False, "shutdown_observed": False, "rendered_frames": None, "actions": [],
        "production_pause_resume": None, "no_render_while_suspended_observed": False,
        "render_count_advanced_after_resume": False,
        "manual_gameplay_acceptance": False, "fifteen_minute_session": False, "r3_acceptance": False,
        "escape_input_exercised": False, "actual_device_lost_exercised": False,
        "actual_in_flight_timeout_exercised": False,
        "limits_seconds": {"startup": options.startup_timeout_seconds, "action": options.action_timeout_seconds,
                           "normal_close": options.exit_timeout_seconds, "minimized_hold": 12},
        "limitations": [
            "Real complete-game window lifecycle verification, not the 14-item manual gameplay or 15-minute acceptance.",
            "OS client/window rectangles are observations, not Renderer GetInfo framebuffer extent or a screenshot.",
            "Production pause/resume render-count logs prove the ordinary suspended loop issued no Render calls; no native fence/Present counter or resumed-pixel capture is included.",
            "WM_CLOSE targets only the PID-verified HWND; Escape remains manual real-keyboard input. No global keyboard injection is used.",
            "Actual validated renderer identity is required; unavailable Debug Layer fails instead of silently disabling validation.",
        ],
    }
    process = None
    hwnd = None

    def save():
        (output / "verification.json").write_text(json.dumps(manifest, ensure_ascii=True, indent=2) + "\n", encoding="utf-8")

    def read_stdout():
        return stdout.read_text(encoding="utf-8", errors="replace") if stdout.exists() else ""

    def assert_target():
        if process.poll() is not None:
            raise RuntimeError("Game exited before the requested window operation.")
        if not native.user32.IsWindow(hwnd) or native.owner(hwnd) != process.pid:
            raise RuntimeError("Window no longer belongs to the launched PID; no operation was sent.")

    def snapshot(phase):
        assert_target()
        client_width, client_height = native.rectangle(hwnd, client=True)
        outer_width, outer_height = native.rectangle(hwnd)
        row = {"phase": phase, "utc": utc_now(), "process_id": process.pid, "hwnd": hex(hwnd),
               "iconic": bool(native.user32.IsIconic(hwnd)), "visible": bool(native.user32.IsWindowVisible(hwnd)),
               "foreground": native.user32.GetForegroundWindow() == hwnd,
               "client_width": client_width, "client_height": client_height,
               "outer_width": outer_width, "outer_height": outer_height}
        manifest["actions"].append(row)
        save()
        return row

    def wait_condition(condition, description):
        deadline = time.monotonic() + options.action_timeout_seconds
        while time.monotonic() < deadline:
            assert_target()
            if condition():
                return
            time.sleep(0.1)
        raise RuntimeError("Bounded target-window wait failed: " + description)

    try:
        save()
        native = NativeWindow()
        with stdout.open("wb") as out, stderr.open("wb") as err:
            process = subprocess.Popen([str(exe)] + arguments, cwd=workdir, stdout=out, stderr=err,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
        manifest["process_id"] = process.pid
        deadline = time.monotonic() + options.startup_timeout_seconds
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError("Game exited during startup; retained logs contain the original failure.")
            if "[runtime] ready; chunks=441;" in read_stdout():
                manifest["ready_observed"] = True
                break
            time.sleep(0.2)
        if not manifest["ready_observed"]:
            raise RuntimeError("Complete 441-chunk game did not report ready within the startup limit.")
        log = read_stdout()
        pattern = (r"^\[renderer\] backend=(?P<backend>D3D12); device=(?P<device>.*?); identity=(?P<identity>.*?); "
                   r"driver=(?P<driver>.*?); samples=(?P<samples>[0-9]+); present=(?P<present>[^;]+); validation=(?P<validation>[01])\r?$")
        match = re.search(pattern, log, re.MULTILINE)
        if not match:
            raise RuntimeError("Missing actual production D3D12 renderer identity.")
        info = {"backend": match["backend"], "device_name": match["device"], "device_identity": match["identity"],
                "driver_version": match["driver"], "actual_samples": int(match["samples"]),
                "present_mode": match["present"], "validation_enabled": match["validation"] == "1"}
        manifest["actual_renderer"] = info
        if (info["device_name"] != options.expected_device_name or not info["device_identity"].strip() or
                not info["driver_version"].strip() or info["actual_samples"] != 4 or
                info["present_mode"] != "Fifo" or not info["validation_enabled"]):
            raise RuntimeError("Actual renderer differs from required RTX, 4x, Fifo, validated production configuration.")
        manifest["world_summary_observed"] = "[world] summary-begin" in log and "[world] summary-end" in log
        if not manifest["world_summary_observed"]:
            raise RuntimeError("Missing production fixed-world summary.")
        handles = native.find(process.pid)
        if len(handles) != 1:
            raise RuntimeError("Expected exactly one game HWND for launched PID; found " + str(len(handles)))
        hwnd = handles[0]
        manifest["window_handle"] = hex(hwnd)
        assert_target()
        # Reveal only our verified HWND; do not suppress SDL's normal initial activation.
        native.require(native.user32.ShowWindowAsync(hwnd, 9), "initial SW_RESTORE")
        wait_condition(lambda: native.user32.IsWindowVisible(hwnd) and not native.user32.IsIconic(hwnd),
                       "target visible before minimization")
        assert_target()
        native.user32.SetForegroundWindow(hwnd)
        wait_condition(lambda: native.user32.GetForegroundWindow() == hwnd, "target foreground before minimization")
        time.sleep(1)
        snapshot("ready")
        assert_target()
        native.require(native.user32.ShowWindowAsync(hwnd, 6), "SW_MINIMIZE")
        wait_condition(lambda: native.user32.IsIconic(hwnd), "real minimization")
        snapshot("minimized-start")
        hold_start = time.monotonic()
        while True:
            assert_target()
            if not native.user32.IsIconic(hwnd):
                raise RuntimeError("Target restored before the continuous 12-second minimization completed.")
            snapshot("minimized-hold")
            if time.monotonic() - hold_start >= 12:
                break
            time.sleep(0.5)
        manifest["minimize_duration_ms"] = (time.monotonic() - hold_start) * 1000
        end_hold = snapshot("minimized-end")
        manifest["continuously_minimized"] = end_hold["iconic"] and manifest["minimize_duration_ms"] >= 12000
        assert_target()
        native.require(native.user32.ShowWindowAsync(hwnd, 9), "SW_RESTORE")
        wait_condition(lambda: not native.user32.IsIconic(hwnd), "real restoration")
        assert_target()
        native.user32.SetForegroundWindow(hwnd)
        wait_condition(lambda: native.user32.GetForegroundWindow() == hwnd, "target foreground after restore")
        restored = snapshot("restored-foreground")
        if restored["client_width"] <= 0 or restored["client_height"] <= 0:
            raise RuntimeError("Restored native client rectangle is nonpositive.")
        time.sleep(2)
        if options.resize:
            assert_target()
            # Outer-window units, not renderer framebuffer pixels; no other window is moved.
            native.require(native.user32.SetWindowPos(hwnd, None, 0, 0, options.resize_width,
                                                      options.resize_height, 0x4016), "SetWindowPos")
            wait_condition(lambda: native.rectangle(hwnd) == (options.resize_width, options.resize_height),
                           "requested outer-window resize")
            snapshot("resized")
            time.sleep(2)
        snapshot("before-close")
        assert_target()
        native.require(native.user32.PostMessageW(hwnd, 0x0010, 0, 0), "PID-verified WM_CLOSE")
        try:
            process.wait(timeout=options.exit_timeout_seconds)
        except subprocess.TimeoutExpired as error:
            raise RuntimeError("Game did not shut down within the normal-close limit.") from error
        manifest["exit_code"] = process.returncode
        log = read_stdout()
        manifest["shutdown_observed"] = "[runtime] shutdown complete; exit_code=0" in log
        frames = re.search(r"\[runtime\] loop finished; rendered_frames=([0-9]+);", log)
        if frames:
            manifest["rendered_frames"] = int(frames[1])
        if process.returncode != 0 or not manifest["shutdown_observed"] or not frames or int(frames[1]) == 0:
            raise RuntimeError("Normal production exit or actual rendered-frame record is missing.")
        suspended = re.search(r"^\[runtime\] suspended; minimized=1; extent=(-?[0-9]+),(-?[0-9]+); "
                              r"rendered_frames=([0-9]+)\r?$", log, re.MULTILINE)
        resumed = re.search(r"^\[runtime\] resumed; extent=(-?[0-9]+),(-?[0-9]+); rendered_frames=([0-9]+)\r?$",
                            log[suspended.end():] if suspended else "", re.MULTILINE)
        if not suspended or not resumed:
            raise RuntimeError("Missing production minimized suspension and subsequent resume logs; OS-only observations are insufficient.")
        manifest["production_pause_resume"] = {
            "suspended_extent": [int(suspended[1]), int(suspended[2])], "suspended_rendered_frames": int(suspended[3]),
            "resumed_extent": [int(resumed[1]), int(resumed[2])], "resumed_rendered_frames": int(resumed[3]),
            "suspended_log": suspended[0], "resumed_log": resumed[0],
        }
        manifest["no_render_while_suspended_observed"] = int(suspended[3]) == int(resumed[3])
        manifest["render_count_advanced_after_resume"] = int(frames[1]) > int(resumed[3])
        if (not manifest["no_render_while_suspended_observed"] or not manifest["render_count_advanced_after_resume"] or
                int(resumed[1]) <= 0 or int(resumed[2]) <= 0):
            raise RuntimeError("Production render count changed during suspension or did not resume with positive extent.")
        if stderr.read_text(encoding="utf-8", errors="replace").strip():
            raise RuntimeError("Production stderr is nonempty; inspect retained diagnostics before acceptance.")
        if [identity(item) for item in package_files(package)] != inputs:
            raise RuntimeError("Candidate file set or bytes changed during window verification.")
        if identity(Path(__file__).resolve()) != manifest["runner_identity"]:
            raise RuntimeError("Window runner changed during verification.")
        manifest["success"] = True
    except Exception as error:
        manifest["failure"] = str(error)
    finally:
        if process is not None:
            try:
                if process.poll() is None:
                    manifest["forced_termination"] = True
                    manifest["success"] = False
                    process.kill()
                    process.wait(timeout=10)
                manifest["exit_code"] = process.returncode
            except Exception as error:
                manifest["success"] = False
                manifest["cleanup_failure"] = str(error)
        manifest["artifacts"] = [identity(item) for item in sorted(output.rglob("*"))
                                 if item.is_file() and item.name != "verification.json"]
        manifest["finished_utc"] = utc_now()
        save()
    if not manifest["success"]:
        print("Production window verification failed. Retained evidence: " + str(output), file=sys.stderr)
        return 1
    print("Production window verification passed; PID={}; minimized_ms={:.3f}; exit={}".format(
        process.pid, manifest["minimize_duration_ms"], manifest["exit_code"]))
    return 0


def bounded_integer(minimum, maximum):
    def parse(value):
        result = int(value)
        if result < minimum or result > maximum:
            raise argparse.ArgumentTypeError("Expected {}..{}".format(minimum, maximum))
        return result
    return parse


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package-directory", required=True)
    parser.add_argument("--output-directory", required=True)
    parser.add_argument("--close-mode", choices=("Window",), default="Window")
    parser.add_argument("--resize", action="store_true")
    parser.add_argument("--resize-width", type=bounded_integer(640, 3840), default=1280)
    parser.add_argument("--resize-height", type=bounded_integer(480, 2160), default=720)
    parser.add_argument("--expected-device-name", default="NVIDIA GeForce RTX 5070 Ti")
    parser.add_argument("--startup-timeout-seconds", type=bounded_integer(30, 300), default=180)
    parser.add_argument("--action-timeout-seconds", type=bounded_integer(5, 60), default=15)
    parser.add_argument("--exit-timeout-seconds", type=bounded_integer(5, 120), default=30)
    options = parser.parse_args()
    if os.name != "nt":
        print("This probe requires Windows and the production D3D12 candidate; no fallback is available.", file=sys.stderr)
        return 2
    try:
        return verify(options)
    except Exception as error:
        print("Window verification setup failed: " + str(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
