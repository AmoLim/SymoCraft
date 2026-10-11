"""Compare genuine old/new game processes, retaining logs and fault packages."""
import argparse
from pathlib import Path
import shutil
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from verification_common import ROOT, assert_hash, capture, cli, clean_environment, file_path, files, new_directory, option, require, sha256, workspace_path, write_json


def verify(args):
    old, new, debug = [file_path(getattr(args, name)) for name in ("OldExecutable", "NewExecutable", "DebugExecutable")]
    output_path = workspace_path(args.OutputDirectory, ROOT / "out")
    for executable in (old, new, debug):
        require(not output_path.is_relative_to(executable.parent), "Evidence must not be inside a source package.")
    files(new.parent)
    inputs = {exe: sha256(exe) for exe in (old, new, debug)}
    output = new_directory(output_path)
    cwd = new_directory(output / "unrelated-cwd")
    records = []
    def probe(name, executable, arguments, expected=0, timeout=120):
        record = capture(executable, arguments, cwd, output / (name + ".stdout.log"), output / (name + ".stderr.log"), timeout, clean_environment())
        records.append({"name": name, "executable": str(executable), "exit_code": record["exit_code"], "expected": expected,
                        "arguments": list(map(str, arguments)), "timed_out": record["timed_out"]})
        write_json(output / "probes.json", records)
        require(not record["timed_out"] and record["exit_code"] == expected, f"Probe {name} failed; see retained logs.")
        return (output / (name + ".stdout.log")).read_bytes()
    write_json(output / "identity.json", {"old": sha256(old).upper(), "new": sha256(new).upper(), "debug": sha256(debug).upper(),
        "note": "Short migration comparison, not the frozen 36-minute baseline. No user gameplay claim."})
    arguments = ["--world-summary", "--seed", "424242", "--scene", "regression", "--checkpoint", "four-chunk", "--test-edits"]
    require(probe("old-world-summary", old, arguments) == probe("new-world-summary", new, arguments), "World summary differs byte-for-byte; inspect logs before continuing.")
    probes = [("old-static", old, "static"), ("new-static", new, "static"), ("new-edit", new, "edit")]
    if args.IncludeWorldRegression:
        probes += [("old-edit", old, "edit"), ("old-walk", old, "walk"), ("new-walk", new, "walk")]
    for name, executable, scene in probes:
        directory = output / name
        probe(name, executable, ["--benchmark", scene, "--output", directory, "--seed", "424242", "--warmup-seconds", "5", "--sample-seconds", "15", "--focus-policy", "allow-unfocused"])
        require((directory / "summary.yaml").is_file(), "Missing benchmark summary.")
    probe("debug-smoke", debug, ["--scene", "regression", "--seed", "424242", "--smoke-frames", "120"])
    for fault, relative in (("shader", "assets/shaders/vs_BlockShader.glsl"), ("texture", "assets/textures/texture_atlas.png"), ("block-config", "assets/configs/blockFormats.yaml")):
        package = output / ("fault-" + fault)
        shutil.copytree(new.parent, package)
        (package / relative).write_text("deliberately invalid migration fixture", encoding="utf-8")
        probe("fault-" + fault, package / new.name, ["--smoke-frames", "1"], 3)
    for executable, digest in inputs.items():
        assert_hash(executable, digest)
    print(f"Runtime checks finished. Evidence: {output}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("OldExecutable", "NewExecutable", "DebugExecutable", "OutputDirectory"):
        option(parser, name, required=True)
    option(parser, "IncludeWorldRegression", action="store_true")
    verify(parser.parse_args())


if __name__ == "__main__":
    cli(main)
