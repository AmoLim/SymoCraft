"""Finite production drawing/window diagnostics, never a performance budget run."""
import argparse
from collections import Counter
from pathlib import Path
import re
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import (ROOT, capture, clean_environment, cli, evidence_sources, identity,
                                 load_json, new_directory, read_log, require, utc_now, workspace_path, write_json)

CALL = re.compile(r"^(vk\w+)\([^\r\n]*", re.M)
QUEUE_CALLS = ("vkAcquireNextImageKHR", "vkQueueSubmit", "vkQueuePresentKHR", "vkCreateSwapchainKHR")
LAYER_NOTICE = '[renderer] Vulkan validation: env var \'VK_INSTANCE_LAYERS\' defined and adding layers "VK_LAYER_LUNARG_api_dump"'


def interval(log, begin, end):
    require(log.count(begin) == 1 and log.count(end) == 1, "Missing/duplicate observation markers")
    start, finish = log.index(begin) + len(begin), log.index(end)
    require(start < finish, "Reversed observation markers")
    return log[start:finish]


def native_trace(log, mode):
    calls = Counter(CALL.findall(log))
    require(all(calls[x] > 0 for x in QUEUE_CALLS), "API dump did not observe actual presentation")
    if mode == "world-window":
        paused = Counter(CALL.findall(interval(log, "BATCH2_MARK_PAUSE_BEGIN", "BATCH2_MARK_PAUSE_END")))
        require(all(paused[x] == 0 for x in QUEUE_CALLS), "Suspension issued native presentation work")
        for create, destroy in (("vkCreateInstance", "vkDestroyInstance"), ("vkCreateDevice", "vkDestroyDevice"),
                                ("vkCreateWin32SurfaceKHR", "vkDestroySurfaceKHR")):
            require(calls[create] == calls[destroy] == 1, "Window/surface/device was recreated or not cleaned")
        require(calls["vkCreateSwapchainKHR"] == calls["vkDestroySwapchainKHR"] >= 6, "Size generations did not retire")
    else:
        invalid = Counter(CALL.findall(interval(log, "BATCH2_MARK_INVALID_FRAME_BEGIN", "BATCH2_MARK_INVALID_FRAME_END")))
        require(not invalid, "Invalid CPU frame entered a native API")
        pending = Counter(CALL.findall(interval(log, "BATCH2_MARK_PENDING_RESIZE_BEGIN", "BATCH2_MARK_PENDING_RESIZE_END")))
        require(pending["vkGetPhysicalDeviceSurfaceCapabilitiesKHR"] > 0 and
                not any(pending[x] for x in (*QUEUE_CALLS, "vkDestroySwapchainKHR")), "Pending resize submitted work or retired the accepted generation")
    return {"calls": dict(calls), "native_out_of_date": len(re.findall(r"returns VkResult VK_ERROR_OUT_OF_DATE_KHR", log)),
            "native_suboptimal": len(re.findall(r"returns VkResult VK_SUBOPTIMAL_KHR", log))}


def diagnostics(errors, traced):
    messages = [line for line in errors.splitlines() if "[renderer] Vulkan validation:" in line]
    require(all(traced and line == LAYER_NOTICE for line in messages), "Unexpected Vulkan validation diagnostic")
    totals = re.findall(r"validation_warnings=(\d+); validation_errors=(\d+); native_retired=true", errors)
    require(totals and all(int(e) == 0 for _, e in totals), "Vulkan errors or missing successful retirement")
    require(sum(int(w) for w, _ in totals) == len(messages), "Unaccounted Vulkan warnings")
    return {"warnings": sum(int(w) for w, _ in totals), "errors": 0, "observer_environment_notices": len(messages)}


