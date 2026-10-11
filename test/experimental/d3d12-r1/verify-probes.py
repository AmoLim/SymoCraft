"""Real discrete-adapter R1 evidence verification, including portable packages."""
import argparse
import os
from pathlib import Path
import platform
import re
import struct
import sys
if (Path(__file__).parent / "scripts_common.py").exists():
    sys.path.insert(0, str(Path(__file__).parent))
else:
    sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, assert_hash, capture, clean_environment, cli, file_path, identity, load_json, new_directory, option, require, sha256, utc_now, verify_payload, write_json
from scripts_common import integer


def report_checks(r, injected):
    checks = {"backend": r.get("backend") == "D3D12", "hardware_adapter": r.get("software_adapter") is False,
              "current_scope_rtx": r.get("adapter") == "NVIDIA GeForce RTX 5070 Ti" and r.get("adapter_vendor_id") == 0x10de and r.get("adapter_device_id") == 0x2c05,
              "actual_device_identity": r.get("device_luid_matches_dxgi_adapter") is True and bool(r.get("adapter_luid")),
              "actual_driver_and_capabilities": bool(re.fullmatch(r"\d+\.\d+\.\d+\.\d+", r.get("driver", ""))) and r.get("feature_level") == "12_0 (requested and created)" and r.get("shader_model") == "6_0 (explicit CheckFeatureSupport)",
              "present_parameters": r.get("present_sync_interval") == 0 and r.get("present_flags") == 0,
              "debug_layer": r.get("debug_layer_enabled") is True, "clean_debug_errors": r.get("debug_error_count") == 0, "clean_debug_warnings": r.get("debug_warning_count") == 0}
    def presentation(prefix, width, height):
        return width is not None and height is not None and width > 0 and height > 0 and all(r.get(prefix + key) == value for key, value in
               {"width": width, "height": height, "format": 28, "buffer_count": 3, "sample_count": 1, "sample_quality": 0, "swap_effect": 4, "flags": 0, "windowed": True}.items())
    checks["actual_initial_presentation"] = presentation("swapchain_initial_", r.get("swapchain_initial_width"), r.get("swapchain_initial_height"))
    if injected:
        checks.update(expected_failure=r.get("status") == "fail" and r.get("timeout_injected") is True,
                      original_diagnostic="injected GPU wait timeout" in r.get("error", ""), cleanup_completed=r.get("cleanup_completed") is True,
                      bounded_failure=r.get("elapsed_ms", float("inf")) < 10000)
    else:
        checks.update(shutdown_completed=r.get("shutdown_completed") is True, success=r.get("status") == "pass",
                      presented_frames=r.get("submitted_frames", 0) > 0, real_pixels=r.get("width", 0) > 0 and r.get("height", 0) > 0,
                      actual_restored_presentation=presentation("swapchain_restored_", r.get("width"), r.get("height")),
                      adapter_identity=bool(r.get("adapter") and r.get("adapter_luid")), capability_record=all(bool(r.get(k)) for k in ("driver", "feature_level", "shader_model")))
        checks.update({key: r.get(key + "_passed") is True for key in ("e01", "e02", "e03")})
    return checks


