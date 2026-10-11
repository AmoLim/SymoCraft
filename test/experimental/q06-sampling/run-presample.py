"""Nine exploratory native GLFW rounds; not Q06/Q07/A11 acceptance."""
import argparse
import csv
import json
import math
import os
import platform
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, assert_hash, capture, clean_environment, cli, file_path, files, identity, load_json, new_directory, option, read_log, reject_links, require, sha256, utc_now, write_json
from scripts_common import integer


def decimal_value(text):
    try:
        value = float(text)
    except (TypeError, ValueError) as error:
        raise ValueError(f"Invalid numeric CSV data: {text}") from error
    require(math.isfinite(value) and value >= 0, f"Invalid numeric CSV data: {text}")
    return value


def memory_statistics(path):
    with Path(path).open(encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.DictReader(stream))
    columns = ("elapsed_s", "working_set_bytes", "private_bytes")
    require(rows and all(k in rows[0] for k in columns), "Missing memory rows/columns.")
    selected, problems, missing, previous = [], [], dict.fromkeys(columns, 0), -1
    for row in rows:
        try:
            elapsed = decimal_value(row["elapsed_s"])
        except ValueError as error:
            missing["elapsed_s"] += 1
            problems.append(str(error))
            continue
        if elapsed <= previous:
            problems.append("Memory timestamps are not strictly ordered.")
        previous = elapsed
        if 10 <= elapsed < 40:
            point = {"elapsed_s": elapsed}
            for column in columns[1:]:
                point[column] = None
                try:
                    value = decimal_value(row[column])
                    require(value > 0 and value == int(value), "Invalid process memory bytes: " + column)
                    point[column] = value
                except ValueError as error:
                    missing[column] += 1
                    problems.append(str(error))
            selected.append(point)
    first = selected[0]["elapsed_s"] if selected else None
    last = selected[-1]["elapsed_s"] if selected else None
    gap = max((b["elapsed_s"] - a["elapsed_s"] for a, b in zip(selected, selected[1:])), default=0) if selected else None
    if len(selected) < 24:
        problems.append("Measured memory coverage has fewer than 24 observations.")
    if first is None or first > 12 or last < 38:
        problems.append("Memory does not cover both ends of the 10-to-40-second window.")
    if gap is not None and gap > 2.5:
        problems.append("Memory observation gap exceeds 2.5 seconds.")
    result = {"window": "elapsed_s >= 10 and < 40", "requested_window_seconds": 30, "samples": len(selected),
              "first_elapsed_s": first, "last_elapsed_s": last, "observed_span_seconds": last - first if selected else None,
              "max_observed_gap_seconds": gap, "invalid_or_missing_cells": missing, "nominal_1s_window_rows": 30,
              "nominal_row_shortfall": max(0, 30 - len(selected)), "passed": not problems, "problems": problems,
              "coverage_requirements": "at least 24 points, first <=12s, last >=38s, max gap <=2.5s; data validity only",
              "method": "nearest-rank ceil(p*N)-1 over periodic observations, not continuous peak detection", "scope": "process memory; private bytes are not VRAM"}
    for column in columns[1:]:
        values = sorted(p[column] for p in selected) if selected and not missing[column] else []
        result[column] = {"p50_bytes": values[math.ceil(len(values) * .5) - 1], "p95_bytes": values[math.ceil(len(values) * .95) - 1],
                          "max_observed_bytes": values[-1], "first_bytes": selected[0][column], "last_bytes": selected[-1][column]} if values else None
    return result


