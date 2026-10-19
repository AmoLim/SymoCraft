"""Collect an explicitly approved 10+50 second GL/modern paired budget."""

import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "scripts"))
from scripts_common import hidden_process
from benchmark import windows_hardware

WARMUP, SAMPLE, REPEATS = 10, 50, 3
SCENES = ("static", "walk", "edit")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=True, indent=2), encoding="utf-8")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identities(executable):
    return {str(path.relative_to(executable.parent)): sha(path)
            for path in sorted(executable.parent.rglob("*")) if path.is_file()
            and (path == executable or "assets" in path.relative_to(executable.parent).parts
                 or path.suffix.lower() == ".dll")}


def rows(path):
    with path.open(encoding="utf-8-sig", newline="") as source:
        return list(csv.DictReader(source))


def rank(values, percentile):
    require(bool(values), "No sample frames")
    require(all(math.isfinite(value) and value >= 0 for value in values), "Invalid frame duration")
    ordered = sorted(values)
    return ordered[math.ceil(len(ordered) * percentile) - 1]


def analyze(capture, warmup=WARMUP, sample=SAMPLE):
    require(math.isfinite(warmup) and math.isfinite(sample) and warmup >= 0 and sample > 0,
            "Invalid observation window")
    frames = rows(capture / "frames.csv")
    measured = [row for row in frames if warmup <= float(row["elapsed_s"]) < warmup + sample]
    require(all(row["phase"] == "sample" for row in measured), "Wrong sample phase")
    times = [float(row["frame_ms"]) for row in measured]
    memory = [row for row in rows(capture / "memory.csv")
              if warmup <= float(row["elapsed_s"]) < warmup + sample]
    require(bool(memory) and len(memory) >= sample - 5, "Insufficient approximately one-second memory observations")
    observed = [float(row["elapsed_s"]) for row in memory]
    require(observed[0] <= warmup + 2 and observed[-1] >= warmup + sample - 2,
            "Memory observations do not cover the formal sample window")
    require(all(0 < after - before <= 2 for before, after in zip(observed, observed[1:])),
            "Memory observations are unordered or have a gap exceeding two seconds")
    result = {"frames": len(times), "p95_ms": rank(times, .95), "p99_ms": rank(times, .99),
              "max_frame_ms": max(times), "memory_observations": len(memory),
              "sample_focused_frames": sum(row["focused"] == "1" for row in measured)}
    for field in ("working_set_bytes", "private_bytes"):
        require(all(row[field] != "" for row in memory), f"Missing {field}; never substitute zero")
        values = [int(row[field]) for row in memory]
        require(all(value > 0 for value in values), f"Invalid {field}")
        result[field] = max(values)
    result["focus_class"] = ("fully-unfocused" if result["sample_focused_frames"] == 0 else
                             "fully-focused" if result["sample_focused_frames"] == len(times) else "mixed")
    result["sample_unfocused_ms"] = sum(float(row["frame_ms"]) for row in measured if row["focused"] != "1")
    result["sample_total_ms"] = sum(times)
    return result


def compare(gl, modern):
    require(gl["p95_ms"] > 0 and gl["p99_ms"] > 0, "GL percentile baseline must be positive")
    conditions = {
        "p95_absolute": modern["p95_ms"] <= 10,
        "p99_absolute": modern["p99_ms"] <= 12,
        "p95_relative": modern["p95_ms"] <= gl["p95_ms"] * 1.10,
        "p99_relative": modern["p99_ms"] <= gl["p99_ms"] * 1.10,
        "working_set_relative": modern["working_set_bytes"] <= gl["working_set_bytes"] * 1.10,
        "private_bytes_relative": modern["private_bytes"] <= gl["private_bytes"] * 1.10,
    }
    focus_matches = gl["focus_class"] == modern["focus_class"] and gl["focus_class"] != "mixed"
    return {"conditions": conditions, "numeric_budget_passed": all(conditions.values()),
            "focus_conditions_match": focus_matches,
            "budget_passed_under_verified_conditions": all(conditions.values()) and focus_matches,
            "memory_ratios": {field: modern[field] / gl[field]
                              for field in ("working_set_bytes", "private_bytes")},
            "frame_ratios": {field: modern[field] / gl[field] if gl[field] else None
                             for field in ("p95_ms", "p99_ms")}}


def execute(command, directory, prefix, timeout):
    started = time.monotonic()
    process = {"command": list(map(str, command)), "cwd": str(directory),
               "started_utc": datetime.now(timezone.utc).isoformat(),
               "timed_out": False, "forced_termination": False}
    try:
        result = subprocess.run(process["command"], cwd=directory, capture_output=True,
                                timeout=timeout, **hidden_process())
    except subprocess.TimeoutExpired as error:
        (directory / (prefix + ".stdout.log")).write_bytes(error.stdout or b"")
        (directory / (prefix + ".stderr.log")).write_bytes(error.stderr or b"")
        process.update(exit_code=None, timed_out=True, forced_termination=True,
                       elapsed_seconds=time.monotonic() - started,
                       finished_utc=datetime.now(timezone.utc).isoformat())
        write(directory / (prefix + ".process.json"), process)
        raise
    (directory / (prefix + ".stdout.log")).write_bytes(result.stdout)
    (directory / (prefix + ".stderr.log")).write_bytes(result.stderr)
    process.update(exit_code=result.returncode, elapsed_seconds=time.monotonic() - started,
                   finished_utc=datetime.now(timezone.utc).isoformat())
    write(directory / (prefix + ".process.json"), process)
    require(result.returncode == 0, f"{prefix} exited {result.returncode}; original logs retained")
    return result.stdout.decode("utf-8-sig", errors="replace")


