"""Inject invalid assets only into retained copies of an OpenGL game package."""

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys

from scripts_common import cli, file_path, new_directory, option, read_log, sha256, write_json

CASES = (
    ("invalid-vertex-shader", "shaders/vs_BlockShader.glsl", "#version 460 core\nthis is deliberately invalid GLSL;\n", "Shader compilation failed:"),
    ("invalid-texture-image", "textures/texture_atlas.png", "This dedicated test fixture is not a PNG image.\n", "Cannot decode image: "),
    ("empty-block-configuration", "configs/blockFormats.yaml", "", "Failed to load block configuration: Block configuration must be a nonempty mapping"),
)


def verify(executable, output_directory):
    executable = file_path(executable)
    assets = (executable.parent / "assets").resolve(strict=True)
    if not assets.is_dir():
        raise ValueError(f"The executable's adjacent assets directory is missing: {assets}")
    output = Path(output_directory).resolve()
    if output.is_relative_to(assets):
        raise ValueError("OutputDirectory cannot be inside source assets because fixtures copy that directory.")
    hashes = {asset: sha256(file_path(assets / asset)).upper() for _, asset, _, _ in CASES}
    output = new_directory(output)
    results = []
    for name, asset, content, diagnostic in CASES:
        fixture = output / name
        package = fixture / "package"
        package.mkdir(parents=True)
        fixture_executable = package / executable.name
        shutil.copy2(executable, fixture_executable)
        fixture_assets = package / "assets"
        shutil.copytree(assets, fixture_assets)
        fault_file = (fixture_assets / asset).resolve()
        if not fault_file.is_relative_to(package.resolve()):
            raise ValueError(f"Refusing to alter a file outside the dedicated fixture: {fault_file}")
        fault_file.write_text(content, encoding="utf-8")
        evidence = fixture / "evidence"
        probe = subprocess.run([sys.executable, str(Path(__file__).with_name("verify-runtime.py")),
                                "--executable", str(fixture_executable), "--output-directory", str(evidence),
                                "--frames", "1", "--expected-exit-code", "3"], capture_output=True, text=True, encoding="utf-8", errors="replace")
        issues = []
        if probe.returncode:
            issues.append(f"Runtime probe failed: {probe.stderr.strip()}")
        result_file = evidence / "result.json"
        runtime = None
        if result_file.is_file():
            runtime = json.loads(read_log(result_file))
            if runtime["exitCode"] != 3 or runtime["timedOut"] or runtime["ready"] or not runtime["shutdown"] or not runtime["passed"]:
                issues.append("The game did not fail before readiness with exit code 3 and a clean shutdown marker.")
        else:
            issues.append("The runtime probe did not produce result.json.")
        stdout = read_log(evidence / "stdout.log") if (evidence / "stdout.log").is_file() else ""
        stderr = read_log(evidence / "stderr.log") if (evidence / "stderr.log").is_file() else ""
        for marker in ("OpenGL vendor:", "OpenGL renderer:", "OpenGL version:"):
            if marker not in stdout:
                issues.append(f"Missing proof of actual OpenGL initialization: {marker}")
        if "[runtime] fatal:" not in stderr or diagnostic not in stderr:
            issues.append(f"Missing expected fault diagnostic: {diagnostic}")
        if "[runtime] shutdown complete; exit_code=3" not in stdout:
            issues.append("Missing the final runtime cleanup marker with exit code 3.")
        after = sha256(assets / asset).upper()
        if after != hashes[asset]:
            issues.append("The source asset changed during the probe; investigate before accepting this evidence.")
        result = {"case": name, "modifiedFixtureAsset": str(fault_file), "expectedDiagnostic": diagnostic,
                  "sourceSha256Before": hashes[asset], "sourceSha256After": after, "runtimeResult": runtime,
                  "passed": not issues, "issues": issues}
        write_json(fixture / "fault-result.json", result)
        results.append(result)
    summary = {"sourceExecutable": str(executable), "sourceAssets": str(assets),
               "checkedSourceAssetsModified": any(result["sourceSha256Before"] != result["sourceSha256After"] for result in results),
               "fixtureDirectoriesRetained": True, "passed": all(result["passed"] for result in results), "cases": results}
    write_json(output / "summary.json", summary)
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "Executable", required=True)
    option(parser, "OutputDirectory", required=True)
    args = parser.parse_args()
    summary = verify(args.Executable, args.OutputDirectory)
    print(json.dumps(summary, indent=2))
    if not summary["passed"]:
        raise RuntimeError(f"Runtime fault verification failed; see {args.OutputDirectory}")


if __name__ == "__main__":
    cli(main)
