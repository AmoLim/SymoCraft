"""Run bounded owned-device diagnostics against a completed current-source build."""

import argparse
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "scripts"))
from verification_common import (archive_destination, capture, copy_evidence, identity, load_json,
                                 new_directory, require, sha256, utc_now, workspace_path, write_json)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", required=True)
    parser.add_argument("--matrix-evidence", required=True)
    parser.add_argument("--output-directory", required=True)
    args = parser.parse_args()
    build = workspace_path(args.build_directory, ROOT / "out/m3-t3")
    matrix = workspace_path(args.matrix_evidence, ROOT / "docs/verification/archive")
    result = load_json(matrix / "matrix-result.json")
    case = next(c for c in result["cases"] if Path(c["build_directory"]) == build)
    require(result["completed"] and result["source_unchanged"] and case["build"]["exit_code"] == 0,
            "A completed current-source build matrix must bind this diagnostic executable.")
    sources = load_json(matrix / "source-inputs.json")
    for entry in sources:
        require(sha256(entry["path"]) == entry["sha256"], f"Compiled input changed: {entry['path']}")
    output = archive_destination(args.output_directory)
    new_directory(output)
    executable = build / "test/integration/renderer_d3d12_fault_probe.exe"
    shaders = build / "bin/assets/shaders/d3d12"
    report = {"schema": "symocraft.t3.r2-controlled-native-faults", "version": 1,
              "started_utc": utc_now(), "success": False, "matrix": identity(matrix / "matrix-result.json"),
              "source_inputs": identity(matrix / "source-inputs.json"), "runner": identity(Path(__file__)),
              "probe": identity(executable), "build_cache": identity(build / "CMakeCache.txt"),
              "libraries": [identity(build / p) for p in (
                  "game/modules/renderer/symocraft_renderer.lib",
                  "game/modules/renderer/symocraft_renderer_d3d12.lib",
                  "game/modules/platform/symocraft_platform.lib")],
              "shaders": [identity(p) for p in sorted(shaders.glob("*.dxil"))],
              "spontaneous_hardware_device_loss": False, "real_native_submission_failure": False,
              "real_memory_exhaustion": False, "public_wrapper_or_gameplay_acceptance": False,
              "r2_complete": False, "r3_acceptance": False, "cases": []}
    try:
        for mode in ("allocation", "submission", "device-removal", "in-flight-timeout"):
            arguments = [mode, shaders, output / f"{mode}.yaml"]
            item = capture(executable, arguments, output, output / f"{mode}.stdout.log",
                           output / f"{mode}.stderr.log", timeout=30)
            item.update(mode=mode, arguments=list(map(str, arguments)))
            report["cases"].append(item)
            write_json(output / "run.json", report)
            print(f"{mode}: exit={item['exit_code']} timeout={item['timed_out']}", flush=True)
            require(not item["timed_out"] and item["exit_code"] == 0,
                    f"Owned diagnostic failed: {mode}; raw report and logs retained.")
            item["report"] = identity(output / f"{mode}.yaml")
        require(all(sha256(e["path"]) == e["sha256"] for e in sources), "Sources changed during native diagnostics")
        report["success"] = True
    finally:
        report["finished_utc"] = utc_now()
        write_json(output / "run.json", report)


if __name__ == "__main__":
    main()
