"""Bounded static-only Vulkan diagnostics, never a replacement budget verdict."""

import argparse
from collections import Counter
from datetime import datetime, timezone
import importlib.util
import json
import math
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location("budget", Path(__file__).with_name("verify-d3d12-budgets.py"))
budget = importlib.util.module_from_spec(spec)
spec.loader.exec_module(budget)
spec = importlib.util.spec_from_file_location("timing_schema", Path(__file__).with_name("frame_timing_schema.py"))
schema = importlib.util.module_from_spec(spec)
spec.loader.exec_module(schema)
spec = importlib.util.spec_from_file_location("native_observation", Path(__file__).with_name("vulkan_native_observation.py"))
native_observation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(native_observation)

GROUPS = {
    "facade": ("facade_prepare", "facade_result"),
    "resources": ("resource_retire", "selection_allocate", "selection_write", "uniform_allocate", "uniform_pack_write"),
    "descriptors": ("descriptor_allocate", "descriptor_update"),
    "synchronization": ("slot_wait_collect", "acquire_image", "present_fence_wait"),
    "commands": ("command_begin", "command_record"),
    "submit_present": ("queue_submit", "queue_present"),
    "other_backend": ("backend_setup", "cpu_pack", "backend_tail"),
}
PHASES = tuple(phase for group in GROUPS.values() for phase in group)


def distribution(values):
    return {"mean": sum(values) / len(values), "p50": budget.rank(values, .5),
            "p95": budget.rank(values, .95), "p99": budget.rank(values, .99), "max": max(values)}


