"""Short real complete-world workloads, not manual gameplay acceptance."""
import argparse
from pathlib import Path
import sys
import re
import uuid
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, assert_hash, capture, clean_environment, cli, evidence_sources, file_path, files, identity, load_json, new_directory, option, read_log, require, utc_now, workspace_path, write_json
from scripts_common import integer


def validate_run(stdout, stderr, backend, workload):
    """Check real console facts; pose/edit correctness lives in the native probes."""
    expected = "D3D12" if backend == "d3d12" else "OpenGL"
    match = re.search(r"\[runtime\] loop finished; rendered_frames=(\d+);", stdout)
    require(match and int(match[1]) > 0, "No actual rendered frames.")
    require("[runtime] ready; chunks=441" in stdout, "Complete fixed world was not ready.")
    require(f"[scenario] completed; name={workload};" in stdout, "Scenario did not complete.")
    checkpoint = "spawn" if workload == "walk" else "four-chunk"
    require(f"[scenario] initial; checkpoint={checkpoint}" in stdout, "Wrong initial checkpoint.")
    require(f"[renderer] backend={expected};" in stdout, "Wrong actual backend.")
    require("[runtime] shutdown complete; exit_code=0" in stdout, "Normal cleanup missing.")
    require("[runtime] fatal:" not in stderr and "Debug Layer warning/error" not in stderr
            and "[performance]" not in stdout, "Run failed or used the retired collector.")
    return {"backend": expected, "rendered_frames": int(match[1]), "checkpoint": checkpoint,
            "scenario_completed": True, "performance_collector": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "BuildDirectory", required=True)
    option(parser, "OutputDirectory", required=True)
    option(parser, "IncludeOpenGLComparison", action="store_true")
    option(parser, "ProcessTimeoutSeconds", type=integer(30, 300), default=180)
    args = parser.parse_args()
    build = workspace_path(args.BuildDirectory, ROOT / "out")
    exe = file_path(build / "bin/SymoCraft.exe")
    inputs = [identity(p) for p in [exe, build / "CMakeCache.txt", *files(build / "bin/assets")]]
    sources = evidence_sources([ROOT / p for p in ("game", "cmake", "test/integration/renderer", "assets/shaders")], [ROOT / "CMakeLists.txt"])
    output = new_directory(workspace_path(args.OutputDirectory, ROOT / "out"))
    workdir = new_directory(output / "unrelated-cwd")
    manifest = {"schema": "symocraft.t3.r2-step3-game-verification", "version": 2, "run_id": str(uuid.uuid4()), "started_utc": utc_now(), "finished_utc": None,
                "source_identity": sources, "input_identity": inputs, "process_timeout_seconds": args.ProcessTimeoutSeconds, "cases": [], "success": False,
                "fixed_complete_world_short_workloads": True, "manual_gameplay_acceptance": False, "fifteen_minute_session": False,
                "actual_device_lost_exercised": False, "actual_in_flight_timeout_exercised": False, "installed_package_acceptance": False,
                "r3_acceptance": False, "opengl_default_changed": False}
    def run(name, arguments, expected):
        row = {"name": name, "arguments": list(map(str, arguments)), "expected_exit": expected, "success": False, "observation": None,
               "stdout": str(output / (name + ".stdout.log")), "stderr": str(output / (name + ".stderr.log"))}
        manifest["cases"].append(row)
        row.update(capture(exe, arguments, workdir, row["stdout"], row["stderr"], args.ProcessTimeoutSeconds, clean_environment()))
        row["success"] = not row["timed_out"] and row["exit_code"] == expected
        return row
    row = None
    try:
        for name, arguments, expected in [("d3d12-assets", ["--renderer", "d3d12", "--check-assets"], 0), ("unavailable-vulkan", ["--renderer", "vulkan", "--smoke-frames", "1"], 3), ("invalid-backend", ["--renderer", "unknown"], 64)]:
            row = run(name, arguments, expected)
            if name == "unavailable-vulkan":
                errors = read_log(row["stderr"])
                row["success"] &= "Vulkan" in errors and "SelectBackend" in errors and "Monitor size:" not in read_log(row["stdout"])
            write_json(output / "verification.json", manifest)
        for backend in ["d3d12"] + (["opengl"] if args.IncludeOpenGLComparison else []):
            for workload, duration in (("static", "2"), ("walk", "3"), ("edit", "9")):
                name = backend + "-" + workload
                arguments = ["--renderer", backend, "--profile-scenario", workload, "--scenario-seconds", duration,
                             "--seed", "424242", "--width", "1920", "--height", "1080", "--vsync", "0"] + (["--msaa", "4", "--validation", "1"] if backend == "d3d12" else [])
                row = run(name, arguments, 0)
                if row["success"]:
                    row["observation"] = validate_run(read_log(row["stdout"]), read_log(row["stderr"]), backend, workload)
                require(not list(workdir.iterdir()), "Game created unexpected files in the unrelated working directory.")
                write_json(output / "verification.json", manifest)
        for entry in sources + inputs:
            assert_hash(entry["path"], entry["sha256"])
        manifest["success"] = all(r["success"] for r in manifest["cases"])
    except (ValueError, OSError, KeyError) as error:
        if row:
            row["success"] = False
        manifest["failure"] = str(error)
        raise
    finally:
        manifest["finished_utc"] = utc_now()
        manifest["artifacts"] = [identity(p) for p in files(output) if p.name != "verification.json"]
        write_json(output / "verification.json", manifest)
    require(manifest["success"], f"Real game verification cases failed; see {output}")


if __name__ == "__main__":
    cli(main)
