"""Bounded controlled Vulkan capability rejection and WSI branch evidence."""
import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3]/"scripts"))
from verification_common import (ROOT, capture, clean_environment, cli, evidence_sources, identity,
    load_json, new_directory, read_log, require, utc_now, workspace_path, write_json)

MODES = ("missing-extension", "missing-feature", "acquire-out-of-date", "acquire-suboptimal",
         "present-suboptimal", "present-out-of-date")


def validate(mode, report):
    require(report.get("schema") == "symocraft.vulkan-wsi.v1" and report.get("mode") == mode and
            report.get("success") is True and report.get("controlled_injection") is True and
            report.get("performance_acceptance") is False and report.get("public_facade_and_production_backend") is True,
            "Missing controlled native evidence")
    require(report.get("native_retired") is True and report.get("validation_warnings") == 0 and
            report.get("validation_errors") == 0, "Native retirement/validation failed")
    if mode.startswith("missing-"):
        key = "extension_rejections" if mode == "missing-extension" else "feature_rejections"
        require(report.get(key, 0) > 0 and report.get("device_creations") == 0 and
                report.get("unsupported_before_device") is True and report.get("error_operation") == "SelectDevice" and
                "swapchainMaintenance1" in report.get("error_message", ""), "No named pre-device capability rejection")
        return
    require(report.get("device") == "NVIDIA GeForce RTX 5070 Ti" and report.get("identity") and
            report.get("driver") and report.get("api") and report.get("device_creations") == 1 and
            report.get("swapchain_creations") == 4 and report.get("sample_ids_complete") is True and
            report.get("validation_enabled") is True and report.get("samples") == 1 and report.get("present_mode") == "Immediate",
            "Actual native configuration/identities incomplete")
    events = report.get("events", [])
    require(len(events) == 3, "Missing repeated WSI cycles")
    for event in events:
        calls = 0 if mode == "acquire-out-of-date" else 1
        require(all(event.get(key) == calls for key in ("native_acquire_calls", "native_submit_calls", "native_present_calls"))
                and all(event.get(key) is True for key in ("submit_fence_signaled", "rebuilt_once", "recovered_pixels_equal")),
                "Unbalanced synchronization or failed recovery")
        if mode == "acquire-out-of-date":
            require(event.get("effective_acquire") == -1000001004 and event.get("actual_acquire") is None and
                    event.get("actual_present") is None and event.get("effective_present") is None,
                    "Acquire out-of-date branch not exercised or uncalled API fabricated")
        elif mode == "acquire-suboptimal":
            require(event.get("actual_acquire") in (0, 1000001003) and
                    event.get("effective_acquire") == 1000001003, "Acquire suboptimal branch not exercised")
        else:
            expected = 1000001003 if mode == "present-suboptimal" else -1000001004
            require(event.get("actual_present") in (0, expected) and event.get("effective_present") == expected,
                    "Present branch not exercised")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", required=True)
    parser.add_argument("--output-directory", required=True)
    args = parser.parse_args()
    build = workspace_path(args.build_directory, ROOT/"out")
    output = new_directory(workspace_path(args.output_directory, ROOT/"out"))
    exe = build/"bin/renderer_vulkan_wsi_probe.exe"
    original = evidence_sources([ROOT/p for p in ("game", "cmake", "test/integration/renderer")],
                                [ROOT/"test/integration/CMakeLists.txt"])
    inputs = [identity(p) for p in (exe, build/"CMakeCache.txt", build/"build.ninja",
        build/"game/modules/renderer/symocraft_renderer.lib", build/"game/modules/renderer/symocraft_renderer_vulkan.lib")]
    inputs.extend(identity(p) for p in sorted((build/"bin/assets/shaders/vulkan").glob("*")) if p.is_file())
    write_json(output/"source-inputs.json", original)
    result = {"status": "running", "started_utc": utc_now(), "inputs": inputs, "cases": [], "performance_acceptance": False}
    try:
        for mode in MODES:
            arguments = [mode, build/"bin/assets/shaders/vulkan", output/mode]
            case = {"mode": mode, "command": list(map(str, [exe, *arguments]))}; result["cases"].append(case)
            case["process"] = capture(exe, arguments, output, output/(mode+".stdout.log"), output/(mode+".stderr.log"), 30, clean_environment())
            write_json(output/"result.json", result)
            require(case["process"]["exit_code"] == 0 and not case["process"]["timed_out"], "Native case failed: "+mode)
            case["report"] = load_json(output/mode/"report.json")
            validate(mode, case["report"])
            require("validation_errors=0; native_retired=true" in read_log(output/(mode+".stderr.log")), "Missing native cleanup log")
            case["status"] = "passed"; write_json(output/"result.json", result)
            print(mode, "passed", flush=True)
        require(all(identity(p["path"]) == p for p in [*original, *inputs]), "Source/build inputs changed during checks")
        result["status"] = "passed"
    except BaseException as error:
        result["status"] = "failed"; result["error"] = str(error); raise
    finally:
        result["finished_utc"] = utc_now(); write_json(output/"result.json", result)


if __name__ == "__main__": cli(main)

