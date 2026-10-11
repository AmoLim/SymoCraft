"""Run a bounded game probe and retain logs and a structured result."""

import argparse
import json
import re

from scripts_common import cli, file_path, integer, new_directory, option, read_log, run_logged, write_json


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    option(result, "Executable", required=True)
    option(result, "OutputDirectory", required=True)
    option(result, "Frames", type=integer(0, 10000), default=120)
    option(result, "TimeoutSeconds", type=integer(10, 600), default=120)
    option(result, "ExpectedExitCode", type=int, choices=(0, 2, 3), default=0)
    option(result, "Seed", type=integer(0, 2**32 - 1))
    option(result, "Scene", choices=("terrain", "regression"))
    option(result, "Checkpoint", choices=("spawn", "positive-x", "negative-x", "four-chunk", "wall-corner", "low-ceiling", "single-block"))
    option(result, "TestEdits", action="store_true")
    return result


def probe(args):
    if args.Scene != "regression" and (args.Checkpoint or args.TestEdits):
        raise ValueError("Checkpoint and TestEdits require the regression scene.")
    executable = file_path(args.Executable)
    output = new_directory(args.OutputDirectory)
    arguments = []
    if args.Scene is not None:
        arguments += ["--scene", args.Scene]
    if args.Frames > 0:
        arguments += ["--smoke-frames", str(args.Frames)]
    if args.Seed is not None:
        arguments += ["--seed", str(args.Seed)]
    if args.Checkpoint:
        arguments += ["--checkpoint", args.Checkpoint]
    if args.TestEdits:
        arguments += ["--test-edits"]
    process = run_logged(executable, arguments, output, args.TimeoutSeconds)
    stdout = read_log(output / "stdout.log")
    stderr = read_log(output / "stderr.log")
    ready = "[runtime] ready;" in stdout
    shutdown = "[runtime] shutdown complete;" in stdout
    frame_match = re.search(r"\[runtime\] loop finished; rendered_frames=(\d+)", stdout)
    frames = int(frame_match[1]) if frame_match else None
    diagnostics = stderr.count("OpenGL diagnostic")
    passed = not process["timedOut"] and process["exitCode"] == args.ExpectedExitCode
    if args.ExpectedExitCode == 0:
        passed = passed and ready and shutdown and frames is not None and diagnostics == 0 and "[runtime] fatal:" not in stderr
        if args.Frames > 0:
            passed = passed and frames == args.Frames
    elif args.ExpectedExitCode == 3:
        passed = passed and not ready and shutdown
    result = {"executable": str(executable), "workingDirectory": str(output), "requestedFrames": args.Frames,
              "arguments": arguments, "renderedFrames": frames, "glDiagnosticCount": diagnostics,
              **process, "expectedExitCode": args.ExpectedExitCode, "ready": ready, "shutdown": shutdown,
              "passed": bool(passed)}
    write_json(output / "result.json", result)
    return result


def main():
    args = parser().parse_args()
    result = probe(args)
    print(json.dumps(result, indent=2))
    if not result["passed"]:
        raise RuntimeError(f"Runtime probe failed; see {result['workingDirectory']}")


if __name__ == "__main__":
    cli(main)
