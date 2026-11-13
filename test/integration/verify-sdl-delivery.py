"""Installed GL delivery/asset-fault diagnostics; scenario cancellation uses the app probe."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from verification_common import ROOT, capture, clean_environment, cli, evidence_sources, file_path, files, hidden_process, identity, load_json, new_directory, option, read_log, require, safe_child, sha256, stop_process, workspace_path, write_json
from scripts_common import integer
from windows_verification import Windows, wait_ready

CRT = re.compile(r"^(msvcp140.*|vcruntime140.*|concrt140)\.dll$", re.I)
FAULTS = [("invalid-vertex-shader", "assets/shaders/vs_BlockShader.glsl", "#version 460 core\nthis is deliberately invalid GLSL;\n", "Shader compilation failed:"),
          ("invalid-texture", "assets/textures/texture_atlas.png", "This dedicated fixture is not a PNG image.\n", "Cannot decode image: "),
          ("invalid-block-config", "assets/configs/blockFormats.yaml", "", "Failed to load block configuration: Block configuration must be a nonempty mapping")]


def package_identity(package):
    return [identity(p, package) for p in files(package)]


def copy_package(source, destination, without_crt=False):
    destination.mkdir()
    for file in files(source):
        relative = file.relative_to(source).as_posix()
        if without_crt and CRT.fullmatch(relative):
            continue
        target = safe_child(destination, relative)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(file, target)


def finite_valid(record, stdout, stderr, expected, frames, diagnostic=None, graphics_required=True):
    match = re.search(r"\[runtime\] loop finished; rendered_frames=(\d+)", stdout)
    rendered = int(match[1]) if match else None
    ready = "[runtime] ready;" in stdout
    shutdown = f"[runtime] shutdown complete; exit_code={expected}" in stdout
    gl = all("OpenGL " + key + ":" in stdout for key in ("vendor", "renderer", "version"))
    valid = not record["timed_out"] and record["exit_code"] == expected and shutdown and (gl if graphics_required else not gl and "Monitor size:" not in stdout)
    if expected == 0:
        valid = valid and ready and rendered == frames and "[runtime] fatal:" not in stderr and "OpenGL diagnostic" not in stderr
    else:
        valid = valid and not ready and "[runtime] fatal:" in stderr and diagnostic in stderr
    return bool(valid), rendered, ready, shutdown, gl


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("GamePackage", "OutputDirectory"):
        option(parser, name, required=True)
    option(parser, "TimeoutSeconds", type=integer(30, 180), default=90)
    option(parser, "Frames", type=integer(1, 1000), default=120)
    args = parser.parse_args()
    api = Windows()
    game = Path(args.GamePackage).resolve(strict=True)
    destination = workspace_path(args.OutputDirectory, ROOT / "out")
    for package in (game,):
        require(package.is_dir() and not destination.is_relative_to(package), "Evidence must not be inside a source package.")
    game_identity = package_identity(game)
    for path in (game / "SymoCraft.exe", game / "assets/configs/blockFormats.yaml"):
        file_path(path)
    output = new_directory(destination)
    write_json(output / "input-identity.json", {"game_package": str(game), "game_files": game_identity,
               "verifier_sha256": sha256(__file__), "verification_sources": evidence_sources([], [ROOT / "scripts/windows_verification.py"])})
    cases = [("installed-finite-frames", None, None, None, False, False, False), ("different-working-directory", None, None, None, True, False, False),
             ("unicode-space-package", None, None, None, True, True, False)]
    cases += [(name, fault, content, diagnostic, False, False, False) for name, fault, content, diagnostic in FAULTS]
    cases += [("missing-app-local-crt-observation", None, None, None, True, False, True)]
    results = []
    for name, fault, content, diagnostic, system_cwd, unicode, omit_crt in cases:
        directory = new_directory(output / name)
        package = directory / ("\u4e2d\u6587 package with spaces" if unicode else "package")
        result = {"case": name, "state": "failed", "passed": False, "error": None}
        try:
            copy_package(game, package, omit_crt)
            if fault:
                target = file_path(safe_child(package, fault))
                target.write_text(content, encoding="utf-8")
                result.update(fixture_fault_file=str(target), fixture_fault_sha256=sha256(target), source_asset_sha256=sha256(game / fault))
            expected, frames = (3, 1) if fault else (0, args.Frames)
            cwd = Path(os.environ.get("SystemRoot", "C:/Windows")) / "System32" if system_cwd else directory
            prior = api.kernel.SetErrorMode(0x8003)
            try:
                actual = capture(package / "SymoCraft.exe", ["--renderer", "opengl", "--smoke-frames", frames], cwd, directory / "stdout.log", directory / "stderr.log", args.TimeoutSeconds, clean_environment())
            finally:
                api.kernel.SetErrorMode(prior)
            passed, rendered, ready, shutdown, gl = finite_valid(actual, read_log(directory / "stdout.log"), read_log(directory / "stderr.log"), expected, frames, diagnostic, name != "invalid-block-config")
            result.update(state="passed" if passed else "failed", passed=passed, executable=str(package / "SymoCraft.exe"), working_directory=str(cwd), process_exit_code=actual["exit_code"],
                          expected_exit_code=expected, requested_frames=frames, rendered_frames=rendered, timed_out=actual["timed_out"], forced_termination=actual["timed_out"], ready=ready, shutdown=shutdown,
                          actual_gl_initialized=gl, expected_diagnostic=diagnostic, elapsed_seconds=actual["elapsed_seconds"], scope="Finite-frame delivery diagnostic; not 15-minute acceptance.")
            if omit_crt and not actual["timed_out"]:
                result.update(state="unavailable", passed=False, clean_machine_missing_crt_verified=False,
                              omitted_app_local_crt=[e["path"] for e in game_identity if CRT.fullmatch(e["path"])],
                              scope="App-local CRT omitted only in dedicated copy; developer/system fallback cannot prove clean-machine behavior. No system files removed.")
        except (ValueError, OSError) as error:
            result["error"] = str(error)
        write_json(directory / "result.json", result)
        results.append(result)
    unchanged = package_identity(game) == game_identity
    failed = sum(r["state"] == "failed" for r in results)
    unavailable = sum(r["state"] == "unavailable" for r in results)
    summary = {"game_package": str(game), "source_packages_unchanged": unchanged, "total_cases": len(results),
               "passed_cases": sum(r["state"] == "passed" for r in results), "failed_cases": failed, "unavailable_cases": unavailable,
               "verified_cases_passed": failed == 0 and unchanged, "all_requested_checks_passed": failed == 0 and unavailable == 0 and unchanged,
               "hardware_acceptance_passed": False, "q06_acceptance_sample": False, "q07_budget_acceptance": False, "y9000p_checked": False,
               "fixtures_and_first_failures_retained": True, "cases": results}
    write_json(output / "summary.json", summary)
    require(summary["verified_cases_passed"], f"SDL delivery verification failed; see {output}")
    if unavailable:
        print("Missing-CRT clean-machine behavior remains unverified; this is not overall delivery acceptance.")


if __name__ == "__main__":
    cli(main)