def collected(exe, arguments, cwd, stdout, stderr, limit):
    env = clean_environment()
    env["SYMOCRAFT_PUBLISH_DIAGNOSTICS"] = "0"
    result = {"executable": str(exe), "arguments": list(map(str, arguments)), "working_directory": str(cwd), "started_utc": utc_now(),
              "launched": False, "exit_code": None, "timeout": False, "collector_error": None, "stdout": str(stdout), "stderr": str(stderr),
              "child_api_diagnostics": "0", "parent_environment_mutated": False}
    try:
        actual = capture(exe, arguments, cwd, stdout, stderr, limit, env)
        result.update(actual, launched=True, timeout=actual["timed_out"], elapsed_ms=actual["elapsed_seconds"] * 1000)
    except OSError as error:
        result["collector_error"] = str(error)
    result["ended_utc"] = utc_now()
    result.update({name + "_sha256": sha256(path) if path.exists() else None for name, path in (("stdout", stdout), ("stderr", stderr))})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("GameExe", "ValidatorExe", "BenchmarkCoreLibrary", "YamlLibrary", "Output"):
        option(parser, name, required=True)
    option(parser, "AssetsRoot")
    option(parser, "DeclaredPowerMode", default="unknown")
    option(parser, "DeclaredClockSettings", default="unknown")
    option(parser, "TimeoutSeconds", type=integer(60, 600), default=180)
    args = parser.parse_args()
    values = [args.GameExe, args.ValidatorExe, args.BenchmarkCoreLibrary, args.YamlLibrary]
    require(all(Path(x).is_absolute() for x in values), "Use absolute paths for native inputs.")
    game, validator, core, yaml = map(file_path, values)
    assets = Path(args.AssetsRoot or game.parent / "assets").resolve(strict=True)
    require(assets == (game.parent / "assets").resolve(), "Identify only actual game-directory assets.")
    reject_links(args.Output)
    resources = [identity(p, assets) for p in files(assets)]
    require(resources, "No game resources.")
    identities = [identity(p) for p in (game, validator, core, yaml, Path(__file__), Path(__file__).with_name("verify-capture.cpp"), Path(__file__).with_name("CMakeLists.txt"), ROOT / "scripts/verification_common.py", ROOT / "scripts/scripts_common.py")]
    imports_path = validator.parent / "production-imports.json"
    imports = load_json(imports_path)
    require(imports["configuration"] == "Release" and imports["benchmark_core"]["sha256"] == sha256(core) and imports["yaml"]["sha256"] == sha256(yaml), "Validator imports do not match actual Release libraries.")
    output = new_directory(args.Output)
    plan = [{"scene": scene, "repeat": repeat, "warmup_seconds": 10, "sample_seconds": 30} for scene in ("static", "walk", "edit") for repeat in range(1, 4)]
    from benchmark import windows_hardware
    hardware = windows_hardware() if os.name == "nt" else {}
    def fact(value, query):
        observed = isinstance(value, (dict, list))
        return {"status": "observed" if observed else "unavailable", "query": query, "utc": utc_now(),
                "records": value if isinstance(value, list) else [value] if observed else [], "error": None if observed else str(value or "not collected")}
    machine = {"computer": fact(hardware.get("memory"), "Win32 GlobalMemoryStatusEx; manufacturer/model not collected"),
               "os": fact({"Caption": platform.system(), "Version": platform.version(), "OSArchitecture": platform.machine()}, "Python platform"),
               "cpu": fact(hardware.get("cpu"), "Win32 topology/processor registry; clock fields not collected"),
               "gpu": fact(hardware.get("displayControllers"), "Windows display registry; PNPDeviceID/DriverDate/AdapterRAM not collected"),
               "battery": fact(hardware.get("battery"), "Win32 GetSystemPowerStatus; native BatteryFlag is not CIM BatteryStatus")}
    machine.update(metadata_source="Win32/registry, not CIM; unavailable fields are not inferred", clock_settings_declared=args.DeclaredClockSettings, power_mode_declared=args.DeclaredPowerMode,
                   temperatures="not collected", note="Display inventory is not selected GPU proof; native GL metadata supplies actual device validation.")
    power = output / "powercfg-stdout.log"
    power_error = output / "powercfg-stderr.log"
    try:
        fact = collected(Path(os.environ.get("SystemRoot", "C:/Windows")) / "System32/powercfg.exe", ["/getactivescheme"], output, power, power_error, 15)
        machine["active_power_plan"] = {"status": "observed" if fact["launched"] and not fact["timeout"] and fact["exit_code"] == 0 else "unavailable", "query": "powercfg /getactivescheme (read-only)", "process": fact,
                                        "stdout_text": read_log(power) if power.exists() else None, "stderr_text": read_log(power_error) if power_error.exists() else None}
    except OSError as error:
        machine["active_power_plan"] = {"status": "unavailable", "error": str(error)}
    manifest = {"purpose": "GLFW frozen identity exploratory Q07 presampling; not formal same-source Q06 or A11 acceptance", "started_utc": utc_now(), "planned": plan,
                "planned_count": 9, "focus_policy": "allow-unfocused", "framebuffer": [1920, 1080], "vsync": 0, "seed": 424242, "identities": identities,
                "resources_root": str(assets), "resources": resources, "validator_imports": imports, "validator_imports_sha256": sha256(imports_path),
                "api_diagnostics": False, "status_polling": False, "retries": False, "sleep": False, "python": sys.version, "machine": machine,
                "user_declarations": {"power_mode": args.DeclaredPowerMode, "clock_settings": args.DeclaredClockSettings, "scope": "Human declaration, separate from observations; unknown remains unknown."}}
    write_json(output / "manifest.json", manifest)
    results, failure = [], None
    try:
        for round in plan:
            name = f"{round['scene']}-{round['repeat']}"
            case = new_directory(output / name)
            capture_dir = case / "capture"
            result = {"scene": round["scene"], "repeat": round["repeat"], "directory": str(case), "game": None, "validation": None, "metrics": None,
                      "memory": None, "runtime_logs": None, "passed": False, "error": None, "failure_kind": None}
            stage = "input_identity"
            try:
                for entry in identities:
                    assert_hash(entry["path"], entry["sha256"])
                stage = "game_process_collection"
                arguments = ["--benchmark", round["scene"], "--output", capture_dir, "--seed", "424242", "--width", "1920", "--height", "1080", "--vsync", "0", "--warmup-seconds", "10", "--sample-seconds", "30", "--focus-policy", "allow-unfocused"]
                result["game"] = collected(game, arguments, game.parent, case / "stdout.log", case / "stderr.log", args.TimeoutSeconds)
                require(result["game"]["launched"] and not result["game"]["timeout"] and not result["game"]["collector_error"], "Game process could not be collected.")
                stage = "actual_capture_validation"
                metrics_path = case / "validated-metrics.csv"
                result["validation"] = collected(validator, [capture_dir, round["scene"], round["repeat"], result["game"]["exit_code"], metrics_path], case, case / "validated-summary.yaml", case / "validation-stderr.log", 60)
                require(result["validation"]["launched"] and not result["validation"]["timeout"] and result["validation"]["exit_code"] == 0, "Native capture validation failed.")
                with metrics_path.open(encoding="utf-8-sig", newline="") as stream:
                    metrics = list(csv.DictReader(stream))
                require(len(metrics) == 1 and metrics[0]["valid"] == "true", "Missing validated capture metrics.")
                result["metrics"] = metrics[0]
                stage = "memory_data_quality"
                result["memory"] = memory_statistics(capture_dir / "memory.csv")
                require(result["memory"]["passed"], "Memory coverage/values incomplete.")
                stage = "runtime_log_validation"
                stdout, stderr = read_log(case / "stdout.log"), read_log(case / "stderr.log")
                lines = (stdout + "\n" + stderr).splitlines()
                result["runtime_logs"] = {"shutdown_success_marker": "[runtime] shutdown complete; exit_code=0" in stdout,
                                          "opengl_diagnostics": [x for x in lines if "OpenGL diagnostic [" in x], "cleanup_failures": [x for x in lines if "[cleanup]" in x or "[runtime] shutdown failed:" in x],
                                          "stderr_nonempty": bool(stderr.strip()), "note": "Emitted logs only; Release may lack GL debug callback."}
                require(result["runtime_logs"]["shutdown_success_marker"] and not result["runtime_logs"]["opengl_diagnostics"] and not result["runtime_logs"]["cleanup_failures"], "Shutdown/GL/cleanup diagnostics failed.")
                result["passed"] = True
            except (ValueError, OSError) as error:
                result.update(error=str(error), failure_kind=stage)
                failure = str(error)
            results.append(result)
            write_json(case / "result.json", result)
            with (output / "runs.jsonl").open("a", encoding="utf-8") as stream:
                stream.write(json.dumps(result, ensure_ascii=False, allow_nan=False) + "\n")
            if not result["passed"]:
                break
        for entry in identities:
            assert_hash(entry["path"], entry["sha256"])
        for entry in resources:
            assert_hash(assets / entry["path"], entry["sha256"])
    except (ValueError, OSError) as error:
        failure = str(error)
    summary = {"purpose": manifest["purpose"], "ended_utc": utc_now(), "manifest": "manifest.json", "records": "runs.jsonl", "planned_count": 9,
               "attempted_count": len(results), "actual_game_processes": sum(bool(r["game"] and r["game"]["launched"]) for r in results), "passed_count": sum(r["passed"] for r in results),
               "failure": failure, "rounds": results, "passed": failure is None and len(results) == 9 and all(r["passed"] for r in results), "sample_window_seconds": 30,
               "memory_scope": "Periodic sample-window observations, not instantaneous peaks or process VRAM", "q06_comparison_complete": False, "q07_budget_approved": False, "a11_accepted": False, "fix1_resumed": False}
    write_json(output / "summary.json", summary)
    require(summary["passed"], f"Exploratory presampling failed; see {output}")


if __name__ == "__main__":
    cli(main)
