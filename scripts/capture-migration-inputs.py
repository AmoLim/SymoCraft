"""Capture byte-verified inputs and Git identity without changing the worktree."""

import argparse
from datetime import datetime
import json
from pathlib import Path
import shutil
import subprocess

from scripts_common import ROOT, cli, file_path, new_directory, option, sha256, write_json


def capture(output_directory, root=ROOT):
    root = Path(root).resolve()
    output = (root / output_directory).resolve()
    if not output.is_relative_to(root) or output == root:
        raise ValueError("The snapshot must stay inside the workspace.")
    if output.exists():
        raise ValueError("Use a new directory; existing migration evidence is never overwritten.")
    command = ["rg", "--files", "--hidden", "-g", "!.git/**", "-g", "!test/experimental/**",
               "game", "cmake", "test", "scripts", "tools/benchmark", "vendor", "assets"]
    inventory = subprocess.run(command, cwd=root, capture_output=True, encoding="utf-8", check=True)
    paths = {"CMakeLists.txt", "CMakePresets.json", "tools/benchmark/README.md",
             "docs/third-party.md", *(Path(relative).as_posix() for relative in inventory.stdout.splitlines())}
    # Resolve the inventory before creating output, so a snapshot cannot include itself.
    originals = []
    for relative in sorted(paths):
        original = file_path(root / relative)
        if not original.is_relative_to(root):
            raise ValueError(f"Input escapes the workspace: {relative}")
        originals.append((relative, original))
    output = new_directory(output)
    source = output / "source"
    source.mkdir()
    files = []
    for relative, original in originals:
        digest = sha256(original)
        size = original.stat().st_size
        destination = source / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(original, destination)
        if sha256(destination) != digest or sha256(original) != digest:
            raise RuntimeError(f"Input changed during capture: {relative}")
        files.append({"path": Path(relative).as_posix(), "bytes": size, "sha256": digest})

    def git(*arguments):
        return subprocess.run(["git", *arguments], cwd=root, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, check=True).stdout

    head = git("rev-parse", "HEAD").decode("ascii").strip()
    (output / "git-status.txt").write_bytes(git("status", "--porcelain=v1"))
    (output / "tracked-changes.patch").write_bytes(git("diff", "--binary", "--", ".", ":!vendor/sdl3"))
    manifest = {"captured_at": datetime.now().astimezone().isoformat(), "git_head": head,
                "scope": "Conservative root CMake/presets, game, cmake, test (excluding isolated experimental probes), scripts, tools/benchmark, vendor, assets and the two installed benchmark documents. Includes dirty and untracked inputs; not a compiler-read trace.",
                "restore_policy": "Reference only. Never reset or overwrite the live worktree automatically.",
                "count": len(files), "files": files}
    write_json(output / "build-inputs.json", manifest)
    return {"output": str(output), "count": len(files), "manifest_sha256": sha256(output / "build-inputs.json"), "byte_verified": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "OutputDirectory", required=True)
    args = parser.parse_args()
    print(json.dumps(capture(args.OutputDirectory), indent=2))


if __name__ == "__main__":
    cli(main)
