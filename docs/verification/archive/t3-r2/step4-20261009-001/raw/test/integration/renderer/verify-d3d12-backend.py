"""Verify real native backend reports and preserve source/input evidence."""
import argparse
from pathlib import Path
import platform
import shutil
import sys
import uuid
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, assert_hash, capture, clean_environment, cli, evidence_sources, file_path, files, identity, load_json, new_directory, option, require, utc_now, workspace_path, write_json
from scripts_common import integer

SHADERS = ("blocks.vs.dxil", "blocks.ps.dxil", "selection.vs.dxil", "selection.ps.dxil")


def validate_report(report, run_id, mode, device):
    require(report["schema"] == "symocraft.d3d12-backend-smoke.v1" and report["run_id"] == run_id and report["mode"] == mode and report["expected_device_name"] == device,
            "Verifier report identity differs from process request.")
    probe = report["json_emitter_probe"]
    require(len(probe["all_control_characters"]) == 32 and probe["quote_backslash"] == '"\\' and probe["null"] is None and
            probe["unicode_utf8"] == "\u00e9\u4e2d" and probe["nonfinite_rejected"] is True, "Structured JSON emitter did not round-trip.")
    require(isinstance(report["checks"], list) and report["checks"], "Missing native checks.")
    return [x for x in report["checks"] if x.get("passed") is not True]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("Executable", "BuildDirectory", "ShaderRoot", "OutputDirectory"):
        option(parser, name, required=True)
    option(parser, "Atlas", default=str(ROOT / "assets/textures/texture_atlas.png"))
    option(parser, "ExpectedDeviceName", default="NVIDIA GeForce RTX 5070 Ti")
    option(parser, "ProcessTimeoutSeconds", type=integer(15, 300), default=90)
    args = parser.parse_args()
    build = workspace_path(args.BuildDirectory, ROOT / "out")
    exe = file_path(workspace_path(args.Executable, ROOT / "out"))
    shaders = workspace_path(args.ShaderRoot, ROOT / "out")
    atlas = file_path(workspace_path(args.Atlas, ROOT))
    inputs = [identity(p) for p in [exe, atlas, build / "CMakeCache.txt", *[shaders / n for n in SHADERS]]]
    output = new_directory(workspace_path(args.OutputDirectory, ROOT / "out/m3-t3"))
    source = evidence_sources([ROOT / p for p in ("game/modules/renderer", "game/modules/platform", "game/modules/assets", "test/integration/renderer", "assets/shaders")],
                             [ROOT / p for p in ("CMakeLists.txt", "cmake/ModuleBoundaries.cmake", "cmake/RendererD3D12.cmake", "cmake/PublicHeaderTests.cmake", "test/integration/CheckRendererBoundary.cmake", "test/integration/CMakeLists.txt", "game/modules/foundation/include/symocraft/foundation/document.h", "game/modules/foundation/src/document.cpp", "game/modules/foundation/src/files.cpp", "game/modules/scene/include/symocraft/scene/mesh.h")])
    run_id = str(uuid.uuid4())
    summary = {"schema": "symocraft.d3d12-backend-verification.v1", "run_id": run_id, "started_utc": utc_now(), "finished_utc": None,
               "repository": str(ROOT), "build_directory": str(build), "output_directory": str(output), "process_timeout_seconds": args.ProcessTimeoutSeconds,
               "gpu_verification": True, "world_or_gameplay_verification": False, "production_package_acceptance": False, "d3d12_default_approved": False,
               "device_lost_exercised": False, "actual_in_flight_timeout_exercised": False, "expected_device_name": args.ExpectedDeviceName,
               "source_identity": source, "input_identity": inputs, "cases": [], "artifacts": [], "success": False,
               "host": {"machine_name": platform.node(), "os_version": platform.platform(), "process_architecture": platform.machine(), "python_version": platform.python_version()}}
    try:
        proof = new_directory(output / "build-identity")
        for name in ("CMakeCache.txt", "compile_commands.json", "renderer-targets.tsv", "production-sources.tsv", "production-dependencies.tsv"):
            if (build / name).exists():
                shutil.copy2(build / name, proof / name)
        missing = new_directory(output / "fixtures/missing-shaders")
        bad = new_directory(output / "fixtures/bad-shaders")
        for name in SHADERS:
            (bad / name).write_bytes(b"NOT_DXIL")
        cases = [("normal-samples-1", "normal", 1, shaders), ("normal-samples-4", "normal", 4, shaders), ("missing-shader", "missing-shader", 1, missing), ("bad-shader", "bad-shader", 1, bad)]
        for name, mode, samples, shader_root in cases:
            directory = output / name
            row = {"name": name, "mode": mode, "samples": samples, "requested_vsync": False, "started_utc": utc_now(), "finished_utc": None,
                   "timed_out": False, "exit_code": None, "success": False, "report": str(directory / "result.json"),
                   "stdout": str(output / (name + ".stdout.log")), "stderr": str(output / (name + ".stderr.log")), "failure": None}
            arguments = ["--output-dir", str(directory), "--shader-root", str(shader_root), "--recovery-shader-root", str(shaders), "--atlas", str(atlas),
                         "--run-id", run_id, "--expected-device-name", args.ExpectedDeviceName, "--mode", mode, "--samples", str(samples), "--vsync", "false"]
            row["arguments"] = arguments
            try:
                row.update(capture(exe, arguments, exe.parent, row["stdout"], row["stderr"], args.ProcessTimeoutSeconds, clean_environment()))
                report = load_json(row["report"])
                row["failed_checks"] = validate_report(report, run_id, mode, args.ExpectedDeviceName)
                row["check_count"] = len(report["checks"])
                row["success"] = not row["timed_out"] and row["exit_code"] == 0 and report["success"] is True and not row["failed_checks"]
                row["failure"] = report.get("failure")
            except (ValueError, OSError, KeyError, TypeError) as error:
                row["failure"] = str(error)
            row["finished_utc"] = utc_now()
            summary["cases"].append(row)
            write_json(output / "verification.json", summary)
        for entry in source + inputs:
            assert_hash(entry["path"], entry["sha256"])
        summary["success"] = len(summary["cases"]) == 4 and all(r["success"] for r in summary["cases"])
    except (ValueError, OSError) as error:
        summary["failure"] = str(error)
    finally:
        summary["finished_utc"] = utc_now()
        summary["artifacts"] = [identity(p) for p in files(output) if p.name != "verification.json"]
        write_json(output / "verification.json", summary)
    require(summary["success"], f"Native D3D12 verifier failed; retained evidence: {output}")


if __name__ == "__main__":
    cli(main)
