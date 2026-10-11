"""Bounded real GL frames for both native input-comparison implementations."""
import argparse
import csv
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, capture, clean_environment, cli, file_path, load_json, new_directory, option, read_log, require, sha256, snapshot_identity, utc_now, workspace_path, write_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "OutputDirectory", required=True)
    option(parser, "GlfwExecutable", default=str(ROOT / "out/m3-t2/probe/input-comparison-glfw-release-001/Release/SymoCraftGlfwInputComparison.exe"))
    option(parser, "SdlExecutable", default=str(ROOT / "out/m3-t2/probe/input-comparison-sdl-release-002/SymoCraftSdl3InputComparison.exe"))
    args = parser.parse_args()
    executables = {"GLFW": file_path(args.GlfwExecutable), "SDL3": file_path(args.SdlExecutable)}
    output = new_directory(workspace_path(args.OutputDirectory, ROOT / "out"))
    cases = [("glfw-normal", "GLFW", "normal", "1"), ("sdl-normal", "SDL3", "normal", "1"), ("glfw-lock", "GLFW", "lock", "1"),
             ("sdl-lock-system", "SDL3", "lock", "1"), ("sdl-lock-unscaled", "SDL3", "lock", "0"), ("glfw-hidden", "GLFW", "hidden", "1"), ("sdl-hidden", "SDL3", "hidden", "1")]
    results = []
    summary = {"created_utc": utc_now(), "kind": "bounded real GL rendering, not human input validation", "passed": 0, "cases": results}
    try:
        for name, backend, cursor, scale in cases:
            exe = executables[backend]
            directory = output / name
            record = capture(exe, ["--frames", "12", "--cursor", cursor, "--system-scale", scale, "--output", directory], ROOT,
                             output / (name + ".stdout.log"), output / (name + ".stderr.log"), 30, clean_environment())
            require(not record["timed_out"] and record["exit_code"] == 0 and not read_log(output / (name + ".stderr.log")).strip(), f"{name} failed")
            report = load_json(directory / "session.json")
            require(report["normal_exit"] and report["rendered_frames"] == 12 and report["recorded_frames"] == 12, "Expected 12 real frames.")
            with (directory / "frames.csv").open(encoding="utf-8-sig", newline="") as stream:
                require(len(list(csv.DictReader(stream))) == 12, "CSV must have 12 rows.")
            frame = file_path(directory / "first-frame.bmp")
            require(frame.stat().st_size >= 1024, "Missing usable rendered frame.")
            results.append({"name": name, "passed": True, "executable": str(exe), "executable_sha256": sha256(exe), "session": report, "frame_sha256": sha256(frame)})
        summary["source_inputs"] = snapshot_identity(["test/experimental/input-comparison", "scripts/verification_common.py", "scripts/scripts_common.py"])
        dependencies = ["test/experimental/sdl3/CMakeLists.txt", "test/experimental/glfw-baseline/CMakeLists.txt", "test/experimental/sdl3/input_candidate.h"]
        dependencies += ["out/m3-t2/build/glfw-release/" + name for name in ["game/modules/" + module + "/symocraft_" + module + ".lib" for module in ("foundation", "ecs", "simulation", "world", "platform")] + ["symocraft_yaml.lib", "vendor/glfw/src/glfw3.lib"]]
        summary["dependency_inputs"] = snapshot_identity(dependencies)
        summary["passed"] = len(results)
    except (ValueError, OSError) as error:
        summary["failure"] = str(error)
        raise
    finally:
        write_json(output / "result.json", summary)


if __name__ == "__main__":
    cli(main)
