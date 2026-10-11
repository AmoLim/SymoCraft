"""Interactive observation launchers; never assign human acceptance automatically."""
import argparse
from datetime import datetime
import os
from pathlib import Path
import subprocess
import uuid
from verification_common import ROOT, assert_hash, clean_environment, file_path, load_json, new_directory, option, platform_imports, require, sha256, utc_now, verify_payload, workspace_path, write_json


def run(kind, source):
    parser = argparse.ArgumentParser(description=__doc__)
    if kind == "comparison":
        option(parser, "Backend", choices=["Both", "GLFW", "SDL3"], default="Both")
        option(parser, "SystemScale", choices=["0", "1"], default="1")
        option(parser, "GlfwExecutable", default=str(ROOT / "out/m3-t2/probe/input-comparison-glfw-release-001/Release/SymoCraftGlfwInputComparison.exe"))
        option(parser, "SdlExecutable", default=str(ROOT / "out/m3-t2/probe/input-comparison-sdl-release-002/SymoCraftSdl3InputComparison.exe"))
        option(parser, "TimeoutSeconds", type=int, default=3600)
    else:
        option(parser, "OutputDirectory", required=True)
        option(parser, "TimeoutSeconds", type=int, default=3600)
        if kind == "production":
            option(parser, "Executable", required=True)
    args = parser.parse_args()
    require(args.TimeoutSeconds > 0, "TimeoutSeconds must be positive.")
    if kind == "comparison":
        output = new_directory(ROOT / "out/m3-t2/input-comparison" / ("manual-" + datetime.now().strftime("%Y%m%d-%H%M%S") + "-" + uuid.uuid4().hex[:8]))
        print("F1 cursor mode; F2 view reset; F3 input reset; Escape closes. Evidence: " + str(output))
        for name in (["GLFW", "SDL3"] if args.Backend == "Both" else [args.Backend]):
            exe = file_path(args.GlfwExecutable if name == "GLFW" else args.SdlExecutable)
            digest = sha256(exe)
            write_json(output / (name + "-identity.json"), {"created_utc": utc_now(), "backend": name, "executable": str(exe), "executable_sha256": digest,
                       "system_scale_requested": args.SystemScale, "manual_result": "not assessed automatically"})
            subprocess.run([str(exe), "--output", str(output / name), "--system-scale", args.SystemScale], cwd=ROOT, env=clean_environment(), check=True, timeout=args.TimeoutSeconds)
            assert_hash(exe, digest)
        return
    if kind == "package":
        package = Path(source).resolve()
        identity_path = package / "package-identity.json"
        manifest = load_json(identity_path)
        verify_payload(package, manifest["files"])
        exe = file_path(package / "SymoCraftPlatformInputTests.exe")
        document = {"machine": os.environ.get("COMPUTERNAME"), "package_identity_sha256": sha256(identity_path),
                    "input_executable_sha256": sha256(exe), "game_executable_sha256": sha256(package / "SymoCraft.exe"),
                    "production_provenance": manifest["production_imports"], "scope": "Portable candidate bytes verified; original build paths are provenance only. Physical observations require separate human acceptance."}
        output = new_directory(args.OutputDirectory)
    else:
        exe = file_path(args.Executable)
        imports = platform_imports(exe, source, ("input_tests.cpp", "input_test_events.h"))
        document = {"executable": str(exe), "executable_sha256": sha256(exe), "production_imports": imports,
                    "input_probe_source_sha256": sha256(Path(source) / "input_tests.cpp"), "scope": "Actual production Window physical input observations, not automatic human acceptance."}
        output = new_directory(workspace_path(args.OutputDirectory, ROOT / "out"))
    document.update(started_utc=utc_now(), human_acceptance_passed=False)
    write_json(output / "manual-session-identity.json", document)
    print("Click the color window. F5 Reset; F6 Normal/Hidden/Lock; F7 second Capture. Test Escape last.")
    exit_code, error = None, None
    try:
        result = subprocess.run([str(exe), "--case", "hardware-manual", "--output-directory", str(output)], env=clean_environment(), timeout=args.TimeoutSeconds)
        exit_code = result.returncode
    except (OSError, subprocess.SubprocessError) as failure:
        error = str(failure)
        raise
    finally:
        write_json(output / "manual-launch-result.json", {"completed_utc": utc_now(), "exit_code": exit_code, "error": error, "human_acceptance_passed": False})
    require(exit_code == 0, f"Manual observation process failed; see {output}")
