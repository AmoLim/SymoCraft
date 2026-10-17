"""Three bounded edit diagnostics; no formal budget replacement."""

import argparse
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
PHASES = ("facade_prepare", "backend_setup", "slot_wait_collect", "query_collect", "command_begin",
          "resource_retire", "descriptor_retire", "cpu_pack", "descriptor_allocate", "descriptor_update",
          "selection_allocate", "selection_write", "command_record", "debug_validation", "presentation_latency",
          "queue_submit", "queue_present", "backend_tail", "facade_result")
WORK = ("layer_scan", "mesh_create_total", "mesh_update_total", "mesh_validate", "mesh_owned_copy",
        "mesh_bounds", "mesh_allocate", "mesh_map_copy", "mesh_debug")


def distribution(values):
    return {"mean": sum(values) / len(values), "p50": budget.rank(values, .5), "p95": budget.rank(values, .95),
            "p99": budget.rank(values, .99), "max": max(values)} if values else None


def srv_operations(raw, sample_ids):
    models = {row.get("srv_binding_model", "unmeasured_v1") for row in raw}
    budget.require(len(models) == 1, "Mixed SRV observation models")
    model = next(iter(models))
    if model == "unmeasured_v1":
        return {"model": model, "sample": None, "all_recorded": None, "per_frame": None}
    budget.require(model == "frame_slot_delta_v1", "Unknown SRV observation model")
    previous, deltas, capacities, slots = (0, 0), {}, {}, set()
    last_id = -1
    for row in raw:
        frame_id, slot, capacity = (int(row[key]) for key in ("renderer_frame_id", "srv_slot", "srv_capacity"))
        current = tuple(int(row[key]) for key in ("srv_heaps_created", "srv_views_written"))
        budget.require(frame_id > last_id and 0 <= slot < 3 and capacity > 0, "Invalid SRV frame/slot/capacity")
        budget.require(all(a >= b for a, b in zip(current, previous)), "SRV counters decreased")
        deltas[str(frame_id)] = dict(zip(("heap_creations", "view_writes"), (a - b for a, b in zip(current, previous))))
        capacities[str(slot)] = max(capacities.get(str(slot), 0), capacity)
        if str(frame_id) in sample_ids:
            slots.add(slot)
        previous, last_id = current, frame_id
    budget.require(bool(sample_ids) and all(frame in deltas for frame in sample_ids), "Missing same-ID SRV observation")
    return {"model": model, "sample": {key: sum(deltas[frame][key] for frame in sample_ids)
             for key in ("heap_creations", "view_writes")},
            "all_recorded": dict(zip(("heap_creations", "view_writes"), previous)),
            "sample_slots": sorted(slots), "max_capacity_per_slot": capacities, "per_frame": deltas}