def validate_report(report, directory, backend, mode, samples):
    require(report.get("success") is True and report.get("schema") == "symocraft.renderer-draw-window.v1"
            and (report["backend"], report["mode"], report["samples"]) == (backend, mode, samples), "Native report failed or mismatched")
    captures = report["captures"]
    if backend == "vulkan" and mode == "drawing":
        require(report["pending_resize_preserved_info"] is True, "Pending resize replaced accepted info")
    require(len(captures) == (21 if mode == "drawing" else 29), "Missing drawing/world captures")
    require(len({c["name"] for c in captures}) == len(captures), "Duplicate capture names")
    for item in captures:
        path = directory / (item["name"] + ".png")
        data = path.read_bytes()
        require(data[:8] == b"\x89PNG\r\n\x1a\n" and struct.unpack(">II", data[16:24]) == (item["width"], item["height"]), "PNG extent/header mismatch")
    if mode == "world-window":
        require(report["chunks"] == 441 and report["initial_upload_calls"] == 361, "Reduced world workload")
        require(report["pause_seconds"] >= 10 and 50 <= report["pause_steps"] <= 150, "Minimization too short or busy-looped")
        require(report["no_performance_session"] and report["synthetic_native_key_after_restore"], "Missing ordinary adapter/input path")
        edits = report["edits"]
        require(len(edits) == 32 and [r["index"] for r in edits] == list(range(32)), "Incomplete forward/reverse edits")
        require(all(r["upload_calls"] == r["changed_records"] > 0 for r in edits), "Uploaded unrelated or unchanged chunks")
        by_name = {c["name"]: c for c in captures}
        for name in ("spawn", "positive-x", "negative-x", "four-chunk", "wall-corner", "low-ceiling", "single-block"):
            require(by_name["initial-"+name]["rgba_fnv1a64"] == by_name["restored-"+name]["rgba_fnv1a64"], "Reverse edit pixels differ")
        require(by_name["initial-four-chunk"]["rgba_fnv1a64"] == by_name["window-restored"]["rgba_fnv1a64"], "Minimize recovery pixels differ")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", required=True)
    parser.add_argument("--output-directory", required=True)
    parser.add_argument("--backends", nargs="+", choices=("vulkan", "d3d12"), default=["vulkan", "d3d12"])
    parser.add_argument("--modes", nargs="+", choices=("drawing", "world-window"), default=["drawing", "world-window"])
    parser.add_argument("--samples", nargs="+", type=int, choices=(1, 4), default=[1, 4])
    parser.add_argument("--api-dump", action="store_true", help="Supplemental Vulkan observation; preserve its environment warnings")
    args = parser.parse_args()
    build = workspace_path(args.build_directory, ROOT / "out")
    output = new_directory(workspace_path(args.output_directory, ROOT / "out"))
    exe = build / "bin/renderer_draw_window_probe.exe"
    sources = lambda: evidence_sources([ROOT/p for p in ("game", "cmake", "assets/shaders/hlsl", "test/integration/renderer")], [ROOT/"CMakeLists.txt", ROOT/"test/integration/CMakeLists.txt"])
    original = sources()
    write_json(output / "source-inputs.json", original)
    result = {"schema": "symocraft.r4.draw-window-run.v1", "started_utc": utc_now(), "status": "running",
              "formal_visual_or_performance_acceptance": False, "faults_injected": False, "human_gameplay": False,
              "input_identities": [identity(p) for p in (exe, build/"CMakeCache.txt", build/"build.ninja")], "cases": []}
    try:
        for backend in args.backends:
            for mode in args.modes:
                for samples in args.samples:
                    name = f"{backend}-{mode}-{samples}x"
                    case = {"name": name, "status": "running"}; result["cases"].append(case)
                    env = clean_environment()
                    # Keep trace settings process-local, and distinguish observer notices from resource warnings.
                    for key in ("VK_INSTANCE_LAYERS", "VK_APIDUMP_DETAILED", "VK_APIDUMP_FLUSH"):
                        env.pop(key, None)
                    traced = args.api_dump and backend == "vulkan"
                    if traced:
                        env.update(VK_INSTANCE_LAYERS="VK_LAYER_LUNARG_api_dump", VK_APIDUMP_DETAILED="false", VK_APIDUMP_FLUSH="true")
                    stdout, stderr = output/(name+".stdout.log"), output/(name+".stderr.log")
                    command = [backend, mode, output/name, samples]
                    case["command"] = list(map(str, [exe, *command]))
                    case["observation_environment"] = {key: env[key] for key in ("VK_INSTANCE_LAYERS", "VK_APIDUMP_DETAILED", "VK_APIDUMP_FLUSH") if key in env}
                    case["process"] = capture(exe, command, output, stdout, stderr, 180, env)
                    write_json(output/"result.json", result)
                    require(case["process"]["exit_code"] == 0 and not case["process"]["timed_out"], f"Native case failed: {name}")
                    report = load_json(output/name/"report.json")
                    validate_report(report, output/name, backend, mode, samples)
                    errors, log = read_log(stderr), read_log(stdout)
                    require("terminal retention" not in errors and "bounded cleanup failure" not in errors, "Normal shutdown was not successful")
                    if backend == "vulkan":
                        case["validation"] = diagnostics(errors, traced)
                        if traced: case["native_trace"] = native_trace(log, mode)
                    if mode == "world-window":
                        expected = "Vulkan" if backend == "vulkan" else "D3D12"
                        require(f"backend={expected}; device=NVIDIA GeForce RTX 5070 Ti" in log
                                and f"samples={samples}; present=Fifo; validation=1" in log, "Actual game adapter configuration differs")
                    require(sources() == original, "Source inputs changed during diagnostics")
                    case["report"] = report
                    case["status"] = "passed"
                    case["artifacts"] = [identity(p, output) for p in sorted((output/name).iterdir()) if p.is_file()]
                    write_json(output/"result.json", result)
                    print(name, "passed", flush=True)
        result["status"] = "passed"
    except BaseException as error:
        result["status"] = "failed"; result["error"] = str(error); raise
    finally:
        result["finished_utc"] = utc_now()
        write_json(output/"result.json", result)


if __name__ == "__main__":
    cli(main)
