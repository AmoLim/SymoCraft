"""Record a final install/notices rebuild and tests without overwriting earlier runs."""

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
CMAKE = "C:/Program Files/CMake/bin/cmake.exe"
CTEST = "C:/Program Files/CMake/bin/ctest.exe"
PACKAGER = ROOT / "scripts/package-d3d12-candidate.py"


def save(name, value):
    (OUT / name).write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def main():
    if (OUT / "result.json").exists():
        raise RuntimeError("This final delivery run is already recorded.")
    spec = importlib.util.spec_from_file_location("candidate_packager", PACKAGER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    before = module.production_inputs()
    save("source-inputs.json", before)
    result = {"cases": [], "source_unchanged": False, "completed": False,
              "known_failures_are_not_passes": True, "r3_acceptance": False}

    def run(name, arguments, allowed_failure=False):
        with (OUT / (name + ".log")).open("wb") as log:
            process = subprocess.run(arguments, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT,
                                     timeout=600, check=False)
        row = {"name": name, "arguments": arguments, "exit_code": process.returncode}
        result["cases"].append(row)
        save("result.json", result)
        print(name, process.returncode, flush=True)
        if process.returncode and not allowed_failure:
            raise RuntimeError(name + " failed; retained evidence is not a pass.")
        return row

    run("python-tests", [sys.executable, "-m", "unittest", "discover", "-s",
                         "test/integration/renderer", "-p", "test_candidate_scripts.py", "-v"])
    for configuration in ("debug", "release"):
        build = ROOT / "out/m3-t3/r2-step4-matrix-004" / ("native-" + configuration)
        run(configuration + "-build", [CMAKE, "--build", str(build), "--parallel", "2"])
        if module.production_inputs() != before:
            raise RuntimeError("Production inputs changed during the final build.")
        xml = OUT / (configuration + "-ctest.xml")
        row = run(configuration + "-ctest", [CTEST, "--test-dir", str(build), "--output-on-failure",
                                             "--parallel", "4", "--output-junit", str(xml)], True)
        tests = list(ET.parse(xml).getroot().iter("testcase"))
        failed = [test for test in tests if test.find("failure") is not None]
        disabled = [test for test in tests if test.attrib.get("status") == "disabled"]
        row["tests"] = len(tests)
        row["disabled"] = len(disabled)
        row["passed"] = len(tests) - len(failed) - len(disabled)
        row["failed"] = []
        for test in failed:
            records = []
            for line in test.findtext("system-out", default="").splitlines():
                try:
                    records.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
            legacy = test.attrib["name"] in {"foundation.files", "performance.export"} and any(
                item.get("outcome") == "unexpected_failure" and item.get("numeric_win32") == 1175
                for item in records)
            row["failed"].append({"test": test.attrib["name"], "known_win32_1175": legacy})
            if not legacy:
                save("result.json", result)
                raise RuntimeError("A non-legacy test failed; packaging is stopped.")
        if row["exit_code"] and not failed:
            raise RuntimeError("CTest returned failure without a classified failing test.")
        save("result.json", result)
        package = ROOT / "out/m3-t3/package" / ("d3d12-game-" + configuration + "-002")
        run(configuration + "-package", [sys.executable, str(PACKAGER), "--build-directory", str(build),
                                         "--output-directory", str(package)])
    result["source_unchanged"] = module.production_inputs() == before
    result["completed"] = result["source_unchanged"]
    save("result.json", result)
    return 0 if result["completed"] else 1


if __name__ == "__main__":
    sys.exit(main())
