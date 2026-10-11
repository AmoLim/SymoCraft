"""Copy inspectable candidate evidence into the milestone without moving runnable packages."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import stat


ROOT = Path(__file__).resolve().parents[3]
SUFFIXES = {".log", ".json", ".txt", ".xml", ".yaml", ".csv", ".png", ".tsv", ".dot", ".dxil", ".py", ".h"}


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2, ensure_ascii=True) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", action="append", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    args = parser.parse_args()
    output = args.output_directory.resolve()
    evidence = ROOT / "docs/milestones/m3-t3/evidence/r2"
    if not output.is_relative_to(evidence) or output == evidence:
        parser.error("Output must be a new directory below the T3 R2 evidence directory.")
    candidates = set()
    for directory in args.input:
        directory = directory.resolve()
        if not directory.is_relative_to(ROOT / "out/m3-t3") or not directory.is_dir():
            parser.error(f"Input must be an existing T3 out directory: {directory}")
        candidates.update(p for p in directory.rglob("*") if p.is_file() and p.suffix.lower() in SUFFIXES
                          and not any(part in {"CMakeScratch", "CompilerIdC", "CompilerIdCXX"} for part in p.parts))
    candidates.add(ROOT / "CMakeLists.txt")
    candidates.add(ROOT / "scripts/package-d3d12-candidate.py")
    for scope in ("game", "cmake", "test/integration/renderer", "assets/shaders/hlsl"):
        candidates.update(p for p in (ROOT / scope).rglob("*") if p.is_file() and "__pycache__" not in p.parts)
    output.mkdir(parents=True, exist_ok=False)
    records = []
    for source in sorted(candidates):
        relative = source.relative_to(ROOT)
        target = output / "raw" / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        before = digest(source)
        shutil.copy2(source, target)
        after = digest(target)
        if before != after or digest(source) != before:
            raise RuntimeError(f"Input changed during archive: {source}")
        target.chmod(stat.S_IREAD)
        records.append({"origin": relative.as_posix(), "path": target.relative_to(output).as_posix(),
                        "bytes": target.stat().st_size, "sha256": before})
    manifest = output / "validation-manifest.json"
    save(manifest, {"schema": "symocraft.candidate-evidence.v1", "source_root": str(ROOT),
                    "files": records, "file_count": len(records), "bytes": sum(r["bytes"] for r in records),
                    "runnable_package": False, "r3_acceptance": False})
    manifest.chmod(stat.S_IREAD)
    checks = [{"path": r["path"], "copy_matches": digest(output/r["path"]) == r["sha256"],
               "origin_matches_at_archive": digest(ROOT/r["origin"]) == r["sha256"]} for r in records]
    save(output / "archive-check.json", {"manifest_sha256": digest(manifest), "files": len(records),
         "all_copies_match": all(r["copy_matches"] for r in checks),
         "all_origins_match_at_archive": all(r["origin_matches_at_archive"] for r in checks), "checks": checks})
    print(json.dumps({"output": str(output), "files": len(records), "manifest_sha256": digest(manifest)}))


if __name__ == "__main__":
    main()
