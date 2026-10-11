"""Derive delivery facts from actual immutable packages and completed runs."""

import hashlib
import json
from pathlib import Path
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
BASE = ROOT / "out/m3-t3"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def save(name, value):
    with (OUT / name).open("x", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")


def ctest(path):
    tests = list(ET.parse(path).getroot().iter("testcase"))
    failed = [t for t in tests if t.find("failure") is not None]
    disabled = [t for t in tests if t.attrib.get("status") == "disabled"]
    return {"path": str(path), "sha256": digest(path), "total": len(tests),
            "enabled": len(tests) - len(disabled), "passed": len(tests) - len(disabled) - len(failed),
            "disabled": len(disabled), "failures": [t.attrib["name"] for t in failed]}


def main():
    packages = []
    for configuration in ("debug", "release"):
        directory = BASE / "package" / ("d3d12-game-" + configuration + "-002")
        manifest = read(directory / "package-manifest.json")
        sources = read(directory / "provenance/candidate-build-identities.json")["sources"]
        actual = {p.relative_to(directory).as_posix(): (p.stat().st_size, digest(p))
                  for p in directory.rglob("*") if p.is_file() and p.name != "package-manifest.json"}
        declared = {p["path"]: (p["bytes"], p["sha256"]) for p in manifest["files"]}
        assert actual == declared
        assert all(digest(ROOT / p["path"]) == p["sha256"] for p in sources)
        packages.append({"directory": str(directory), "configuration": configuration,
                         "manifest_sha256": digest(directory / "package-manifest.json"),
                         "origin": manifest["origin_build"], "payload_files": len(actual),
                         "runtime_policy": manifest["runtime_policy"], "source_count": len(sources),
                         "sources": sources, "payload_unchanged": True, "current_production_inputs_match": True})
    assert packages[0]["sources"] == packages[1]["sources"]
    for package in packages:
        del package["sources"]
    runs = []
    images = []
    decoded = {}
    for configuration, name in (("debug", "r2-step4-candidate-debug-004"),
                                ("release", "r2-step4-candidate-release-002")):
        path = BASE / name / "verification.json"
        report = read(path)
        assert report["success"]
        assert report["package_identity"] == report["package_identity_after"]
        cases = []
        for case in report["cases"]:
            observation = dict(case.get("observation") or {})
            for key in ("frame_rows", "package_identity_before", "package_identity_after"):
                observation.pop(key, None)
            cases.append({"name": case["name"], "success": case["success"], "exit_code": case["exit_code"],
                          "observation": observation})
            image = BASE / name / case["name"] / "final-frame.png"
            if image.is_file():
                with Image.open(image) as opened:
                    pixels = np.asarray(opened.convert("RGB"))
                assert pixels.shape == (1080, 1920, 3) and float(pixels.std()) > 10
                decoded[configuration, case["name"]] = pixels
                images.append({"path": str(image), "file_sha256": digest(image), "extent": [1920, 1080],
                               "rgb_sha256": hashlib.sha256(pixels.tobytes()).hexdigest(),
                               "channel_standard_deviation": float(pixels.std()), "nonblank": True})
        runs.append({"configuration": configuration, "path": str(path), "sha256": digest(path),
                     "success": True, "cases": cases, "original_package_unchanged": True})
    pairs = []
    for workload in ("static", "walk", "edit"):
        left = decoded["debug", "d3d12-" + workload]
        for label, right in (("Debug-vs-Release-D3D12", decoded["release", "d3d12-" + workload]),
                             ("Debug-D3D12-vs-Debug-OpenGL", decoded["debug", "opengl-" + workload])):
            delta = np.abs(left.astype(np.int16) - right.astype(np.int16))
            pairs.append({"comparison": label, "workload": workload,
                          "different_pixels": int(np.count_nonzero(np.any(delta, axis=2))),
                          "mean_rgb_byte_difference": float(delta.mean()), "max_rgb_byte_difference": int(delta.max())})
    save("image-observations.json", {"scope": "Nine actual final PNGs; observations, not approved visual tolerance or performance acceptance.",
                                      "images": images, "comparisons": pairs})
    windows = []
    for name in ("r2-step4-window-debug-003", "r2-step4-window-release-004"):
        path = BASE / name / "verification.json"
        data = read(path)
        assert data["success"]
        windows.append({"path": str(path), "sha256": digest(path), "success": data["success"],
                        "minimize_duration_ms": data["minimize_duration_ms"], "actual_renderer": data["actual_renderer"],
                        "production_pause_resume": data["production_pause_resume"],
                        "rendered_frames": data["rendered_frames"], "exit_code": data["exit_code"]})
    old = {"r1/freeze-20261009-001/freeze-manifest.json": "ce27adb37619672634521bd5faca54ef605a3dcf73019826476746b897c49985",
           "r1/review-20261009-001/archive-manifest.json": "dac9096a6f43162ee28d2b97bfd8820e25c24c1c4c23399bf02fc325c97ab4fe",
           "r2/step1-20261009-001/validation-manifest.json": "35b09449d1dd59611c861e1cb26e4f24c0f4163eeef87736f3fdac67975f2ef2",
           "r2/step2-20261009-001/validation-manifest.json": "f63913b237691632b29bb4a41c2d2169bfe8139778a65e43a02ff83a56cf7e5f",
           "r2/step3-20261009-001/validation-manifest.json": "9246eeed80bfd3f7d5b0a7cd36bc43ee17cb4c74a7ed9b2bdb29fb9b10cf6879"}
    retained = []
    for name, expected in old.items():
        path = ROOT / "docs/milestones/m3-t3/evidence" / name
        actual = digest(path)
        retained.append({"path": str(path), "sha256": actual, "matches_previous": actual == expected})
    summary = {"schema": "symocraft.t3.r2-step4-delivery.v1", "packages": packages,
               "same_production_inputs": True, "candidate_runs": runs, "windows": windows,
               "ctest": [ctest(BASE / parent / (configuration + "-ctest.xml"))
                         for parent in ("r2-step4-final-cpu-001", "r2-step4-final-delivery-002")
                         for configuration in ("debug", "release")],
               "python_candidate_unit_tests": 24, "old_manifests": retained,
               "actual_device_lost_exercised": False, "actual_submission_failure_exercised": False,
               "actual_in_flight_timeout_exercised": False, "manual_gameplay_acceptance": False,
               "fifteen_minute_acceptance": False, "r3_acceptance": False, "opengl_default_changed": False,
               "modern_exclusive_build_exercised": False, "all_backends_off_game_build_exercised": False,
               "sdk_absent_clean_machine_exercised": False, "gui_benchmark_modern_protocol_supported": False,
               "supplementary_no_tests_and_tool_builds": "Sandbox C1902; escalated attempt aborted before execution; not passed."}
    save("delivery-summary.json", summary)
    print(json.dumps({"packages": packages, "ctest": summary["ctest"], "images": len(images),
                      "old_manifest_matches": all(row["matches_previous"] for row in retained)}))


if __name__ == "__main__":
    main()
