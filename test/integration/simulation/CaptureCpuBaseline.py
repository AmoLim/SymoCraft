"""Capture byte-verified CPU baseline inputs without changing live sources."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, assert_hash, cli, files, load_json, new_directory, option, reject_links, sha256, utc_now, write_json, require

GROUPS = {
    "production": ["game/modules/" + name for name in ("foundation", "scene", "ecs", "world", "simulation")],
    "dependencies": ["vendor/" + name for name in ("glm", "fast_noise_lite", "yaml-cpp")],
    "assets": ["assets/configs/blockFormats.yaml"],
    "build": ["CMakeLists.txt", "CMakePresets.json", "cmake", "game/CMakeLists.txt", "test/CMakeLists.txt",
              "scripts/build.py", "scripts/scripts_common.py", "scripts/verification_common.py"],
    "fixtures": ["test/unit/simulation", "test/integration/simulation"],
    "reference_only": ["game/app/src/application.cpp"],
}


def capture(args):
    root = Path(args.RepositoryRoot).resolve(strict=True)
    output_path = Path(args.OutputDirectory).resolve()
    reject_links(output_path)
    require(not output_path.is_relative_to(root) or output_path.is_relative_to(root / "out"), "Workspace snapshots belong under out, not inside source inputs.")
    entries = []
    for group, paths in GROUPS.items():
        for relative in paths:
            path = root / relative
            for item in files(path) if path.is_dir() else [path]:
                relative_item = item.relative_to(root)
                if "__pycache__" in relative_item.parts or item.suffix in {".pyc", ".pyo"}:
                    continue
                entries.append({"scope": group, "path": relative_item.as_posix(), "sha256": sha256(item)})
    entries.sort(key=lambda entry: entry["path"])
    if args.CompareDirectory:
        before = load_json(Path(args.CompareDirectory) / "identity.json")["files"]
        stable = set(args.CompareScopes)
        require({(x["path"], x["sha256"]) for x in before if x["scope"] in stable} ==
                {(x["path"], x["sha256"]) for x in entries if x["scope"] in stable},
                "Baseline inputs changed in the selected comparison scopes.")
    output = new_directory(args.OutputDirectory)
    for entry in entries:
        if entry["scope"] == "dependencies":
            continue
        original, destination = root / entry["path"], output / "sources" / entry["path"]
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(original, destination)
        assert_hash(destination, entry["sha256"], "Captured source")
        assert_hash(original, entry["sha256"], "Live source during capture")
    def git(*arguments):
        return subprocess.run(["git", "-C", str(root), *arguments], capture_output=True, text=True, encoding="utf-8", check=True, timeout=30).stdout
    result = {"schema": 1, "captured_utc": utc_now(), "repository": str(root), "head": git("rev-parse", "HEAD").strip(),
              "authority": "Actual file SHA-256, including dirty working tree; HEAD is context only.",
              "dependency_policy": "Vendor dependencies hashed in place; CPU/build/fixture sources copied byte-for-byte.",
              "reference_policy": "Application is scheduling context only, not linked or frozen by these CPU tests.",
              "compared_directory": args.CompareDirectory, "compared_scopes": args.CompareScopes,
              "working_tree": git("status", "--short").splitlines(), "files": entries}
    write_json(output / "identity.json", result)
    print(f"Captured {len(entries)} files in {output}")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "OutputDirectory", required=True)
    option(parser, "CompareDirectory")
    option(parser, "CompareScopes", nargs="+", choices=tuple(GROUPS), default=["production", "dependencies", "assets"])
    option(parser, "RepositoryRoot", default=str(ROOT))
    capture(parser.parse_args())


if __name__ == "__main__":
    cli(main)
