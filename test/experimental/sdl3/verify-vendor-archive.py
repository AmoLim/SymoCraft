"""Compare an official ZIP archive against local vendor files byte-for-byte."""
import argparse
from datetime import datetime
import hashlib
from pathlib import Path, PurePosixPath
import sys
import zipfile
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import cli, files, option, reject_links, require, sha256, write_json


def verify(archive, vendor):
    archive, vendor = Path(archive), Path(vendor).resolve(strict=True)
    reject_links(vendor)
    entries, paths, top = [], set(), None
    with zipfile.ZipFile(archive) as source:
        for entry in source.infolist():
            if entry.is_dir():
                continue
            original_name = entry.orig_filename
            parts = PurePosixPath(original_name).parts
            require(len(parts) > 1 and not PurePosixPath(original_name).is_absolute() and ".." not in parts and "\\" not in original_name,
                    "Expected safe files under one top-level archive directory.")
            require(top is None or top == parts[0], "Archive must have exactly one top-level directory.")
            top = parts[0]
            relative = PurePosixPath(*parts[1:]).as_posix()
            require(relative not in paths, f"Duplicate archive entry: {relative}")
            paths.add(relative)
            digest = hashlib.sha256()
            with source.open(entry) as stream:
                for block in iter(lambda: stream.read(1024 * 1024), b""):
                    digest.update(block)
            reject_links(vendor / relative)
            local = (vendor / relative).resolve()
            require(local.is_relative_to(vendor), "Archive entry escapes vendor directory.")
            local_hash = sha256(local) if local.is_file() else None
            digest = digest.hexdigest()
            entries.append({"path": relative, "archive_sha256": digest, "local_sha256": local_hash,
                            "status": "missing" if local_hash is None else "identical" if digest == local_hash else "different"})
    return {"captured_at": datetime.now().astimezone().isoformat(), "archive_sha256": sha256(archive), "archive_files": len(entries),
            **{name: sum(x["status"] == name for x in entries) for name in ("identical", "different", "missing")},
            "extra": [x.relative_to(vendor).as_posix() for x in files(vendor) if x.relative_to(vendor).as_posix() not in paths], "files": entries}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("Archive", "VendorDirectory", "OutputFile"):
        option(parser, name, required=True)
    args = parser.parse_args()
    require(not Path(args.OutputFile).exists(), "OutputFile must be new.")
    result = verify(args.Archive, args.VendorDirectory)
    write_json(args.OutputFile, result)
    print({key: result[key] for key in ("archive_sha256", "archive_files", "identical", "different", "missing", "extra")})
    require(result["archive_files"] > 0 and not result["different"] and not result["missing"] and not result["extra"], "Vendor files do not match the archive; report retained.")


if __name__ == "__main__":
    cli(main)
