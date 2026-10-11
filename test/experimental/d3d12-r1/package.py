"""Install a verified Release R1 fixture with immutable portable provenance."""
import argparse
from pathlib import Path
import shutil
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, assert_hash, cache, cli, file_path, files, identity, load_json, load_script, logged, new_directory, option, require, sha256, utc_now, workspace_path, write_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "BuildDirectory", required=True)
    option(parser, "OutputDirectory", required=True)
    option(parser, "CMakePath")
    args = parser.parse_args()
    build = workspace_path(args.BuildDirectory, ROOT / "out/m3-t3")
    output = workspace_path(args.OutputDirectory, ROOT / "out/m3-t3/package")
    require(not build.is_relative_to(output) and not output.is_relative_to(build), "Keep package and build separate.")
    saved = cache(build)
    require(saved.get("CMAKE_BUILD_TYPE") == "Release" and Path(saved.get("CMAKE_HOME_DIRECTORY", "")).resolve() == Path(__file__).parent.resolve(), "Package only this isolated Release project; Debug CRT is not redistributable.")
    identities = load_json(build / "build-identities.json")
    imports = load_json(build / "production-imports.json")
    require(imports["configuration"] == "Release", "Production imports must be Release.")
    load_script(Path(__file__).with_name("verify-probes.py")).verify_origin(build, imports, identities)
    cmake = file_path(args.CMakePath or saved["CMAKE_COMMAND"])
    output = new_directory(output)
    logged(cmake, ["--install", build, "--config", "Release", "--component", "R1Fixture", "--prefix", output], output / "install.log")
    for name in ("SymoCraftD3D12R1Probe.exe", "shaders/probe-vs.cso", "shaders/probe-ps.cso", "build-identities.json", "production-imports.json",
                 "verify-probes.py", "verification_common.py", "scripts_common.py", "licenses/SDL3-LICENSE.txt", "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll"):
        file_path(output / name)
    assert_hash(output / "SymoCraftD3D12R1Probe.exe", identities["executable_sha256"])
    assert_hash(output / "build-identities.json", sha256(build / "build-identities.json"))
    assert_hash(output / "production-imports.json", sha256(build / "production-imports.json"))
    for entry in identities["shaders"]:
        assert_hash(output / "shaders" / Path(entry["path"]).name, entry["sha256"])
    (output / "README.txt").write_text("SymoCraft T3 R1 portable discrete-adapter experiment\n\nWindows x64, Python 3.10+, a real D3D12 GPU and installed Graphics Tools Debug Layer are required. No WARP or backend fallback.\n\npython verify-probes.py --executable SymoCraftD3D12R1Probe.exe --output-directory evidence/discrete-release-001 --package-manifest package-manifest.json --run-timeout-probe\n\nUse fresh evidence paths. Review all five PNGs visually. Timeout is explicit injection, not actual device loss. Origin paths are provenance only. Not Renderer v1 freeze, gameplay, Q06/Q07 budget or T3 acceptance. Return the complete evidence directory.\n", encoding="utf-8")
    payload = [identity(p, output) for p in files(output)]
    manifest = {"schema_version": 1, "configuration": "Release", "created_utc": utc_now(), "origin_build": {"directory": str(build),
                "build_identities_sha256": sha256(build / "build-identities.json"), "production_imports_sha256": identities["production_imports_sha256"], "executable_sha256": identities["executable_sha256"]},
                "payload": payload, "required_debug_layer": True, "software_adapter_allowed": False, "scope": "Isolated candidate R1 fixture; no game, debug-layer redistribution, freeze or acceptance."}
    write_json(output / "package-manifest.json", manifest)
    print(f"Packaged {len(payload)} payload files: {output}")


if __name__ == "__main__":
    cli(main)
