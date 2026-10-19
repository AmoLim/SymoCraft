"""Bounded production Vulkan in-flight/fault diagnostics; never performance acceptance."""
import argparse
from pathlib import Path
import shutil
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3]/"scripts"))
from verification_common import (ROOT, capture, clean_environment, cli, evidence_sources, identity,
                                 load_json, new_directory, read_log, require, utc_now, workspace_path, write_json)

MODES = ("lifetime", "allocation", "host-allocation", "pipeline", "submit", "device-lost",
         "texture-timeout", "frame-timeout", "game-normal", "game-allocation", "game-submit",
         "game-device-lost", "game-timeout")
RETAINED = {"submit", "device-lost", "texture-timeout", "frame-timeout", "game-submit", "game-device-lost", "game-timeout"}
TIMEOUTS = {"texture-timeout", "frame-timeout", "game-timeout"}


def validate(mode, report, errors, log):
    require(report.get("schema") == "symocraft.vulkan-production-lifetime-fault.v1" and
            report.get("mode") == mode and report.get("success") is True, "Incomplete/mismatched native report")
    require(report.get("spontaneous_hardware_device_loss_or_real_oom") is False, "Injection relabeled as spontaneous hardware fault")
    require(report.get("validation_warnings") == report.get("validation_errors") == 0 and
            "[renderer] Vulkan validation:" not in errors, "Native diagnostic warning/error")
    require(report["quarantined"] == (mode in RETAINED) and
            report["native_destroyed"] == report["loader_released"] == (mode not in RETAINED), "Owner/loader outcome mismatch")
    if mode in RETAINED:
        require(report["live_bytes"] > 0 and "terminal retention" in errors and "native_retired=true" not in errors,
                "Unproven owner was falsely reclaimed")
    else:
        require(report["live_bytes"] == 0 and report["buffers_created"] == report["buffers_released"] and
                report["images_created"] == report["images_released"] and "native_retired=true" in errors,
                "Normal/partial native cleanup incomplete")
    if mode in TIMEOUTS:
        require(report.get("actual_in_flight") is True and report.get("fence_at_timeout") == 1 and
                report.get("timeline_at_timeout") == 0 and report.get("owned_work_completed_after_failure") is True and
                report["isolated_timeline_gate_feature"] is True, "Timeout lacks actual pending native work")
    if mode == "lifetime":
        events = report.get("events", [])
        require(len(events) == 3 and [r["step"] for r in events] == [0, 1, 2] and report.get("capacity_creates") == 144,
                "Missing update/empty/delete/capacity evidence")
        require(all(r["fence_before_and_after_mutation"] == 1 and r["timeline_before_and_after_mutation"] == 0 and
                    r["old_mesh_retained"] and r["old_texture_retained"] and r["old_mesh_retired"] and
                    r["diagnostic_deferred_present"] for r in events) and events[-1]["deleted_texture_retired"],
                "Lifetime is not a real gated retained-then-retired version")
    if mode in {"allocation", "host-allocation"}:
        require(report.get("old_contents_preserved") is True and report.get("failed_create_rolled_back") is True,
                "Failed update/Create lost accepted data or leaked its candidate")
    if mode in {"submit", "device-lost", "texture-timeout", "frame-timeout"}:
        expected = (7, "-13") if mode == "submit" else (6, "-4") if mode == "device-lost" else (8, "2")
        require((report.get("first_category"), report.get("first_native_code")) == expected and
                report.get("failed_rejections") == 4 and report.get("repeated_shutdown_not_success") is True and
                report.get("no_further_submit_or_present") is True, "Fatal category/Failed lifecycle/no-submit evidence mismatch")
    if "device-lost" in mode:
        require(report["last_actual_present_result"] == 0 and report["fault_kind"] == "injected DeviceLost after actual native Present",
                "DeviceLost injection was not distinguished from actual Present")
    if mode.startswith("game-"):
        require(report.get("game_exit_code") == (0 if mode == "game-normal" else 3) and report.get("repeat_application_free") is True and
                report.get("platform_reinitialized") is True, "Production game did not attempt/complete aggregate cleanup")
        if mode in RETAINED:
            require(report.get("actual_loader_mapped_after_window_free") is True and "shutdown failed" in errors,
                    "Game cleanup falsely reported success or lost retained loader")
        if mode in {"game-normal", "game-device-lost"}:
            require("[runtime] ready; chunks=441;" in log and "backend=Vulkan; device=NVIDIA GeForce RTX 5070 Ti" in log,
                    "Production gameplay workload/device identity missing")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", required=True)
    parser.add_argument("--output-directory", required=True)
    parser.add_argument("--modes", nargs="+", choices=MODES, default=MODES)
    parser.add_argument("--asset-failures", action="store_true")
    args = parser.parse_args()
    build = workspace_path(args.build_directory, ROOT/"out")
    output = new_directory(workspace_path(args.output_directory, ROOT/"out"))
    exe = build/"bin/renderer_vulkan_lifetime_fault_probe.exe"
    sources = lambda: evidence_sources([ROOT/p for p in ("game", "cmake", "test/integration/renderer", "assets/shaders/hlsl")],
                                      [ROOT/"CMakeLists.txt", ROOT/"test/integration/CMakeLists.txt"])
    original = sources()
    write_json(output/"source-inputs.json", original)
    assets = [identity(p) for p in sorted((build/"bin/assets").rglob("*")) if p.is_file()]
    result = {"status": "running", "started_utc": utc_now(), "formal_visual_or_performance_acceptance": False,
              "real_hardware_loss_or_oom": False, "cases": [], "asset_cases": [],
              "inputs": [identity(p) for p in (exe, build/"bin/SymoCraft.exe", build/"CMakeCache.txt", build/"build.ninja")],
              "libraries": [identity(p) for p in (build/"game").rglob("*.lib")], "assets": assets}
    env = clean_environment()
    for key in ("VK_INSTANCE_LAYERS", "VK_APIDUMP_DETAILED", "VK_APIDUMP_FLUSH"):
        env.pop(key, None)
    try:
        for mode in args.modes:
            command = [mode, output/mode]
            case = {"mode": mode, "command": list(map(str, [exe, *command])), "status": "running"}
            result["cases"].append(case)
            case["process"] = capture(exe, command, output, output/(mode+".stdout.log"), output/(mode+".stderr.log"), 30, env)
            write_json(output/"result.json", result)
            require(case["process"]["exit_code"] == 0 and not case["process"]["timed_out"], "Native case failed: "+mode)
            report = load_json(output/mode/"report.json")
            validate(mode, report, read_log(output/(mode+".stderr.log")), read_log(output/(mode+".stdout.log")))
            case["report"] = report
            case["artifacts"] = [identity(p, output) for p in sorted((output/mode).iterdir()) if p.is_file()]
            case["status"] = "passed"
            write_json(output/"result.json", result)
            print(mode, "passed", flush=True)
        if args.asset_failures:
            for mode, relative, changed, expected, text in (
                ("missing-spv", "shaders/vulkan/blocks.vs.spv", None, 2, "Vulkan"),
                ("bad-spv", "shaders/vulkan/blocks.vs.spv", b"bad!", 3, "Vulkan"),
                ("bad-png", "textures/texture_atlas.png", b"invalid png", 3, "texture_atlas.png"),
                ("bad-yaml", "configs/blockFormats.yaml", b"blocks: [\n", 3, "block configuration"),
                ("invalid-backend", None, None, 64, "renderer")):
                package = new_directory(output/("isolated "+mode))
                shutil.copy2(build/"bin/SymoCraft.exe", package/"SymoCraft.exe")
                for p in (build/"bin").glob("*.dll"): shutil.copy2(p, package/p.name)
                shutil.copytree(build/"bin/assets", package/"assets")
                if relative:
                    target = package/"assets"/relative
                    if changed is None: target.unlink()
                    else: target.write_bytes(changed)
                command = ["--renderer", "invalid" if mode == "invalid-backend" else "vulkan", "--validation", "1", "--smoke-frames", "2"]
                case = {"mode": mode, "command": [str(package/"SymoCraft.exe"), *command]}
                case["process"] = capture(package/"SymoCraft.exe", command, output, package/"stdout.log", package/"stderr.log", 30, env)
                result["asset_cases"].append(case)
                write_json(output/"result.json", result)
                errors = read_log(package/"stderr.log")
                require(case["process"]["exit_code"] == expected and not case["process"]["timed_out"] and text.lower() in errors.lower(),
                        "Asset/game failure classification mismatch: "+mode)
                require("[renderer] Vulkan validation:" not in errors, "Asset failure triggered unresolved validation")
                case["status"] = "passed"
                print(mode, "passed", flush=True)
        require(sources() == original and all(identity(p["path"]) == p for p in assets), "Source/original assets changed")
        result["status"] = "passed"
    except BaseException as error:
        result["status"] = "failed"; result["error"] = str(error); raise
    finally:
        result["finished_utc"] = utc_now()
        write_json(output/"result.json", result)


if __name__ == "__main__":
    cli(main)
