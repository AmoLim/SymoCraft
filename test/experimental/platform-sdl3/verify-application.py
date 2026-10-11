"""Owned Win32 lifecycle/focus operations against the actual installed game."""
import argparse
from pathlib import Path
import subprocess
import sys
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, capture, clean_environment, cli, file_path, hidden_process, identity, load_json, new_directory, option, read_log, require, sha256, stop_process, workspace_path, write_json
from windows_verification import Windows, wait_ready


def application_valid(name, row, stdout, stderr):
    if row["timed_out"] or row["forced_termination"] or row["error"] or "[runtime] shutdown complete;" not in stdout or "[runtime] fatal:" in stderr or "OpenGL diagnostic" in stderr:
        return False
    r = row["protocol"] or {}
    if r.get("runtime_error") or not row["actual_owned_action"]:
        return False
    if name in {"paused-close", "minimized-close"}:
        return row["exit_code"] == 0
    if name == "allow-unfocused":
        return row["exit_code"] == 0 and r.get("valid_run") is True and r.get("completed") is True and r.get("unfocused_frames", 0) > 0
    if row["exit_code"] != 4 or r.get("valid_run") is not False:
        return False
    if name == "strict-unfocused":
        return r.get("completed") is True and r.get("window_not_focused") is True and r.get("unfocused_frames", 0) > 0
    if r.get("completed") is not False:
        return False
    return r.get("framebuffer_changed_or_minimized" if name in {"benchmark-minimized", "benchmark-resized"} else "duration_not_completed") is True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("Executable", "ProtocolInspector", "OutputDirectory"):
        option(parser, name, required=True)
    args = parser.parse_args()
    api = Windows()
    exe, inspector = file_path(args.Executable), file_path(args.ProtocolInspector)
    inputs = [identity(exe), identity(inspector)]
    output = new_directory(workspace_path(args.OutputDirectory, ROOT / "out"))
    results = []
    for name in ("paused-close", "minimized-close", "allow-unfocused", "strict-unfocused", "benchmark-minimized", "benchmark-resized", "benchmark-close"):
        directory = new_directory(output / name)
        benchmark = name not in {"paused-close", "minimized-close"}
        arguments = ["--seed", "424242"] + (["--benchmark", "static", "--output", str(directory / "run"), "--warmup-seconds", "0", "--sample-seconds", "3", "--width", "1920", "--height", "1080", "--vsync", "0", "--focus-policy", "strict" if name == "strict-unfocused" else "allow-unfocused"] if benchmark else [])
        process, sink = None, None
        row = {"case": name, "passed": False, "partial": False, "actual_owned_action": False, "pixels_before": None, "pixels_after": None, "exit_code": None,
               "timed_out": False, "forced_termination": False, "error": None, "protocol": None, "arguments": arguments}
        try:
            with (directory / "stdout.log").open("wb") as stdout, (directory / "stderr.log").open("wb") as stderr:
                try:
                    process = subprocess.Popen([str(exe), *arguments], cwd=directory, env=clean_environment(), stdout=stdout, stderr=stderr, **hidden_process())
                    hwnd = wait_ready(process, api, directory / "stdout.log")
                    row["pixels_before"] = api.pixels(hwnd, process.pid)
                    if name in {"paused-close", "allow-unfocused", "strict-unfocused"}:
                        sink = api.focus_sink()
                        api.user.SetForegroundWindow(sink)
                        time.sleep(.15)
                        row["actual_owned_action"] = api.user.GetForegroundWindow() == sink
                        row["partial"] = not row["actual_owned_action"]
                    if name in {"minimized-close", "benchmark-minimized"}:
                        api.owned(hwnd, process.pid)
                        api.user.ShowWindow(hwnd, 6)
                        time.sleep(.15)
                        row["actual_owned_action"] = bool(api.user.IsIconic(hwnd))
                        require(row["actual_owned_action"], "Owned window did not minimize.")
                    if name == "benchmark-resized":
                        api.resize(hwnd, process.pid)
                        row["actual_owned_action"] = True
                        row["pixels_after"] = api.pixels(hwnd, process.pid)
                        row["partial"] = row["pixels_after"] == row["pixels_before"]
                        require(not row["partial"], "Fixed window rejected forced resize; production invalidation not exercised.")
                    if name in {"paused-close", "minimized-close", "benchmark-close"}:
                        api.close(hwnd, process.pid)
                        if name == "benchmark-close":
                            row["actual_owned_action"] = True
                    try:
                        process.wait(timeout=60)
                    except subprocess.TimeoutExpired:
                        row["timed_out"] = True
                finally:
                    if process:
                        row["forced_termination"] = process.poll() is None
                        stop_process(process)
                        row["exit_code"] = process.returncode
            if benchmark:
                actual = capture(inspector, [directory / "run"], directory, directory / "protocol.json", directory / "protocol.stderr.log", 10)
                row["protocol"] = load_json(directory / "protocol.json")
                require(not actual["timed_out"] and actual["exit_code"] == 0 and row["protocol"].get("protocol") is True, "Structured protocol verification failed.")
        except (ValueError, OSError) as error:
            row["error"] = str(error)
        finally:
            stop_process(process)
            if sink:
                api.user.DestroyWindow(sink)
        row["passed"] = application_valid(name, row, read_log(directory / "stdout.log"), read_log(directory / "stderr.log"))
        results.append(row)
    unchanged = all(sha256(x["path"]) == x["sha256"] for x in inputs)
    summary = {"scope": "S3 owned Win32 operations; restricted forced resize; three-second protocol diagnostics, not Q06/Q07/physical acceptance",
               "executable": str(exe), "executable_sha256": inputs[0]["sha256"], "protocol_inspector": str(inspector), "protocol_inspector_sha256": inputs[1]["sha256"],
               "input_identity_unchanged": unchanged, "passed": unchanged and all(r["passed"] for r in results), "cases": results}
    write_json(output / "summary.json", summary)
    require(summary["passed"], f"Application checks incomplete or failed; see {output}")


if __name__ == "__main__":
    cli(main)
