"""Installed SDL delivery/fault/cancellation diagnostics, not hardware acceptance."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from verification_common import ROOT, capture, clean_environment, cli, evidence_sources, file_path, files, hidden_process, identity, load_json, new_directory, option, read_log, require, safe_child, sha256, stop_process, workspace_path, write_json
from scripts_common import integer
from windows_verification import Windows, wait_ready

CRT = re.compile(r"^(msvcp140.*|vcruntime140.*|concrt140)\.dll$", re.I)
FAULTS = [("invalid-vertex-shader", "assets/shaders/vs_BlockShader.glsl", "#version 460 core\nthis is deliberately invalid GLSL;\n", "Shader compilation failed:"),
          ("invalid-texture", "assets/textures/texture_atlas.png", "This dedicated fixture is not a PNG image.\n", "Cannot decode image: "),
          ("invalid-block-config", "assets/configs/blockFormats.yaml", "", "Failed to load block configuration: Block configuration must be a nonempty mapping")]


def package_identity(package):
    return [identity(p, package) for p in files(package)]


def copy_package(source, destination, without_crt=False):
    destination.mkdir()
    for file in files(source):
        relative = file.relative_to(source).as_posix()
        if without_crt and CRT.fullmatch(relative):
            continue
        target = safe_child(destination, relative)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(file, target)


def finite_valid(record, stdout, stderr, expected, frames, diagnostic=None):
    match = re.search(r"\[runtime\] loop finished; rendered_frames=(\d+)", stdout)
    rendered = int(match[1]) if match else None
    ready = "[runtime] ready;" in stdout
    shutdown = f"[runtime] shutdown complete; exit_code={expected}" in stdout
    gl = all("OpenGL " + key + ":" in stdout for key in ("vendor", "renderer", "version"))
    valid = not record["timed_out"] and record["exit_code"] == expected and shutdown and gl
    if expected == 0:
        valid = valid and ready and rendered == frames and "[runtime] fatal:" not in stderr and "OpenGL diagnostic" not in stderr
    else:
        valid = valid and not ready and "[runtime] fatal:" in stderr and diagnostic in stderr
    return bool(valid), rendered, ready, shutdown, gl


def inspect(inspector, mode, path, base):
    record = capture(inspector, [mode, path], base.parent, base.with_suffix(".json"), Path(str(base) + ".stderr.log"), 10)
    write_json(Path(str(base) + ".process.json"), {"exit_code": record["exit_code"], "forced_termination": record["timed_out"], "mode": mode, "path": str(path)})
    require(not record["timed_out"] and record["exit_code"] == 0, "Structured protocol inspector failed.")
    report = load_json(base.with_suffix(".json"))
    require(report.get("protocol") is True, "Incompatible structured protocol.")
    return report


def runner_cancel(api, game_source, runner_source, output, inspector, timeout):
    directory = new_directory(output / "runner-cancel")
    game_copy, runner_copy = directory / "\u4e2d\u6587 game package", directory / "\u4e2d\u6587 runner package"
    session_parent = directory / "\u4e2d\u6587 result sessions"
    runner, child = None, None
    result = {"case": "runner-cancel", "state": "failed", "passed": False, "runner_executable": str(runner_copy / "SymoCraftBenchmark.exe"),
              "game_executable": str(game_copy / "SymoCraft.exe"), "runner_process_id": None, "game_process_id": None, "runner_hwnd": None, "game_hwnd": None,
              "runner_exit_code": None, "game_exit_code": None, "own_runner_close_sent": False, "timed_out": False, "fixture_forced_termination": False,
              "session_directory": None, "live_status_before_cancel": None, "protocol": None, "error": None,
              "scope": "Existing quick-profile cancellation diagnostic; requires structured live readiness, actual frames and normal cleanup. Not Q06/Q07 acceptance."}
    started = time.monotonic()
    try:
        copy_package(game_source, game_copy)
        copy_package(runner_source, runner_copy)
        session_parent.mkdir()
        with (directory / "runner.stdout.log").open("wb") as stdout, (directory / "runner.stderr.log").open("wb") as stderr:
            try:
                runner = subprocess.Popen([result["runner_executable"]], cwd=Path(os.environ.get("SystemRoot", "C:/Windows")) / "System32", env=clean_environment(), stdout=stdout, stderr=stderr, **hidden_process())
                result["runner_process_id"] = runner.pid
                hwnd = wait_ready(runner, api, timeout=20, title="SymoCraft Benchmark", klass="SymoCraftBenchmarkRunner")
                result["runner_hwnd"] = hwnd
                for identifier, text in ((100, result["game_executable"]), (102, str(session_parent)), (106, "s5-cancellation-diagnostic-not-acceptance")):
                    api.text(hwnd, runner.pid, identifier, text)
                api.choice(hwnd, runner.pid, 104, 1)
                api.choice(hwnd, runner.pid, 105, 0)
                api.start(hwnd, runner.pid)
                deadline = time.monotonic() + min(60, timeout)
                session, ready, live_count = None, False, 0
                while time.monotonic() < deadline:
                    require(runner.poll() is None, "Runner exited before live child.")
                    sessions = [p for p in session_parent.iterdir() if p.is_dir()]
                    require(len(sessions) <= 1, "Unexpected multiple sessions in fresh fixture.")
                    session = sessions[0] if sessions else None
                    if child is None:
                        child = api.child(runner.pid, result["game_executable"])
                    if child and session:
                        result["game_process_id"] = child.pid
                        child_hwnd = api.find(child.pid)
                        result["game_hwnd"] = child_hwnd
                        attempt = session / "static-1-attempt-1"
                        log = attempt / "stdout.log"
                        status = attempt / "capture/status.yaml"
                        if child_hwnd and log.exists() and "[runtime] ready;" in read_log(log) and status.exists():
                            live_count += 1
                            result["live_status_before_cancel"] = inspect(inspector, "--live-status", status, directory / f"live-status-{live_count:03}")
                            ready = result["live_status_before_cancel"].get("live_phase") is True
                        if ready:
                            break
                        require(not child.wait(0), "Child exited before live readiness.")
                    time.sleep(.025)
                require(ready and child and session, "Cancellable runner child/live-status readiness timed out.")
                result["session_directory"] = str(session)
                time.sleep(.25)
                api.runner_control(hwnd, runner.pid, 110)
                api.close(hwnd, runner.pid)
                result["own_runner_close_sent"] = True
                try:
                    runner.wait(timeout=15)
                except subprocess.TimeoutExpired as error:
                    result["timed_out"] = True
                    raise ValueError("Runner did not stop normally within 15 seconds.") from error
                require(child.wait(1), "Owned game child survived runner exit.")
                result.update(runner_exit_code=runner.returncode, game_exit_code=child.exit_code())
                result["protocol"] = inspect(inspector, "--runner", session, directory / "protocol")
                require(result["runner_exit_code"] == 0 and result["game_exit_code"] == 4, "Normal runner/game exit codes must be 0/4.")
                require("[runtime] shutdown complete; exit_code=4" in read_log(session / "static-1-attempt-1/stdout.log"), "Missing normal game cleanup marker.")
                api.exclusive_lock_check(session / "session.lock")
                result.update(passed=True, state="passed")
            finally:
                if child:
                    result["game_exit_code"] = child.exit_code()
                    result["fixture_forced_termination"] |= child.dispose()
                    child = None
                if runner:
                    result["fixture_forced_termination"] |= runner.poll() is None
                    stop_process(runner)
                    result["runner_exit_code"] = runner.returncode
    except (OSError, ValueError) as error:
        result["error"] = str(error)
        result.update(passed=False, state="failed")
    finally:
        if child:
            result["fixture_forced_termination"] |= child.dispose()
        stop_process(runner)
        result["elapsed_seconds"] = time.monotonic() - started
        write_json(directory / "result.json", result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("GamePackage", "RunnerPackage", "OutputDirectory"):
        option(parser, name, required=True)
    option(parser, "ProtocolInspector", default=str(ROOT / "out/m3-t2/s5/inspect-sdl-delivery.exe"))
    option(parser, "TimeoutSeconds", type=integer(30, 180), default=90)
    option(parser, "Frames", type=integer(1, 1000), default=120)
    args = parser.parse_args()
    api = Windows()
    game, runner = Path(args.GamePackage).resolve(strict=True), Path(args.RunnerPackage).resolve(strict=True)
    destination = workspace_path(args.OutputDirectory, ROOT / "out")
    for package in (game, runner):
        require(package.is_dir() and not destination.is_relative_to(package), "Evidence must not be inside a source package.")
    game_identity, runner_identity = package_identity(game), package_identity(runner)
    for path in (game / "SymoCraft.exe", runner / "SymoCraftBenchmark.exe", game / "assets/configs/blockFormats.yaml"):
        file_path(path)
    inspector = file_path(args.ProtocolInspector)
    output = new_directory(destination)
    write_json(output / "input-identity.json", {"game_package": str(game), "runner_package": str(runner), "game_files": game_identity, "runner_files": runner_identity,
               "verifier_sha256": sha256(__file__), "verification_sources": evidence_sources([], [ROOT / "scripts/windows_verification.py"]),
               "protocol_inspector": str(inspector), "protocol_inspector_sha256": sha256(inspector)})
    cases = [("installed-finite-frames", None, None, None, False, False, False), ("different-working-directory", None, None, None, True, False, False),
             ("unicode-space-package", None, None, None, True, True, False)]
    cases += [(name, fault, content, diagnostic, False, False, False) for name, fault, content, diagnostic in FAULTS]
    cases += [("missing-app-local-crt-observation", None, None, None, True, False, True)]
    results = []
    for name, fault, content, diagnostic, system_cwd, unicode, omit_crt in cases:
        directory = new_directory(output / name)
        package = directory / ("\u4e2d\u6587 package with spaces" if unicode else "package")
        result = {"case": name, "state": "failed", "passed": False, "error": None}
        try:
            copy_package(game, package, omit_crt)
            if fault:
                target = file_path(safe_child(package, fault))
                target.write_text(content, encoding="utf-8")
                result.update(fixture_fault_file=str(target), fixture_fault_sha256=sha256(target), source_asset_sha256=sha256(game / fault))
            expected, frames = (3, 1) if fault else (0, args.Frames)
            cwd = Path(os.environ.get("SystemRoot", "C:/Windows")) / "System32" if system_cwd else directory
            prior = api.kernel.SetErrorMode(0x8003)
            try:
                actual = capture(package / "SymoCraft.exe", ["--smoke-frames", frames], cwd, directory / "stdout.log", directory / "stderr.log", args.TimeoutSeconds, clean_environment())
            finally:
                api.kernel.SetErrorMode(prior)
            passed, rendered, ready, shutdown, gl = finite_valid(actual, read_log(directory / "stdout.log"), read_log(directory / "stderr.log"), expected, frames, diagnostic)
            result.update(state="passed" if passed else "failed", passed=passed, executable=str(package / "SymoCraft.exe"), working_directory=str(cwd), process_exit_code=actual["exit_code"],
                          expected_exit_code=expected, requested_frames=frames, rendered_frames=rendered, timed_out=actual["timed_out"], forced_termination=actual["timed_out"], ready=ready, shutdown=shutdown,
                          actual_gl_initialized=gl, expected_diagnostic=diagnostic, elapsed_seconds=actual["elapsed_seconds"], scope="Finite-frame delivery diagnostic; not 15-minute acceptance.")
            if omit_crt and not actual["timed_out"]:
                result.update(state="unavailable", passed=False, clean_machine_missing_crt_verified=False,
                              omitted_app_local_crt=[e["path"] for e in game_identity if CRT.fullmatch(e["path"])],
                              scope="App-local CRT omitted only in dedicated copy; developer/system fallback cannot prove clean-machine behavior. No system files removed.")
        except (ValueError, OSError) as error:
            result["error"] = str(error)
        write_json(directory / "result.json", result)
        results.append(result)
    results.append(runner_cancel(api, game, runner, output, inspector, args.TimeoutSeconds))
    unchanged = package_identity(game) == game_identity and package_identity(runner) == runner_identity
    failed = sum(r["state"] == "failed" for r in results)
    unavailable = sum(r["state"] == "unavailable" for r in results)
    summary = {"game_package": str(game), "runner_package": str(runner), "source_packages_unchanged": unchanged, "total_cases": len(results),
               "passed_cases": sum(r["state"] == "passed" for r in results), "failed_cases": failed, "unavailable_cases": unavailable,
               "verified_cases_passed": failed == 0 and unchanged, "all_requested_checks_passed": failed == 0 and unavailable == 0 and unchanged,
               "hardware_acceptance_passed": False, "q06_acceptance_sample": False, "q07_budget_acceptance": False, "y9000p_checked": False,
               "fixtures_and_first_failures_retained": True, "cases": results}
    write_json(output / "summary.json", summary)
    require(summary["verified_cases_passed"], f"SDL delivery verification failed; see {output}")
    if unavailable:
        print("Missing-CRT clean-machine behavior remains unverified; this is not overall delivery acceptance.")


if __name__ == "__main__":
    cli(main)