def collect(args):
    executable = Path(args.executable).resolve(strict=True)
    gl_executable = Path(args.opengl_executable or args.executable).resolve(strict=True)
    backend_name = args.modern_backend
    native = Path(args.native_consumer).resolve(strict=True)
    probe = Path(args.report_probe).resolve(strict=True)
    output = Path(args.output_directory).resolve()
    output.mkdir(parents=True, exist_ok=False)
    original = identities(executable)
    gl_original = identities(gl_executable)
    manifest = {"schema": f"symocraft.{backend_name}-approved-budget-10-50.v1",
                "started_utc": datetime.now(timezone.utc).isoformat(),
                "warmup_seconds": WARMUP, "sample_seconds": SAMPLE, "repeats": REPEATS,
                "game": str(executable), "package_identity": original,
                "modern_backend": backend_name, "opengl_game": str(gl_executable),
                "opengl_package_identity": gl_original,
                "native_consumer": str(native), "report_probe": str(probe),
                "native_consumer_sha256": sha(native), "report_probe_sha256": sha(probe),
                "collector_sha256": sha(Path(__file__)), "machine": windows_hardware(),
                "long_window_rare_stall_and_sustained_growth_coverage": False,
                "rounds": [], "pairs": [], "collection_valid": False, "budgets_passed": False}
    write(output / "result.json", manifest)
    try:
        for backend in ("opengl", backend_name):
            for scene in SCENES:
                for repeat in range(1, REPEATS + 1):
                    directory = output / f"{backend}-{scene}-{repeat}"
                    directory.mkdir()
                    capture = directory / "capture"
                    game = gl_executable if backend == "opengl" else executable
                    command = [game, "--renderer", backend, "--benchmark", scene, "--output", capture,
                               "--seed", "424242", "--width", "1920", "--height", "1080", "--vsync", "0",
                               "--warmup-seconds", str(WARMUP), "--sample-seconds", str(SAMPLE),
                               "--focus-policy", "allow-unfocused"]
                    if backend != "opengl":
                        command += ["--msaa", "4", "--validation", "1"]
                    print(f"{backend} {scene} {repeat}/3: 10s warmup + 50s sample", flush=True)
                    stdout = execute(command, directory, "game", WARMUP + SAMPLE + 60)
                    require("[runtime] shutdown complete; exit_code=0" in stdout and
                            "[performance] exported; valid_run=1" in stdout, "Incomplete game/export shutdown")
                    execute([native, capture, backend], directory, "native-consumer", 15)
                    expected_backend = {"d3d12": "D3D12", "vulkan": "Vulkan", "opengl": "OpenGL"}[backend]
                    decoded = execute([probe, capture, expected_backend, scene],
                                      directory, "game-report", 15)
                    item = {"backend": backend, "scene": scene, "repeat": repeat,
                            "command": list(map(str, command)), "capture": str(capture),
                            "summary": analyze(capture), "native_report": json.loads(decoded),
                            "artifacts": {path.name: sha(path) for path in capture.iterdir() if path.is_file()}}
                    manifest["rounds"].append(item)
                    require(identities(executable) == original, "Game/package bytes changed during sampling")
                    require(identities(gl_executable) == gl_original, "GL package bytes changed during sampling")
                    write(output / "result.json", manifest)
        for scene in SCENES:
            for repeat in range(1, REPEATS + 1):
                paired = [next(item for item in manifest["rounds"] if item["backend"] == backend
                               and item["scene"] == scene and item["repeat"] == repeat)
                          for backend in ("opengl", backend_name)]
                manifest["pairs"].append({"scene": scene, "repeat": repeat,
                                          **compare(paired[0]["summary"], paired[1]["summary"])})
        require(sha(native) == manifest["native_consumer_sha256"] and sha(probe) == manifest["report_probe_sha256"],
                "Validators changed during sampling")
        manifest["collection_valid"] = True
        manifest["budgets_passed"] = all(item["budget_passed_under_verified_conditions"] for item in manifest["pairs"])
    except Exception as error:
        manifest["failure"] = str(error)
        raise
    finally:
        manifest["finished_utc"] = datetime.now(timezone.utc).isoformat()
        write(output / "result.json", manifest)
    print(f"18 rounds valid; budget passed={manifest['budgets_passed']}", flush=True)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("executable", "native-consumer", "report-probe", "output-directory"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--modern-backend", choices=("d3d12", "vulkan"), default="d3d12")
    parser.add_argument("--opengl-executable", help="Same-version GL-only game when the modern package omits GL")
    result = collect(parser.parse_args())
    return 0 if result["budgets_passed"] else 4


if __name__ == "__main__":
    sys.exit(main())