def verify_origin(build, imports, identities, portable=False):
    assert_hash(build / "SymoCraftD3D12R1Probe.exe", identities["executable_sha256"])
    assert_hash(build / "production-imports.json", identities["production_imports_sha256"])
    if not portable:
        assert_hash(Path(imports["production_build"]) / "CMakeCache.txt", imports["production_cache_sha256"])
        for entry in imports["imports"]:
            assert_hash(entry["archive"], entry["sha256"])
        require(imports.get("foundation_sources"), "Missing foundation source identities; rebuild dependencies and probe.")
        for entry in imports["platform_sources"] + imports["foundation_sources"] + identities["sources"]:
            assert_hash(entry["path"], entry["sha256"])
        assert_hash(imports["dxc"], imports["dxc_sha256"])
    for entry in identities["shaders"]:
        assert_hash(build / "shaders" / Path(entry["path"]).name if portable else entry["path"], entry["sha256"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "Executable", required=True)
    option(parser, "OutputDirectory", required=True)
    option(parser, "PackageManifest")
    option(parser, "RunTimeoutProbe", action="store_true")
    option(parser, "TimeoutSeconds", type=integer(5, 600), default=60)
    args = parser.parse_args()
    root, allowed, portable = ROOT, ROOT / "out", bool(args.PackageManifest)
    if portable:
        manifest_path = file_path(args.PackageManifest if Path(args.PackageManifest).exists() else Path(__file__).parent / args.PackageManifest)
        root = manifest_path.parent
        allowed = root / "evidence"
        package = load_json(manifest_path)
        require(package["schema_version"] == 1 and package["configuration"] == "Release" and package["required_debug_layer"] is True and package["software_adapter_allowed"] is False,
                "Unsupported or weakened portable package requirements.")
        verify_payload(root, package["payload"])
        exe = file_path(root / args.Executable)
        require(exe.parent == root, "Executable must belong to the verified package.")
        for name, key in (("build-identities.json", "build_identities_sha256"), ("production-imports.json", "production_imports_sha256")):
            assert_hash(root / name, package["origin_build"][key])
        assert_hash(exe, package["origin_build"]["executable_sha256"])
    else:
        exe = file_path(args.Executable)
    build = exe.parent
    imports, identities = load_json(build / "production-imports.json"), load_json(build / "build-identities.json")
    verify_origin(build, imports, identities, portable)
    path = (root / args.OutputDirectory).resolve()
    require(path != allowed and path.is_relative_to(allowed), "Evidence must be below workspace out or package evidence.")
    output = new_directory(path)
    # Optional host inventory is explicitly not selected-adapter evidence.
    machine = {"recorded_utc": utc_now(), "computer_name": platform.node(), "windows": [{"Version": platform.version(), "OSArchitecture": platform.machine()}],
               "cpu": [{"Name": platform.processor(), "NumberOfLogicalProcessors": os.cpu_count()}], "display_inventory": [],
               "metadata_errors": [{"class_name": "Win32_VideoController", "message": "CIM removed; selected device/driver/capabilities are measured by the native D3D12 fixture.", "error_id": "not_collected"}],
               "optional_metadata_is_not_gpu_proof": True, "display_inventory_is_not_selected_adapter_proof": True, "process_is_64_bit": struct.calcsize("P") == 8,
               "metadata_source": "Python platform; unavailable CIM fields remain unverified"}
    write_json(output / "machine.json", machine)
    results = []
    verification = {"executable": str(exe), "executable_sha256": sha256(exe), "production_imports": imports, "build_identities": identities,
                    "shaders": [identity(build / "shaders" / name) for name in ("probe-vs.cso", "probe-ps.cso")], "experiment_sources": identities["sources"],
                    "portable_package": portable, "sources_are_build_origin_records": portable, "machine_file": str(output / "machine.json"),
                    "passed": False, "freeze_authorized": False, "scope": "Isolated discrete-adapter R1; timeout/zero extent are contract injections, not driver-loss proof.",
                    "remaining": "T2 acceptance, R1 review and other matrix items remain separate; not gameplay or T3 acceptance.", "cases": results}
    try:
        for name, injected in [("hardware", False)] + ([("injected-timeout", True)] if args.RunTimeoutProbe else []):
            directory = new_directory(output / name)
            started = utc_now()
            arguments = ["--output", directory, "--shaders", build / "shaders", "--require-debug-layer"] + (["--inject-timeout"] if injected else [])
            record = {"case": name, "started_utc": started, "expected_exit_code": int(injected), "exit_code": None, "timed_out": False,
                      "timeout_seconds": args.TimeoutSeconds, "injected_test": injected, "debug_layer_required": True, "failure": None, "report": None, "passed": False,
                      "stdout": str(directory / "stdout.log"), "stderr": str(directory / "stderr.log")}
            checks = {}
            try:
                actual = capture(exe, arguments, directory, directory / "stdout.log", directory / "stderr.log", args.TimeoutSeconds, clean_environment())
                record.update(actual, elapsed_ms=actual["elapsed_seconds"] * 1000)
                checks = {"process_completed": not actual["timed_out"], "expected_exit": actual["exit_code"] == int(injected), "json_report": (directory / "result.json").exists()}
                report = load_json(directory / "result.json")
                record["report"] = report
                checks.update(report_checks(report, injected))
            except (ValueError, OSError, TypeError) as error:
                record["failure"] = str(error)
                checks["complete_schema"] = False
            screenshots = [identity(p) for p in sorted(directory.glob("*.png"))]
            if not injected:
                for screenshot in ("e01-linear.png", "e01-srgb.png", "e02-updated.png", "e02-empty.png", "e03-restored.png"):
                    checks["screenshot_" + screenshot] = sum(Path(p["path"]).name == screenshot and p["bytes"] > 0 for p in screenshots) == 1
            record.update(checks=checks, screenshots=screenshots, passed=not record["failure"] and bool(checks) and all(checks.values()))
            results.append(record)
        verify_origin(build, imports, identities, portable)
        if portable:
            verify_payload(root, package["payload"])
        verification["passed"] = bool(results) and all(r["passed"] for r in results)
    finally:
        write_json(output / "verification.json", verification)
    require(verification["passed"], f"Required R1 evidence failed/missing; see {output}")


if __name__ == "__main__":
    cli(main)
