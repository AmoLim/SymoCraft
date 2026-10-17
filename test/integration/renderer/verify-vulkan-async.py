"""Observe bounded asynchronous samples against the actual production Vulkan backend."""
import argparse
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3]/"scripts"))
from verification_common import (ROOT, capture, clean_environment, cli, evidence_sources, identity, load_json,
                                 load_script, new_directory, read_log, require, utc_now, workspace_path, write_json)

MODES = ("async-gated", "async-overflow", "async-unsupported", "async-discarded", "async-device-error")


def validate(mode, report, errors):
    require(report.get("success") is True and report.get("mode") == mode and
            report.get("schema") == "symocraft.vulkan-production-lifetime-fault.v1", "Missing/mismatched native evidence")
    require(report.get("validation_errors") == report.get("validation_warnings") == 0, "Native validation diagnostic")
    terminal = mode == "async-device-error"
    require(report["quarantined"] == terminal and report["native_destroyed"] == report["loader_released"] == (not terminal), "Wrong native retirement")
    if not terminal:
        require(report["live_bytes"] == 0 and report["buffers_created"] == report["buffers_released"] and
                report["images_created"] == report["images_released"], "Incomplete normal native cleanup")
    if mode == "async-gated":
        require(report.get("pending_poll_calls") == 10 and report.get("pending_fence") == 1 and report.get("pending_timeline") == 0
                and report.get("poll_added_waits") == 0 and report.get("actual_in_flight") is True
                and report.get("resolved_frame_id") == 2, "Unready Poll was not a real non-waiting frame-ID test")
    if mode == "async-overflow":
        submitted, returned = report.get("submitted_ids", []), report.get("returned_ids", [])
        dropped = list(map(int, re.findall(r"Vulkan GPU sample discarded: frame=(\d+)", errors)))
        require(len(submitted) == len(set(submitted)) == 301 and len(returned) == len(set(returned)) == 256
                and len(dropped) == len(set(dropped)) == 45 and not set(returned).intersection(dropped)
                and set(returned).union(dropped) == set(submitted), "Sample loss/duplication not explained by bounded ring diagnostics")
    if mode in {"async-unsupported", "async-discarded"}:
        require(report.get("missing_reason") == ("unsupported" if mode == "async-unsupported" else "discarded") and
                report.get("missing_has_duration") is False, "Missing GPU result fabricated duration/reason")
    if mode == "async-unsupported":
        require(report.get("simulated_unsupported") is True and report["native_query_calls"] == 0,
                "Injected unsupported capability mislabeled as actual hardware unsupported")
    if terminal:
        require(report.get("injected_query_device_lost") is True and report.get("first_native_code") == "-4" and
                report.get("first_operation") == "ReadTimestampQuery" and report.get("no_further_submit_or_present") is True
                and report.get("repeated_shutdown_not_success") is True and report["live_bytes"] > 0,
                "Query error became valid zero time or resumed a failed renderer")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", required=True)
    parser.add_argument("--output-directory", required=True)
    parser.add_argument("--api-dump", action="store_true")
    args = parser.parse_args()
    build = workspace_path(args.build_directory, ROOT/"out")
    output = new_directory(workspace_path(args.output_directory, ROOT/"out"))
    exe = build/"bin/renderer_vulkan_lifetime_fault_probe.exe"
    original = evidence_sources([ROOT/p for p in ("game", "cmake", "test/integration/renderer")], [ROOT/"test/integration/CMakeLists.txt"])
    write_json(output/"source-inputs.json", original)
    inputs = [identity(p) for p in (exe, build/"CMakeCache.txt", build/"build.ninja")]
    result = {"status": "running", "started_utc": utc_now(), "inputs": inputs, "cases": [], "performance_acceptance": False}
    helper = load_script(ROOT/"test/integration/renderer/verify-draw-window.py")
    env = clean_environment()
    for key in ("VK_INSTANCE_LAYERS", "VK_APIDUMP_DETAILED", "VK_APIDUMP_FLUSH"): env.pop(key, None)
    if args.api_dump:
        env.update(VK_INSTANCE_LAYERS="VK_LAYER_LUNARG_api_dump", VK_APIDUMP_DETAILED="false", VK_APIDUMP_FLUSH="true")
    try:
        for mode in MODES:
            case = {"mode": mode, "command": [str(exe), mode, str(output/mode)]}
            result["cases"].append(case)
            case["process"] = capture(exe, [mode, output/mode], output, output/(mode+".stdout.log"), output/(mode+".stderr.log"), 30, env)
            write_json(output/"result.json", result)
            require(case["process"]["exit_code"] == 0 and not case["process"]["timed_out"], "Native async case failed: "+mode)
            report, errors = load_json(output/mode/"report.json"), read_log(output/(mode+".stderr.log"))
            if args.api_dump:
                notices = [line for line in errors.splitlines() if "Vulkan validation:" in line]
                require(notices == [helper.LAYER_NOTICE]*2 and report["validation_warnings"] == 2, "Unexpected observer diagnostics")
                case["observer_notices"] = 2
                normalized = dict(report, validation_warnings=0)
                validate(mode, normalized, errors)
                if mode == "async-gated":
                    log = read_log(output/(mode+".stdout.log"))
                    interval = helper.interval(log, "BATCH4_POLL_BEGIN", "BATCH4_POLL_END")
                    calls = helper.CALL.findall(interval)
                    require(calls.count("vkGetFenceStatus") == 10 and not any(x in calls for x in
                        ("vkWaitForFences", "vkQueueSubmit", "vkQueuePresentKHR", "vkAcquireNextImageKHR", "vkGetQueryPoolResults")),
                        "Pending Poll entered blocking/native submission work")
                    case["pending_native_calls"] = calls
            else:
                require("Vulkan validation:" not in errors, "Unaccounted native diagnostic")
                validate(mode, report, errors)
            case["report"] = report
            case["status"] = "passed"
            write_json(output/"result.json", result)
            print(mode, "passed", flush=True)
        require(all(identity(p["path"]) == p for p in [*original, *inputs]), "Async source/build inputs changed")
        result["status"] = "passed"
    except BaseException as error:
        result["status"] = "failed"; result["error"] = str(error); raise
    finally:
        result["finished_utc"] = utc_now()
        write_json(output/"result.json", result)


if __name__ == "__main__": cli(main)