def analyze(capture, timings):
    frames, raw = budget.rows(capture / "frames.csv"), budget.rows(timings)
    ids = [row["renderer_frame_id"] for row in raw]
    budget.require(len(ids) == len(set(ids)), "Duplicate diagnostic ID")
    by_id = dict(zip(ids, raw))
    joined, layers = [], []
    for frame in frames:
        if not 10 <= float(frame["elapsed_s"]) < 60:
            continue
        budget.require(frame["phase"] == "sample", "Wrong sample phase")
        budget.require(frame["renderer_frame_id"] in by_id, "Missing same-ID timing")
        row = by_id[frame["renderer_frame_id"]]
        budget.require(row["completed"] == "1" and row["screenshot"] == "0", "Incomplete/screenshot sample")
        wall = {p: float(row[p + "_wall_ms"]) for p in PHASES}
        work = {p: float(row[p + "_work_ms"]) for p in WORK}
        budget.require(all(math.isfinite(v) and v >= 0 for v in [*wall.values(), *work.values()]), "Invalid diagnostic time")
        for phase in PHASES:
            if phase not in ("selection_allocate", "selection_write"):
                budget.require(int(row[phase + "_visits"]) > 0, "Missing phase: " + phase)
        selection = int(row["selection_allocate_visits"]) > 0
        budget.require(int(row["selection_write_visits"]) == int(selection), "Incomplete selection measurement")
        layer = schema.layer_work(row, frame, "per_vertex_v1")
        layers.append(layer)
        if layer["model"] == "cached_max_v1":
            work["layer_summary_check"] = layer["wall_ms"]
        uploads = sum(int(row[p + "_work_visits"]) for p in ("mesh_create_total", "mesh_update_total"))
        budget.require(uploads == int(frame["upload_calls"]), "Pre-Render upload lost/misattributed to frame")
        budget.require(layer["wall_ms"] <= wall["facade_prepare"] + .001, "Nested layer work exceeds facade interval")
        cpu = None
        if row["cpu_valid"] == "1":
            cpu = {p: float(row[p + "_cpu_ms"]) for p in PHASES}
            budget.require(all(math.isfinite(v) and v >= 0 for v in cpu.values()), "Invalid CPU time")
        joined.append((frame, row, wall, work, cpu))
    budget.require(bool(joined), "No matched samples")
    srv = srv_operations(raw, {x[0]["renderer_frame_id"] for x in joined})
    models = {item["model"] for item in layers}
    budget.require(len(models) == 1, "Mixed layer validation models")
    model = layers[0]["model"]
    work_names = (*WORK, "layer_summary_check") if model == "cached_max_v1" else WORK
    partitions = {"all": joined, "edit": [x for x in joined if int(x[0]["edits"]) > 0],
                  "non_edit": [x for x in joined if int(x[0]["edits"]) == 0]}
    stats = {}
    for name, items in partitions.items():
        stats[name] = {"frames": len(items), "frame_ms": distribution([float(x[0]["frame_ms"]) for x in items]),
                       "phases_wall_ms": {p: distribution([x[2][p] for x in items]) for p in PHASES},
                       "nested_work_ms": {p: distribution([x[3][p] for x in items]) for p in work_names}}
    def detail(item):
        frame, row, wall, work, cpu = item
        return {"renderer_frame_id": int(frame["renderer_frame_id"]),
                **{key: float(frame[key]) for key in ("elapsed_s", "frame_ms", "cpu_prepare_ms", "cpu_submit_ms", "present_ms", "mesh_ms", "pack_ms", "upload_cpu_ms", "instrumentation_ms", "gpu_draw_ms")},
                "edits": int(frame["edits"]), "rebuilt_chunks": int(frame["rebuilt_chunks"]),
                "phases_wall_ms": wall, "nested_work_ms": work, "phases_cpu_ms": cpu,
                "exclusive_render_wall_ms": sum(wall.values()),
                "native_cumulative": {key: int(row[key]) for key in ("mesh_buffers_created", "mesh_buffers_released", "wait_calls")}}
    tails = [x for x in joined if float(x[0]["frame_ms"]) > 20]
    return {"matched_sample_frames": len(joined), "timing_rows": len(raw), "statistics": stats,
            "srv_operations": {key: value for key, value in srv.items() if key != "per_frame"},
            "layer_validation": {"model": model, "wall_ms": distribution([x["wall_ms"] for x in layers]),
                                 "visits": sum(x["visits"] for x in layers),
                                 "scanned_vertices": sum(x["scanned_vertices"] for x in layers)},
            "cpu_missing_frames": sum(x[4] is None for x in joined),
            "method": "Exclusive Render phases; model-specific layer work is nested in facade_prepare. cached_max_v1 keeps retired scan fields at zero, not a missing measurement; per_vertex_v1 is legacy scan. Mesh work before Render belongs once to next original ID; never add nested work/pack/upload_cpu to parents.",
            "over_20ms": {"frames": len(tails), "edit_frames": sum(int(x[0]["edits"]) > 0 for x in tails),
                          "elapsed_range": [min(float(x[0]["elapsed_s"]) for x in tails), max(float(x[0]["elapsed_s"]) for x in tails)] if tails else None},
            "sample_upload_calls": sum(int(x[0]["upload_calls"]) for x in joined),
            "sample_update_work_calls": sum(int(x[1]["mesh_update_total_work_visits"]) for x in joined),
            "top_10_frames": [detail(x) for x in sorted(joined, key=lambda x: float(x[0]["frame_ms"]), reverse=True)[:10]],
            "top_5_update_frames": [detail(x) for x in sorted([x for x in joined if int(x[1]["mesh_update_total_work_visits"])], key=lambda x: x[3]["mesh_update_total"], reverse=True)[:5]],
            "artifacts": {str(p): budget.sha(p) for p in [timings, *sorted(capture.iterdir())] if p.is_file()}}


def snapshots(paths):
    return {str(p): budget.sha(p) for p in sorted(set(paths))}