def analyze(capture, timings):
    frames = budget.rows(capture / "frames.csv")
    raw = budget.rows(timings)
    ids = [row["renderer_frame_id"] for row in raw]
    budget.require(len(ids) == len(set(ids)), "Duplicate timing frame ID")
    by_id = dict(zip(ids, raw))
    joined, layers = [], []
    for frame in frames:
        if not 10 <= float(frame["elapsed_s"]) < 60:
            continue
        budget.require(frame["phase"] == "sample", "Wrong diagnostic sample window")
        budget.require(frame["renderer_frame_id"] in by_id, "Missing same-ID timing; never fill zero")
        timing = by_id[frame["renderer_frame_id"]]
        budget.require(timing["screenshot"] == "0" and timing["completed"] == "1", "Screenshot/incomplete frame entered sample")
        wall = {phase: float(timing[phase + "_wall_ms"]) for phase in PHASES}
        budget.require(all(math.isfinite(value) and value >= 0 for value in wall.values()), "Invalid wall timing")
        budget.require(all(int(timing[phase + "_visits"]) > 0 for phase in PHASES), "Missing diagnostic stage")
        layer = schema.layer_work(timing, frame, "unmeasured_v1")
        layers.append(layer)
        if layer["wall_ms"] is not None:
            budget.require(layer["wall_ms"] <= wall["facade_prepare"] + .001, "Nested summary exceeds facade interval")
        cpu = None
        if timing["cpu_valid"] == "1":
            cpu = {phase: float(timing[phase + "_cpu_ms"]) for phase in PHASES}
            budget.require(all(math.isfinite(value) and value >= 0 for value in cpu.values()), "Invalid CPU timing")
        joined.append((frame, timing, wall, cpu))
    budget.require(bool(joined), "No matched sample frames")
    budget.require(len({item["model"] for item in layers}) == 1, "Mixed layer validation models")
    measured_layers = layers[0]["wall_ms"] is not None
    cpu_complete = all(item[3] is not None for item in joined)
    phases = {phase: {"wall_ms": distribution([item[2][phase] for item in joined]),
                       "sum_cpu_ms": sum(item[3][phase] for item in joined) if cpu_complete else None} for phase in PHASES}
    groups = {group: {"wall_ms": distribution([sum(item[2][p] for p in names) for item in joined]),
                       "sum_cpu_ms": sum(sum(item[3][p] for p in names) for item in joined) if cpu_complete else None}
              for group, names in GROUPS.items()}
    tails = [item for item in joined if float(item[0]["frame_ms"]) > 20]

    def detail(item):
        frame, timing, wall, cpu = item
        original = {field: float(frame[field]) for field in ("elapsed_s", "frame_ms", "cpu_prepare_ms", "cpu_submit_ms", "present_ms", "instrumentation_ms", "gpu_draw_ms")}
        return {"renderer_frame_id": int(frame["renderer_frame_id"]), **original,
                "timed_wall_ms": sum(wall.values()), "timed_cpu_ms": sum(cpu.values()) if cpu else None,
                "phases_wall_ms": wall, "phases_cpu_ms": cpu,
                "cumulative_native_counts": {field: int(timing[field]) for field in
                                             ("buffers_created", "buffers_released", "wait_calls", "swapchain_creations")}}

    dominance = Counter(max(GROUPS, key=lambda group: sum(item[2][p] for p in GROUPS[group])) for item in tails)
    totals = [sum(item[2].values()) for item in joined]
    native_counts = {field: sorted({int(item[1][field]) for item in joined}) for field in ("swapchain_creations",)}
    return {"matched_sample_frames": len(joined), "timing_rows": len(raw),
            "native_observation": native_observation.analyze(raw, frames, timings),
            "layer_validation": {"model": layers[0]["model"],
                                 "wall_ms": distribution([x["wall_ms"] for x in layers]) if measured_layers else None,
                                 "visits": sum(x["visits"] for x in layers) if measured_layers else None,
                                 "scanned_vertices": sum(x["scanned_vertices"] for x in layers) if measured_layers else None},
            "screenshot_rows_excluded": sum(row["screenshot"] == "1" for row in raw),
            "cpu_missing_frames": sum(item[3] is None for item in joined),
            "cpu_time_caveat": "GetThreadTimes 100ns units, coarse accounting; per-stage CPU may exceed wall due to quantization. Not an exact blocked-time or scheduler trace.",
            "frame_ms": distribution([float(item[0]["frame_ms"]) for item in joined]),
            "total_timed_wall_ms": distribution(totals), "phases": phases, "groups": groups,
            "over_20ms": {"frames": len(tails), "dominant_group_counts": dict(dominance),
                          "elapsed_range": [min(float(item[0]["elapsed_s"]) for item in tails), max(float(item[0]["elapsed_s"]) for item in tails)] if tails else None,
                          "sum_wall_by_phase_ms": {p: sum(item[2][p] for item in tails) for p in PHASES},
                          "sum_cpu_by_phase_ms": {p: sum(item[3][p] for item in tails) if tails and all(item[3] is not None for item in tails) else None for p in PHASES}},
            "native_counts": native_counts,
            "workload_sets": {field: sorted({item[0][field] for item in joined}) for field in
                              ("vertices", "draw_calls", "rebuilt_chunks", "resource_upload_bytes", "render_upload_bytes", "upload_calls", "edits", "focused")},
            "top_10_frames": [detail(item) for item in sorted(joined, key=lambda item: float(item[0]["frame_ms"]), reverse=True)[:10]],
            "artifacts": {str(path): budget.sha(path) for path in [timings, *sorted(capture.iterdir())] if path.is_file()}}


def snapshots(paths):
    return {str(path): budget.sha(path) for path in sorted(set(paths))}


