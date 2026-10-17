"""Opt-in ordinary-game comparison; foreground mode preserves every focus failure."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "scripts"))
from scripts_common import read_log, stop_process
from verification_common import clean_environment, identity, load_script, require, utc_now, write_json
from windows_verification import Windows

budget = load_script(Path(__file__).with_name("verify-d3d12-budgets.py"))
SHARED = ("workload", "world_radius", "world_chunks", "world_seed", "digest_schema", "initial_mesh_chunks",
          "terrain_digest", "scene_digest", "initial_digest", "final_digest", "sample_edits", "scheduled_edits", "actual_samples")

def game_command(game, backend, scene, target, warmup, sample, focus_mode):
    command = [game, "--renderer", backend, "--benchmark", scene, "--output", target, "--seed", "424242",
        "--width", "1920", "--height", "1080", "--vsync", "0", "--warmup-seconds", str(warmup),
        "--sample-seconds", str(sample), "--focus-policy", "strict" if focus_mode == "foreground" else "allow-unfocused"]
    if backend != "opengl": command += ["--msaa", "4", "--validation", "1"]
    if focus_mode == "foreground": command += ["--foreground-ready-timeout-seconds", "60"]
    return command

def wait_foreground(process, api, log, timeout=15):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        require(process.poll() is None, "Game exited before foreground confirmation")
        hwnd = api.find(process.pid)
        if hwnd:
            api.owned(hwnd, process.pid)
            if api.user.IsWindowVisible(hwnd) and not api.user.IsIconic(hwnd) and api.user.GetForegroundWindow() == hwnd:
                return {"pid": process.pid, "hwnd": int(hwnd), "confirmed_utc": utc_now(),
                    "foreground": True, "client_pixels": api.pixels(hwnd, process.pid),
                    "confirmed_before_ready_log": "[runtime] ready;" not in read_log(log)}
        time.sleep(.02)
    raise ValueError("Foreground confirmation timed out; desktop activation may be required; no automatic retry")

def foreground_process_options():
    # Hide only a console, not the GUI's initial ShowWindow request.
    return {"creationflags": subprocess.CREATE_NO_WINDOW}

def execute_foreground(command, directory, timeout):
    started = time.monotonic()
    record = {"command": list(map(str, command)), "cwd": str(directory), "started_utc": utc_now(),
        "timed_out": False, "forced_termination": False, "activation_request": "game strict policy enables SDL activate-when-shown",
        "desktop_backup": "authorized Computer Use only; observer does not inject input or repeatedly steal focus",
        "gui_startup_visibility": "no STARTF_USESHOWWINDOW/SW_HIDE; CREATE_NO_WINDOW only"}
    process = None
    error = None
    with (directory / "game.stdout.log").open("wb") as stdout, (directory / "game.stderr.log").open("wb") as stderr:
        try:
            process = subprocess.Popen(record["command"], cwd=directory, env=clean_environment(),
                stdout=stdout, stderr=stderr, **foreground_process_options())
            record["pid"] = process.pid
            write_json(directory / "game.process.json", record)
            try:
                record["focus"] = wait_foreground(process, Windows(), directory / "game.stdout.log", timeout=65)
                require(record["focus"]["client_pixels"] == [1920, 1080], "Foreground client extent differs")
            except Exception as failure:
                error = failure
                record["focus_failure"] = str(failure)
            write_json(directory / "game.process.json", record)
            process.wait(timeout=max(.1, timeout - (time.monotonic() - started)))
        except subprocess.TimeoutExpired as failure:
            error = failure
            record.update(timed_out=True, forced_termination=True)
        finally:
            if process is not None:
                if process.poll() is None: record["forced_termination"] = True
                stop_process(process)
                record["exit_code"] = process.returncode
            record.update(finished_utc=utc_now(), elapsed_seconds=time.monotonic() - started)
            write_json(directory / "game.process.json", record)
    if error is not None: raise error
    require(record["exit_code"] == 0, f"Game exited {record['exit_code']}; original logs retained")
    return read_log(directory / "game.stdout.log")

def validate_focus(capture, summary, mode):
    if mode != "foreground": return
    frames = budget.rows(capture / "frames.csv")
    require(bool(frames) and all(row["focused"] == "1" for row in frames),
        "Foreground round has an unfocused warmup/sample frame; preserve without retry")
    require(summary["focus_class"] == "fully-focused", "Foreground sample is not fully focused")

def collect(args):
    require(args.warmup_seconds >= 0 and 0 < args.sample_seconds <= 600 and args.warmup_seconds <= 600 and
        1 <= args.repeats <= 10, "Invalid collection window/repeats")
    require(len(args.backends) >= 2 and args.backends[0] == "opengl" and len(set(args.backends)) == len(args.backends),
        "Use unique backends with GL first")
    game, native, probe = (Path(value).resolve(strict=True) for value in (args.executable, args.native_consumer, args.report_probe))
    output = Path(args.output_directory).resolve()
    output.mkdir(parents=True, exist_ok=False)
    package = budget.identities(game)
    inputs = [identity(path) for path in (game, native, probe, Path(__file__), Path(budget.__file__))]
    result = {"schema": "symocraft.renderer-comparison.v1", "status": "running", "started_utc": utc_now(),
        "warmup_seconds": args.warmup_seconds, "sample_seconds": args.sample_seconds, "repeats": args.repeats,
        "backends": args.backends, "focus_mode": args.focus_mode, "package_identity": package, "inputs": inputs,
        "machine": budget.windows_hardware(), "rounds": [], "attempts": [], "pairs": [], "collection_valid": False,
        "budgets_passed": False, "formal_acceptance": False, "private_observer_bound": False,
        "automatic_retest": False, "historical_results_superseded": False}
    write_json(output / "result.json", result)
    try:
        for backend in args.backends:
            for scene in budget.SCENES:
                for repeat in range(1, args.repeats + 1):
                    name = f"{backend}-{scene}-{repeat}"
                    directory = output / name
                    directory.mkdir()
                    capture = directory / "capture"
                    command = game_command(game, backend, scene, capture, args.warmup_seconds, args.sample_seconds, args.focus_mode)
                    attempt = {"name": name, "backend": backend, "scene": scene, "repeat": repeat, "status": "running",
                        "command": list(map(str, command)), "directory": str(directory), "started_utc": utc_now()}
                    result["attempts"].append(attempt)
                    result["active_round"] = name
                    write_json(output / "result.json", result)
                    print(f"{name}: {args.warmup_seconds}s warmup + {args.sample_seconds}s sample; {args.focus_mode}", flush=True)
                    if args.focus_mode == "foreground":
                        stdout = execute_foreground(command, directory, args.warmup_seconds + args.sample_seconds + 125)
                    else:
                        stdout = budget.execute(command, directory, "game", args.warmup_seconds + args.sample_seconds + 60)
                    require("[runtime] shutdown complete; exit_code=0" in stdout and "[performance] exported; valid_run=1" in stdout,
                        "Incomplete/invalid game export; preserve all frames")
                    if args.focus_mode == "foreground":
                        require("[benchmark] foreground-ready;" in stdout, "Foreground preparation did not complete")
                    stderr = read_log(directory / "game.stderr.log").strip()
                    if backend == "vulkan":
                        require(stderr == "[renderer] Vulkan mesh memory: policy=prefer-device-local-host-visible; type=4; flags=0x7; heap=0; host-visible-fallback=false\n"
                            "[renderer] Vulkan shutdown: validation_warnings=0; validation_errors=0; native_retired=true",
                            "Unexpected Vulkan diagnostics/memory policy; preserve and stop")
                    else: require(not stderr, "Unexpected GL/D3D12 diagnostics; preserve and stop")
                    budget.execute([native, capture, backend], directory, "native-consumer", 20)
                    report = json.loads(budget.execute([probe, capture, {"opengl":"OpenGL","d3d12":"D3D12","vulkan":"Vulkan"}[backend], scene],
                        directory, "game-report", 20))
                    summary = budget.analyze(capture, args.warmup_seconds, args.sample_seconds)
                    validate_focus(capture, summary, args.focus_mode)
                    require(summary["frames"] == report["measured_frames"], "Native frame count differs")
                    measured = [row for row in budget.rows(capture / "frames.csv") if
                        args.warmup_seconds <= float(row["elapsed_s"]) < args.warmup_seconds + args.sample_seconds]
                    item = {"backend": backend, "scene": scene, "repeat": repeat, "capture": str(capture), "summary": summary,
                        "native_report": report, "over_20ms_frames": sum(float(row["frame_ms"]) > 20 for row in measured),
                        "over_50ms_frames": sum(float(row["frame_ms"]) > 50 for row in measured),
                        "artifacts": [identity(path) for path in sorted(capture.iterdir()) if path.is_file()]}
                    result["rounds"].append(item)
                    attempt.update(status="passed", finished_utc=utc_now())
                    require(budget.identities(game) == package, "Game/package changed during comparison")
                    write_json(output / "result.json", result)
                    print(name + " complete: " + json.dumps(summary), flush=True)
        for backend in args.backends[1:]:
            for scene in budget.SCENES:
                for repeat in range(1, args.repeats + 1):
                    gl, modern = [next(row for row in result["rounds"] if row["backend"] == selected and row["scene"] == scene and row["repeat"] == repeat)
                        for selected in ("opengl", backend)]
                    match = all(gl["native_report"][key] == modern["native_report"][key] for key in SHARED)
                    result["pairs"].append({"modern_backend": backend, "scene": scene, "repeat": repeat,
                        "workload_matches": match, **budget.compare(gl["summary"], modern["summary"])})
                    require(match, "Paired workload differs; do not pool or choose another GL round")
        require(all(budget.sha(Path(row["path"])) == row["sha256"] for row in inputs), "Collector/validator/game identity changed")
        result.update(status="passed", active_round=None, collection_valid=True,
            budgets_passed=all(row["budget_passed_under_verified_conditions"] for row in result["pairs"]))
    except BaseException as error:
        result.update(status="failed", failure=repr(error))
        if result["attempts"] and result["attempts"][-1]["status"] == "running":
            result["attempts"][-1].update(status="failed", failure=repr(error), finished_utc=utc_now())
        raise
    finally:
        result["finished_utc"] = utc_now()
        write_json(output / "result.json", result)
    print(f"{len(result['rounds'])} rounds valid; numeric/focus budgets passed={result['budgets_passed']}", flush=True)
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("executable", "native-consumer", "report-probe", "output-directory"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--backends", nargs="+", choices=("opengl", "d3d12", "vulkan"), default=["opengl", "d3d12"])
    parser.add_argument("--warmup-seconds", type=int, default=10)
    parser.add_argument("--sample-seconds", type=int, default=50)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--focus-mode", choices=("allow-unfocused", "foreground"), default="allow-unfocused")
    collect(parser.parse_args())
    return 0

if __name__ == "__main__": sys.exit(main())
