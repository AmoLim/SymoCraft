"""Refresh the existing CPU-only builds after the concurrent Python-test registration."""

import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent


def main():
    if (OUT / "result.json").exists():
        raise RuntimeError("Existing CPU evidence must not be overwritten.")
    inputs = json.loads((ROOT / "out/m3-t3/r2-step4-final-delivery-002/source-inputs.json").read_text())
    result = {"cases": [], "production_inputs_unchanged": False}
    for configuration in ("debug", "release"):
        build = ROOT / "out/m3-t3/r2-step4-matrix-004" / ("cpu-" + configuration)
        commands = [("build", ["C:/Program Files/CMake/bin/cmake.exe", "--build", str(build), "--parallel", "2"]),
                    ("ctest", ["C:/Program Files/CMake/bin/ctest.exe", "--test-dir", str(build), "--output-on-failure",
                               "--parallel", "4", "--output-junit", str(OUT / (configuration + "-ctest.xml"))])]
        for action, arguments in commands:
            name = configuration + "-" + action
            with (OUT / (name + ".log")).open("wb") as log:
                completed = subprocess.run(arguments, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, timeout=300)
            result["cases"].append({"name": name, "arguments": arguments, "exit_code": completed.returncode})
            (OUT / "result.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
            print(name, completed.returncode, flush=True)
    result["production_inputs_unchanged"] = all(
        hashlib.sha256((ROOT / row["path"]).read_bytes()).hexdigest() == row["sha256"] for row in inputs)
    result["success"] = result["production_inputs_unchanged"] and all(row["exit_code"] == 0 for row in result["cases"])
    (OUT / "result.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return 0 if result["success"] else 1


if __name__ == "__main__":
    sys.exit(main())
