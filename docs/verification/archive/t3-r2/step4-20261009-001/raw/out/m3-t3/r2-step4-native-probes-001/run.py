"""One bounded evidence run of the actual native fixture, not the game package."""

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import uuid

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent


def identity(path):
    return {"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def main():
    result_path = OUT / "verification.json"
    if result_path.exists():
        raise RuntimeError("This evidence run is already recorded.")
    sources = [ROOT / "test/integration/renderer/d3d12_backend_smoke.cpp"]
    sources += list((ROOT / "game/modules/renderer").rglob("*.cpp"))
    sources += list((ROOT / "game/modules/renderer").rglob("*.h"))
    sources += list((ROOT / "assets/shaders/hlsl").rglob("*hlsl*"))
    before = [identity(path) for path in sorted(sources) if path.is_file()]
    result = {"cases": [], "sources_before": before, "success": False,
              "scope": "Native fixture; not full-game gameplay, actual DeviceLost or in-flight timeout."}
    for configuration in ("debug", "release"):
        build = ROOT / "out/m3-t3/r2-step4-matrix-004" / ("native-" + configuration)
        exe = build / "test/integration/renderer_d3d12_backend_smoke.exe"
        good = build / "bin/assets/shaders/d3d12"
        atlas = ROOT / "assets/textures/texture_atlas.png"
        for mode in ("normal", "missing-shader", "bad-shader"):
            name = configuration + "-" + mode
            shaders = good
            if mode != "normal":
                shaders = OUT / (name + "-shaders")
                shutil.copytree(good, shaders)
                changed = shaders / "blocks.vs.dxil"
                if mode == "missing-shader":
                    changed.unlink()
                else:
                    changed.write_bytes(b"isolated-invalid-dxil")
            arguments = [str(exe), "--output-dir", str(OUT / name), "--shader-root", str(shaders),
                         "--recovery-shader-root", str(good), "--atlas", str(atlas),
                         "--run-id", str(uuid.uuid4()), "--expected-device-name", "NVIDIA GeForce RTX 5070 Ti",
                         "--mode", mode, "--samples", "4", "--vsync", "false"]
            row = {"name": name, "arguments": arguments, "executable": identity(exe),
                   "shader_inputs": [identity(path) for path in sorted(good.glob("*.dxil"))],
                   "atlas": identity(atlas), "exit_code": None, "success": False}
            result["cases"].append(row)
            with (OUT / (name + ".log")).open("wb") as log:
                try:
                    completed = subprocess.run(arguments, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT,
                                               timeout=120, check=False)
                    row["exit_code"] = completed.returncode
                    row["success"] = completed.returncode == 0
                except subprocess.TimeoutExpired:
                    row["timed_out"] = True
            row["executable_unchanged"] = identity(exe) == row["executable"]
            result_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
            print(name, row["exit_code"], flush=True)
    result["sources_after"] = [identity(Path(row["path"])) for row in before]
    result["sources_unchanged"] = result["sources_after"] == before
    result["success"] = result["sources_unchanged"] and all(
        row["success"] and row["executable_unchanged"] for row in result["cases"])
    result_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return 0 if result["success"] else 1


if __name__ == "__main__":
    sys.exit(main())
