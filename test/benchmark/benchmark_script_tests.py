"""Exercise the Python collector against the existing headless native fixture."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import shutil
import sys
import uuid


def verify(script, executable, parent):
    root = Path(parent).resolve() / uuid.uuid4().hex / "shell directory \u6d4b\u8bd5"
    root.mkdir(parents=True)
    copied_fixture = root / "fixture game \u6d4b\u8bd5.exe"
    shutil.copy2(executable, copied_fixture)
    executable = copied_fixture
    for name, seed, code, passed, repeats in (("success", 0, 0, True, 2), ("fast-success", 2, 0, True, 2),
                                              ("focus-failure", 4, 4, False, 3), ("runtime-failure", 3, 3, False, 3)):
        process = subprocess.run([sys.executable, str(script), "--executable", str(executable),
                                  "--output-directory", name, "--scenarios", "static", "--seed", str(seed),
                                  "--warmup-seconds", "0", "--sample-seconds", "1", "--repeats", str(repeats),
                                  "--skip-gpu-sensors"], cwd=root, capture_output=True, text=True, timeout=30)
        result_path = root / name / "static-1/result.json"
        if not result_path.is_file():
            raise RuntimeError(f"Relative output was not retained: {result_path}; {process.stderr}")
        result = json.loads(result_path.read_text(encoding="utf-8"))
        if result["exitCode"] != code or result["passed"] != passed:
            raise RuntimeError(f"Incorrect exit code or validity: {name}")
        if passed:
            if process.returncode or result["frameMs"]["count"] != 2 or result["frameMs"]["p95"] != 30 or result["gpuDrawMs"]["count"] != 1:
                raise RuntimeError(f"Successful statistics changed: {name}; {process.stderr}")
            if not (root / name / "static-2/result.json").is_file():
                raise RuntimeError("Successful sequence stopped early.")
        else:
            if not process.returncode or not result["failureReasons"] or (root / name / "static-2").exists():
                raise RuntimeError("Failure diagnostics or fail-fast behavior changed.")
            if seed == 4 and (result["focusLostFrames"] != 1 or "window-not-focused" not in process.stderr):
                raise RuntimeError("Focus failure was not reported.")
        aggregate = json.loads((root / name / "results.json").read_text(encoding="utf-8"))
        if not isinstance(aggregate, list) or len(aggregate) != (repeats if passed else 1):
            raise RuntimeError("Aggregate results must remain a JSON array.")
        print(f"{name}: passed")
    existing = root / "success/machine.json"
    before = hashlib.sha256(existing.read_bytes()).hexdigest()
    process = subprocess.run([sys.executable, str(script), "--executable", str(executable),
                              "--output-directory", "success", "--skip-gpu-sensors"],
                             cwd=root, capture_output=True, text=True, timeout=30)
    if not process.returncode or "previous runs are not overwritten" not in process.stderr or hashlib.sha256(existing.read_bytes()).hexdigest() != before:
        raise RuntimeError("Existing output was not protected.")
    print("Output protection passed; no game/GL context was used.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark-script", required=True, type=Path)
    parser.add_argument("--fixture-executable", required=True, type=Path)
    parser.add_argument("--output-parent", required=True, type=Path)
    args = parser.parse_args()
    verify(args.benchmark_script.resolve(), args.fixture_executable.resolve(), args.output_parent)