def collect(args):
    probe = Path(args.probe).resolve(strict=True)
    native = Path(args.native_consumer).resolve(strict=True)
    report = Path(args.report_probe).resolve(strict=True)
    output = Path(args.output_directory).resolve()
    output.mkdir(parents=True, exist_ok=False)
    formal = ROOT / "out/m3-t3/vulkan-formal-budget-10-50-001"
    budget.require(budget.sha(formal / "result.json") == "0b295d12add24fa7a623dc648a50a34bf63236b7fa7149ad6345499846ba7850", "Old formal result changed")
    preflight = json.loads((ROOT / "out/m3-t3/vulkan-budget-preflight-002/result.json").read_text())
    frozen = [Path(item["path"]) for item in preflight["installed_inputs"]]
    for item in preflight["installed_inputs"] + [preflight["old_d3d12_budget_result"]]:
        budget.require(budget.sha(Path(item["path"])) == item["sha256"], "Frozen installed/D3D12 input changed")
    frozen += [Path(preflight["old_d3d12_budget_result"]["path"])]
    frozen += [path for path in formal.rglob("*") if path.is_file()]
    added = [ROOT / path for path in (
        "game/modules/renderer/src/frame_diagnostics.h", "test/integration/renderer/vulkan_frame_timing_probe.cpp",
        "test/unit/renderer/frame_diagnostics_tests.cpp", "test/unit/renderer/CMakeLists.txt",
        "test/integration/renderer/verify-vulkan-frame-timings.py", "test/integration/renderer/test_vulkan_frame_timings.py",
        "test/integration/renderer/frame_timing_schema.py")]
    sources = snapshots([Path(item["path"]) for item in preflight["source_inputs"]] + added)
    source_delta = [item["path"] for item in preflight["source_inputs"] if sources[item["path"]] != item["sha256"]]
    unchanged = snapshots(frozen)
    probe_identity = budget.identities(probe)
    validators = snapshots([native, report])
    manifest = {"schema": "symocraft.vulkan-static-stage-diagnostic.v2", "diagnostic_only": True,
                "layer_validation_model": "cached_max_v1",
                "formal_budget_replacement": False, "automatic_retest": False, "collection_valid": False,
                "started_utc": datetime.now(timezone.utc).isoformat(), "machine": budget.windows_hardware(),
                "probe": str(probe), "probe_package": probe_identity, "validators": validators,
                "source_inputs": sources, "source_delta_from_formal": source_delta, "frozen_before": unchanged,
                "policy": "static only; 3 independent processes; 10s warmup + 50s sample; validation1; 1920x1080/4x/Immediate; seed424242; allow-unfocused; no discard/retry",
                "diagnostic_perturbation": "19 stage wall/thread-CPU boundaries plus new per-draw wall-only summary checks (legacy Vulkan had no nested layer clock); bounded in-memory records; no per-frame writes; private native counters; not an uninstrumented comparison",
                "rounds": []}
    budget.write(output / "result.json", manifest)
    try:
        for repeat in range(1, 4):
            directory = output / f"static-{repeat}"
            directory.mkdir()
            capture = directory / "capture"
            timings = directory / "timings.csv"
            command = [probe, timings, "SymoCraft", "--renderer", "vulkan", "--benchmark", "static", "--output", capture,
                       "--seed", "424242", "--width", "1920", "--height", "1080", "--vsync", "0",
                       "--warmup-seconds", "10", "--sample-seconds", "50", "--focus-policy", "allow-unfocused",
                       "--msaa", "4", "--validation", "1"]
            print(f"Diagnostic static {repeat}/3: 10+50 seconds", flush=True)
            stdout = budget.execute(command, directory, "game", 120)
            budget.require("[runtime] shutdown complete; exit_code=0" in stdout and "[performance] exported; valid_run=1" in stdout,
                           "Incomplete production shutdown/export")
            budget.require("overflow=0; warnings=0; errors=0; retired=1; production_deadline_ms=2500" in stdout, "Diagnostic overflow/native error/retirement/deadline mismatch")
            budget.execute([native, capture, "vulkan"], directory, "native-consumer", 15)
            decoded = budget.execute([report, capture, "Vulkan", "static"], directory, "game-report", 15)
            item = {"repeat": repeat, "capture": str(capture), "summary": budget.analyze(capture),
                    "native_report": json.loads(decoded), "timings": analyze(capture, timings)}
            budget.require(item["timings"]["layer_validation"]["model"] == "cached_max_v1", "Stale probe: cached model required")
            manifest["rounds"].append(item)
            budget.require(budget.identities(probe) == probe_identity, "Probe/package changed during collection")
            budget.write(output / "result.json", manifest)
        budget.require(snapshots([Path(path) for path in sources]) == sources, "Diagnostic sources changed during sampling")
        budget.require(snapshots([Path(path) for path in validators]) == validators, "Validator changed")
        manifest["frozen_after_matches"] = snapshots(frozen) == unchanged
        budget.require(manifest["frozen_after_matches"], "Frozen evidence changed")
        manifest["collection_valid"] = True
    except Exception as error:
        manifest["failure"] = str(error)
        raise
    finally:
        manifest["finished_utc"] = datetime.now(timezone.utc).isoformat()
        budget.write(output / "result.json", manifest)
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("probe", "native-consumer", "report-probe", "output-directory"):
        parser.add_argument("--" + name, required=True)
    collect(parser.parse_args())
    return 0


if __name__ == "__main__":
    sys.exit(main())