def collect(args):
    probe = Path(args.probe).resolve(strict=True)
    consumer = Path(args.native_consumer).resolve(strict=True)
    reporter = Path(args.report_probe).resolve(strict=True)
    output = Path(args.output_directory).resolve()
    output.mkdir(parents=True, exist_ok=False)
    previous = ROOT / "out/m3-t3/vulkan-static-timing-001"
    previous_result = json.loads((previous / "result.json").read_text())
    formal = ROOT / "out/m3-t3/formal-budget-10-50-001"
    budget.require(budget.sha(formal / "result.json") == "8c00f7bc480c977dc282d6d8ca2eb34e6b101761fbd031298f50891d1baa9a47", "Old D3D12 formal result changed")
    frozen_paths = [Path(p) for p in previous_result["frozen_before"]]
    for p, digest in previous_result["frozen_before"].items():
        budget.require(budget.sha(Path(p)) == digest, "Old frozen evidence changed")
    for folder in (previous, formal):
        frozen_paths += [p for p in folder.rglob("*") if p.is_file()]
    old = json.loads((formal / "result.json").read_text())
    for relative, digest in old["package_identity"].items():
        path = Path(old["game"]).parent / relative
        budget.require(budget.sha(path) == digest, "Old D3D12 package changed")
        frozen_paths.append(path)
    frozen = snapshots(frozen_paths)
    sources = snapshots([Path(p) for p in previous_result["source_inputs"]] + [ROOT / p for p in (
        "test/integration/renderer/d3d12_frame_timing_probe.cpp", "test/integration/renderer/verify-d3d12-frame-timings.py",
        "test/integration/renderer/test_d3d12_frame_timings.py", "test/integration/renderer/frame_timing_schema.py")])
    package, validators = budget.identities(probe), snapshots([consumer, reporter])
    manifest = {"schema": "symocraft.d3d12-edit-stage-diagnostic.v3", "diagnostic_only": True,
                "layer_validation_model": "cached_max_v1",
                "srv_binding_model": "frame_slot_delta_v1",
                "formal_budget_replacement": False, "automatic_retest": False, "collection_valid": False,
                "started_utc": datetime.now(timezone.utc).isoformat(), "machine": budget.windows_hardware(),
                "probe": str(probe), "probe_package": package, "validators": validators, "source_inputs": sources,
                "frozen_before": frozen, "rounds": [],
                "policy": "edit only; 3 independent processes; 10s warmup + 50s sample; validation1; 1920x1080/4x/Immediate; seed424242; allow-unfocused; all slow frames retained",
                "perturbation": "Extra phase clocks and per-draw wall-only summary checks (old D3D12 used wall-only scans); fixed-capacity in-memory recorder; GetResourceAllocationInfo on mesh creation. No per-frame disk IO; CPU counter coarse; not a budget/memory comparison."}
    budget.write(output / "result.json", manifest)
    try:
        for repeat in range(1, 4):
            directory = output / f"edit-{repeat}"
            directory.mkdir()
            capture, timing = directory / "capture", directory / "timings.csv"
            command = [probe, timing, "SymoCraft", "--renderer", "d3d12", "--benchmark", "edit", "--output", capture,
                       "--seed", "424242", "--width", "1920", "--height", "1080", "--vsync", "0",
                       "--warmup-seconds", "10", "--sample-seconds", "50", "--focus-policy", "allow-unfocused", "--msaa", "4", "--validation", "1"]
            print(f"Diagnostic D3D12 edit {repeat}/3: 10+50 seconds", flush=True)
            stdout = budget.execute(command, directory, "game", 120)
            budget.require("[runtime] shutdown complete; exit_code=0" in stdout and "[performance] exported; valid_run=1" in stdout, "Incomplete production export/shutdown")
            budget.require("overflow=0; warnings=0; errors=0; corruptions=0; retired=1; production_deadline_ms=2500" in stdout, "Recorder/native/deadline mismatch")
            budget.execute([consumer, capture, "d3d12"], directory, "native-consumer", 15)
            decoded = budget.execute([reporter, capture, "D3D12", "edit"], directory, "game-report", 15)
            item = {"repeat": repeat, "capture": str(capture), "summary": budget.analyze(capture),
                    "native_report": json.loads(decoded), "timings": analyze(capture, timing)}
            budget.require(item["timings"]["layer_validation"]["model"] == "cached_max_v1", "Stale probe: cached model required")
            budget.require(item["timings"]["srv_operations"]["model"] == "frame_slot_delta_v1", "Stale probe: native SRV observations required")
            manifest["rounds"].append(item)
            budget.require(budget.identities(probe) == package, "Probe/package changed during sampling")
            budget.write(output / "result.json", manifest)
        budget.require(snapshots([Path(p) for p in sources]) == sources, "Sources changed during sampling")
        budget.require(snapshots([Path(p) for p in validators]) == validators, "Validators changed during sampling")
        manifest["frozen_after_matches"] = snapshots(frozen_paths) == frozen
        budget.require(manifest["frozen_after_matches"], "Frozen data changed during sampling")
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
