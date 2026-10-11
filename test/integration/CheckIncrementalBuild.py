"""Verify actual private/public header incremental dependencies and relinking."""
import argparse
from pathlib import Path
import re
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from verification_common import ROOT, cli, file_path, logged, new_directory, option, probe_header, require, workspace_path, write_json


def verify(args):
    build = workspace_path(args.BuildDirectory, ROOT / "out")
    output = new_directory(workspace_path(args.OutputDirectory, ROOT / "out"))
    cmake = file_path(args.CMakePath)
    def run(name):
        return logged(cmake, ["--build", build, "--target", "SymoCraft", "--parallel", "8"], output / (name + ".log"))
    require("Building CXX object" not in run("baseline"), "The initial build was not settled. Rerun after production changes finish.")
    results = []
    for name, relative in (("private-renderer", "game/modules/renderer/src/batch.hpp"), ("public-scene", "game/modules/scene/include/symocraft/scene/mesh.h")):
        with probe_header(ROOT / relative) as timestamp:
            log = run(name).replace("\\", "/")
            counts = {module: len(re.findall("Building CXX object " + ("game/app" if module == "app" else "game/modules/" + module) + "/", log))
                      for module in ("world", "simulation", "renderer", "app")}
            if name == "private-renderer":
                require(counts["renderer"] > 0 and all(counts[x] == 0 for x in ("world", "simulation", "app")), f"Private header rebuilt the wrong owners: {counts}")
            else:
                require(all(counts[x] > 0 for x in ("world", "renderer", "app")), f"Public scene header did not rebuild consumers: {counts}")
            require(re.search(r"Linking CXX executable .*SymoCraft.exe", log), "Changed library did not relink the game.")
            results.append({"Probe": name, "Header": relative, "Sha256": timestamp["sha256"], "CompiledOwners": counts, "Relinked": True})
    write_json(output / "result.json", results)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("BuildDirectory", "OutputDirectory", "CMakePath"):
        option(parser, name, required=True)
    print(verify(parser.parse_args()))


if __name__ == "__main__":
    cli(main)
