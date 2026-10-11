"""Archive completed four-case SDL offline/input evidence and exact provenance."""
import argparse
from pathlib import Path
import re
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, archive_destination, assert_hash, cli, copy_evidence, file_path, files, identity, load_json, new_directory, option, read_log, require, sha256, snapshot_identity, utc_now, workspace_path, write_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "AuditDirectory", default="out/m3-t2/cmake-3.22-offline-audit-005")
    option(parser, "OutputDirectory", default="docs/verification/archive/t2-input/input-v2-001")
    args = parser.parse_args()
    destination = archive_destination(args.OutputDirectory)
    audit_root = workspace_path(args.AuditDirectory)
    audit = load_json(audit_root / "summary.json")
    require(audit["completed"] and len(audit["cases"]) == 4 and all(c["completed"] and not c["changed_inputs"] for c in audit["cases"]), "Four-case audit incomplete or inputs changed.")
    vendor_path = ROOT / "out/m3-t2/sdl-official-vendor-input-v2-comparison.json"
    vendor = load_json(vendor_path)
    require(not vendor["different"] and not vendor["missing"] and not vendor["extra"] and vendor["identical"] == 2183, "Official SDL source identity changed.")
    copies = {audit_root / "summary.json": "cmake322-audit.json", audit_root / "import-audit.json": "cmake322-imports.json"}
    copies.update({ROOT / ("out/m3-t2/" + key): value for key, value in {
        "input-comparison/automated-002/result.json": "comparison-rendering.json", "probe/gl-release-cmake322-runtime-001/summary.json": "cmake322-gl-runtime.json",
        "runtime/sdl-input-002/result.json": "runtime-input.json", "runtime/sdl-input-002/stdout.log": "runtime-input.stdout.log",
        "runtime/sdl-input-002/stderr.log": "runtime-input.stderr.log", "probe/input-comparison-sdl-release-002/Testing/Temporary/LastTest.log": "candidate-185.log"}.items()})
    for path in copies:
        file_path(path)
    archive = ROOT / "out/m3-t2/tools/cmake-3.22.6/cmake-3.22.6-windows-x86_64.zip"
    checksum = archive.with_name("cmake-3.22.6-SHA-256.txt")
    matches = re.findall(r"^([0-9a-fA-F]{64})\s+cmake-3\.22\.6-windows-x86_64\.zip$", read_log(checksum), re.M)
    require(len(matches) == 1 and matches[0].lower() == sha256(archive), "Portable CMake ZIP differs from official checksum.")
    baseline = load_json(ROOT / "out/m3-t2/baseline-input-001/build-inputs.json")
    production = [e for e in baseline["files"] if e["path"] in {"CMakeLists.txt", "CMakePresets.json"} or re.match(r"^(game/|cmake/|assets/|vendor/(glfw|glad|KHR|yaml-cpp)/)", e["path"])]
    for entry in production:
        assert_hash(ROOT / entry["path"], entry["sha256"], "Frozen production input")
    from_path = ROOT / "out/m3-t2"
    executable_paths = ["probe/input-comparison-sdl-release-002/SymoCraftSdl3InputComparison.exe", "probe/input-comparison-glfw-release-001/Release/SymoCraftGlfwInputComparison.exe",
                        "probe/input-comparison-sdl-release-002/SymoCraftSdl3RuntimeInputTests.exe", "probe/input-comparison-sdl-release-002/SymoCraftSdl3InputCandidateTests.exe"]
    sources = snapshot_identity(["test/experimental", "scripts/verification_common.py", "scripts/scripts_common.py"])
    executables = [identity(from_path / p, ROOT) for p in executable_paths]
    build_log = from_path / "input-comparison-sdl-release-002-build.log"
    document = {"created_utc": utc_now(), "scope": "Experimental observations/current binaries; SDL bound by official archive recheck; frozen CPU/GLFW archives recorded separately.",
                "sources": sources, "executables": executables, "production_inputs_compared": len(production), "production_changes": [], "production_platform": "GLFW",
                "audit_root": args.AuditDirectory, "audit_input_manifest_sha256": sha256(audit_root / "inputs-sha256.json"),
                "continuous_release_build_log": build_log.relative_to(ROOT).as_posix(), "continuous_release_build_log_sha256": sha256(build_log), "hardware_validation": False}
    destination = new_directory(destination)
    copied = [{"source": path.relative_to(ROOT).as_posix(), "path": name,
               **copy_evidence(path, destination / name)} for path, name in copies.items()]
    write_json(destination / "cmake322-provenance.json", {"version": "3.22.6", "source_url": "https://github.com/Kitware/CMake/releases/download/v3.22.6/cmake-3.22.6-windows-x86_64.zip",
               "archive_sha256": sha256(archive), "official_checksum_file_sha256": sha256(checksum), "system_installation_modified": False})
    write_json(destination / "vendor-recheck.json", {"archive_sha256": vendor["archive_sha256"], "files": vendor["archive_files"], "identical": vendor["identical"],
               "different": vendor["different"], "missing": vendor["missing"], "extra": vendor["extra"], "raw_comparison": vendor_path.relative_to(ROOT).as_posix(), "raw_comparison_sha256": sha256(vendor_path)})
    write_json(destination / "input-identities.json", document)
    for row in copied:
        assert_hash(ROOT / row["source"], row["sha256"], "Evidence origin")
    write_json(destination / "archive-manifest.json", {"schema": "symocraft.sdl-input-archive.v1", "source_root": str(ROOT),
               "files": [identity(p, destination) for p in files(destination)], "copied_sources": copied,
               "runnable_package": False, "acceptance": False})


if __name__ == "__main__":
    cli(main)
