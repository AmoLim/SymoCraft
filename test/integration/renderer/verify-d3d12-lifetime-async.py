"""Bounded public-facade observations of real D3D12 lifetime and async capacity."""
import argparse
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3]/"scripts"))
from verification_common import (ROOT, capture, clean_environment, cli, evidence_sources, identity,
                                 load_json, new_directory, read_log, require, utc_now, workspace_path, write_json)

MODES = ("lifetime", "async-pending", "async-overflow")


def validate(mode, report, errors):
    require(report.get("schema") == "symocraft.d3d12-lifetime-async.v1" and report.get("mode") == mode and
            report.get("success") is True and report.get("public_facade_and_production_backend") is True,
            "Missing/mismatched native lifetime evidence")
    require(report.get("device") == "NVIDIA GeForce RTX 5070 Ti" and report.get("validation_enabled") is True and
            report.get("samples") == 1 and bool(report.get("identity")) and bool(report.get("driver")), "Actual native scope mismatch")
    require(report.get("info_queue_checks", 0) > 0 and all(report.get(k) == 0 for k in
            ("validation_warnings", "validation_errors", "validation_corruptions")), "Native diagnostics are not clean")
    require(report.get("native_cleared") is True and report.get("quarantined") is False and
            report.get("live_resource_bytes") == 0 and report.get("peak_resource_bytes", 0) > 0 and
            report.get("mesh_buffers_created", 0) > 0 and report["mesh_buffers_created"] == report.get("mesh_buffers_released") and
            report.get("textures_created", 0) > 0 and report["textures_created"] == report.get("textures_released"),
            "Persistent native resource retirement incomplete")
    dropped = list(map(int, re.findall(r"D3D12 GPU sample discarded: frame=(\d+)", errors)))
    if mode == "lifetime":
        events = report.get("events", [])
        require(len(events) == 3 and [e.get("step") for e in events] == [0, 1, 2], "Missing update/empty/destroy event")
        for event in events:
            require(event.get("pending_submit_fence", 0) > event.get("completed_before_release", 0) and
                    event.get("gate_before_release") == 0 and event.get("watchdog_fired") is False and
                    event.get("allocation_failure_preserved_old_version") is True and event.get("old_versions_retained") is True and
                    event.get("old_mesh_retired") is True and event.get("capacity_creates") == 48 and
                    event.get("deleted_texture_retired") == (event["step"] == 2), "Unproven actual in-flight retirement")
    if mode == "async-pending":
        require(report.get("pending_poll_calls") == 10 and report.get("poll_added_waits") == report.get("poll_added_query_reads") == 0 and
                report.get("pending_submit_fence", 0) > report.get("completed_before_release", 0) and
                report.get("resolved_frame_id") == 9007199254740993 and report.get("watchdog_fired") is False,
                "Poll did not prove pending non-waiting exact frame identity")
    if mode == "async-overflow":
        submitted, returned = report.get("submitted_ids", []), report.get("returned_ids", [])
        require(submitted == [9007199254740993+3*i for i in range(301)] and
                len(returned) == len(set(returned)) == 256 and len(dropped) == len(set(dropped)) == report.get("sample_drops") == 45 and
                not set(returned).intersection(dropped) and set(returned).union(dropped) == set(submitted),
                "Native bounded ring loss/duplication is not fully explained")
    else:
        require(not dropped and report.get("sample_drops") == 0, "Unexpected GPU result loss")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", required=True)
    parser.add_argument("--output-directory", required=True)
    args = parser.parse_args()
    build = workspace_path(args.build_directory, ROOT/"out")
    output = new_directory(workspace_path(args.output_directory, ROOT/"out"))
    exe = build/"bin/renderer_d3d12_lifetime_async_probe.exe"
    original = evidence_sources([ROOT/p for p in ("game", "cmake", "test/integration/renderer")], [ROOT/"test/integration/CMakeLists.txt"])
    inputs = [identity(p) for p in (exe, build/"CMakeCache.txt", build/"build.ninja",
        build/"game/modules/renderer/symocraft_renderer.lib", build/"game/modules/renderer/symocraft_renderer_d3d12.lib")]
    inputs.extend(identity(p) for p in sorted((build/"bin/assets/shaders/d3d12").glob("*.dxil")))
    write_json(output/"source-inputs.json", original)
    result = {"status": "running", "started_utc": utc_now(), "inputs": inputs, "cases": [], "performance_acceptance": False}
    try:
        for mode in MODES:
            arguments = [mode, build/"bin/assets/shaders/d3d12", output/mode]
            case = {"mode": mode, "command": list(map(str, [exe, *arguments]))}; result["cases"].append(case)
            case["process"] = capture(exe, arguments, output, output/(mode+".stdout.log"), output/(mode+".stderr.log"), 30, clean_environment())
            write_json(output/"result.json", result)
            require(case["process"]["exit_code"] == 0 and not case["process"]["timed_out"], "Native case failed: "+mode)
            case["report"] = load_json(output/mode/"report.json")
            validate(mode, case["report"], read_log(output/(mode+".stderr.log")))
            case["status"] = "passed"; write_json(output/"result.json", result)
            print(mode, "passed", flush=True)
        require(all(identity(p["path"]) == p for p in [*original, *inputs]), "Source/build inputs changed during native checks")
        result["status"] = "passed"
    except BaseException as error:
        result["status"] = "failed"; result["error"] = str(error); raise
    finally:
        result["finished_utc"] = utc_now(); write_json(output/"result.json", result)


if __name__ == "__main__": cli(main)
