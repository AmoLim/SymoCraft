"""Curate existing preparation evidence, without overwriting historical records."""
import argparse
from pathlib import Path
import subprocess
import sys
import zipfile
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, archive_destination, assert_hash, cli, copy_evidence, file_path, files, identity, load_json, new_directory, option, read_log, require, sha256, snapshot_identity, utc_now, write_json

COPIES = {
    "glfw-debug-test-retry.log": "glfw-debug-test.log", "glfw-release-test.log": "glfw-release-test.log",
    "glfw-release-install-retry.log": "glfw-release-install.log", "sdl-gl-debug-final-test-002.log": "sdl-gl-debug-test.log",
    "sdl-vulkan-debug-final-test-002.log": "sdl-vulkan-debug-test.log", "sdl-gl-release-clean-final-test.log": "sdl-gl-release-test.log",
    "sdl-gl-release-final-install.log": "sdl-gl-release-install.log", "sdl-gl-release-final-dependents.log": "sdl-gl-release-dependents.log",
    "sdl-vulkan-debug-dependents.log": "sdl-vulkan-debug-dependents.log", "probe/gl-debug-runtime-final/summary.json": "sdl-gl-debug-probes.json",
    "probe/vulkan-debug-runtime-final/summary.json": "sdl-vulkan-debug-probes.json", "\u72ec\u7acb\u90e8\u7f72 \u542b\u7a7a\u683c/runtime/summary.json": "sdl-release-installed-probes.json",
    "runtime/glfw-debug-001/result.json": "glfw-debug-smoke.json", "runtime/glfw-release-001/result.json": "glfw-release-smoke.json",
    "runtime/glfw-faults-002/summary.json": "glfw-faults.json", "glfw-api-runtime-final/result.json": "glfw-api-probe.json",
    "glfw-api-runtime-final/stdout.log": "glfw-api-probe.stdout.log", "probe/gl-debug/Testing/Temporary/LastTest.log": "sdl-input-candidate.log"}


def source_inventory():
    result = subprocess.run(["rg", "--files", "test/experimental", "vendor/glad", "vendor/KHR"], cwd=ROOT, check=True, capture_output=True, encoding="utf-8", timeout=30)
    paths = result.stdout.splitlines() + ["cmake/ThirdPartyHeaders.cmake", "game/modules/platform/include/symocraft/platform/window.h",
                                        "scripts/capture-migration-inputs.py", "scripts/verify-runtime-faults.py", "scripts/scripts_common.py", "scripts/verification_common.py"]
    return snapshot_identity(paths)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "OutputDirectory", default="docs/verification/archive/t2-preparation/preparation-001")
    args = parser.parse_args()
    source = ROOT / "out/m3-t2"
    destination = archive_destination(args.OutputDirectory)
    comparison_path = source / "sdl-official-vendor-final-comparison.json"
    comparison = load_json(comparison_path)
    require(not comparison["different"] and not comparison["missing"] and not comparison["extra"], "SDL archive comparison failed.")
    archive = source / "Symocraft-M3-T2-GLFW-baseline-windows-x64.zip"
    require(not archive.exists(), "Baseline archive already exists.")
    package = source / "install/glfw-release"
    installed = [identity(p, package) for p in files(package)]
    require(installed, "Empty installed baseline.")
    for name in COPIES:
        file_path(source / name)
    provenance = {"captured_at": utc_now(), "version": "3.4.18", "tag": "release-3.4.18", "commit": read_log(ROOT / "vendor/sdl3/.git-hash").strip(),
                  "revision": read_log(ROOT / "vendor/sdl3/REVISION.txt").strip(), "source_url": "https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.zip",
                  "archive_sha256": comparison["archive_sha256"], "vendor_files": comparison["archive_files"], "byte_identical_files": comparison["identical"],
                  "comparison_path": "out/m3-t2/sdl-official-vendor-final-comparison.json", "comparison_sha256": sha256(comparison_path),
                  "license_path": "vendor/sdl3/LICENSE.txt", "license_sha256": sha256(ROOT / "vendor/sdl3/LICENSE.txt"), "vendor_source_modified": False, "old_sdl_directory_retained": False}
    baseline = {"captured_at": utc_now(), "git_head": subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, encoding="utf-8", check=True, timeout=30).stdout.strip(),
                "dirty_input": True, "baseline_manifest": "out/m3-t2/baseline-input-001/build-inputs.json", "baseline_manifest_sha256": sha256(source / "baseline-input-001/build-inputs.json"),
                "snapshot_exclusions": "out/m3-t2/baseline-input-001/snapshot-exclusions.json", "install_supplement": "out/m3-t2/baseline-input-001/install-input-supplement.json",
                "debug_game_sha256": sha256(source / "build/glfw-debug/bin/SymoCraft.exe"), "release_game_sha256": sha256(package / "SymoCraft.exe"),
                "runner_sha256": sha256(package / "SymoCraftBenchmark.exe"), "platform_archive_sha256": sha256(source / "build/glfw-release/game/modules/platform/symocraft_platform.lib"),
                "zip_path": archive.relative_to(ROOT).as_posix(), "installed_files": installed}
    inputs = source_inventory()
    executables = [identity(source / path, ROOT) for path in ("probe/gl-debug/SymoCraftSdl3Probe.exe", "probe/vulkan-debug/SymoCraftSdl3Probe.exe", "install/sdl-gl-probe-release/SymoCraftSdl3Probe.exe")]
    destination = new_directory(destination)
    with zipfile.ZipFile(archive, "x", zipfile.ZIP_DEFLATED) as output:
        for entry in installed:
            output.write(package / entry["path"], entry["path"])
    baseline["zip_sha256"] = sha256(archive)
    write_json(destination / "sdl-provenance.json", provenance)
    write_json(destination / "glfw-baseline-identity.json", baseline)
    write_json(destination / "preparation-inputs.json", {"captured_at": utc_now(), "scope": "Preparation/helper and GLAD/header identities, full SDL bound by archive manifest; not compiler-read trace.", "files": inputs})
    copied = [{"source": (source / source_name).relative_to(ROOT).as_posix(), "path": name,
               **copy_evidence(source / source_name, destination / name)} for source_name, name in COPIES.items()]
    write_json(destination / "sdl-probe-identities.json", {"captured_at": utc_now(), "executables": executables})
    for row in copied:
        assert_hash(ROOT / row["source"], row["sha256"], "Evidence origin")
    require(not any(p.suffix in {".exe", ".dll", ".lib", ".pdb", ".obj", ".zip"} for p in files(destination)), "Binary artifact in documentation evidence.")
    write_json(destination / "archive-manifest.json", {"schema": "symocraft.sdl-preparation-archive.v1", "source_root": str(ROOT),
               "files": [identity(p, destination) for p in files(destination)], "copied_sources": copied,
               "runnable_package": False, "acceptance": False})
    print(destination)


if __name__ == "__main__":
    cli(main)
