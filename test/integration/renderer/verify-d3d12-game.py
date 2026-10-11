"""Short real complete-world workloads, not manual gameplay acceptance."""
import argparse
import csv
from pathlib import Path
import sys
import uuid
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, assert_hash, capture, clean_environment, cli, evidence_sources, file_path, files, identity, load_json, new_directory, option, read_log, require, utc_now, workspace_path, write_json
from scripts_common import integer


def validate_frames(frames, backend, workload):
    require(frames, "No actual frame records.")
    if backend == "d3d12":
        ids = [r["renderer_frame_id"] for r in frames]
        require(all(ids) and len(ids) == len(set(ids)), "Renderer frame IDs are not unique and explicit.")
        uploads = [r for r in frames if int(r["resource_upload_bytes"]) > 0]
        require(bool(uploads) == (workload == "edit"), "Real edits must update geometry; unchanged worlds must not reupload.")
        for row in frames:
            require(bool(row["gpu_draw_ms"]) != bool(row["gpu_missing_reason"]), "GPU duration/missing reason inconsistent.")
            require(int(row["upload_calls"]) == int(row["rebuilt_chunks"]), "Cache operations differ from published rebuilds.")
    if workload == "walk":
        require(len({r["x"] for r in frames}) >= 2, "Actual player did not move.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "BuildDirectory", required=True)
    option(parser, "OutputDirectory", required=True)
    option(parser, "IncludeOpenGLComparison", action="store_true")
    option(parser, "ProcessTimeoutSeconds", type=integer(30, 300), default=180)
    args = parser.parse_args()
    build = workspace_path(args.BuildDirectory, ROOT / "out/m3-t3")
    exe = file_path(build / "bin/SymoCraft.exe")
    probe = file_path(build / "test/integration/renderer_game_report_probe.exe")
    inputs = [identity(p) for p in [exe, probe, build / "CMakeCache.txt", *files(build / "bin/assets")]]
    sources = evidence_sources([ROOT / p for p in ("game", "cmake", "test/integration/renderer", "assets/shaders")], [ROOT / "CMakeLists.txt"])
    output = new_directory(workspace_path(args.OutputDirectory, ROOT / "out/m3-t3"))
    workdir = new_directory(output / "unrelated-cwd")
    manifest = {"schema": "symocraft.t3.r2-step3-game-verification", "version": 1, "run_id": str(uuid.uuid4()), "started_utc": utc_now(), "finished_utc": None,
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
                directory = output / name
                arguments = ["--renderer", backend, "--benchmark", workload, "--output", directory, "--seed", "424242", "--width", "1920", "--height", "1080",
                             "--vsync", "0", "--warmup-seconds", "0", "--sample-seconds", duration, "--focus-policy", "allow-unfocused"] + (["--msaa", "4", "--validation", "1"] if backend == "d3d12" else [])
                row = run(name, arguments, 0)
                if row["success"]:
                    expected = "D3D12" if backend == "d3d12" else "OpenGL"
                    actual = capture(probe, [directory, expected, workload], ROOT, output / (name + ".probe.json"), output / (name + ".probe.stderr.log"), 30)
                    row["success"] = not actual["timed_out"] and actual["exit_code"] == 0
                    if row["success"]:
                        row["observation"] = load_json(output / (name + ".probe.json"))
                        with (directory / "frames.csv").open(encoding="utf-8-sig", newline="") as stream:
                            frames = list(csv.DictReader(stream))
                        validate_frames(frames, backend, workload)
                        row["observation"]["frame_rows"] = len(frames)
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
