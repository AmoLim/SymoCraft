"""Build current-source CPU/native Debug/Release candidates without overwriting evidence."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]


def identity(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": digest.hexdigest()}


def inputs():
    paths = [ROOT / "CMakeLists.txt", ROOT / "scripts/package-d3d12-candidate.py"]
    for scope in ("cmake", "game", "assets", "vendor", "test", "tools"):
        paths.extend(p for p in (ROOT / scope).rglob("*") if p.is_file())
    # Bytecode is not an input; imports must not invalidate their own source guard.
    return [identity(p) for p in sorted(set(paths)) if "__pycache__" not in p.parts]


def save(path, data):
    path.write_text(json.dumps(data, indent=2, ensure_ascii=True) + "\n", encoding="utf-8")


def run(tool, args, log):
    with log.open("w", encoding="utf-8") as output:
        result = subprocess.run([str(tool), *map(str, args)], stdout=output,
                                stderr=subprocess.STDOUT, check=False)
    print(f"{log.parent.name}/{log.name}: exit={result.returncode}", flush=True)
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument("--cmake", type=Path, default=Path("C:/Program Files/CMake/bin/cmake.exe"))
    parser.add_argument("--ninja", type=Path, default=Path("E:/Applications/JetBrains/CLion/bin/ninja/win/x64/ninja.exe"))
    parser.add_argument("--dxc", type=Path, default=Path("C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64/dxc.exe"))
    parser.add_argument("--jobs", type=int, choices=range(1, 17), default=8)
    args = parser.parse_args()
    if os.name != "nt":
        parser.error("The current production MSVC/D3D12 build requires Windows x64.")
    output = args.output_directory.resolve()
    if not output.is_relative_to(ROOT / "out") or output == ROOT / "out":
        parser.error("Output must be a new directory below this repository out directory.")
    for tool in (args.cmake, args.ninja, args.dxc):
        if not tool.is_file():
            parser.error(f"Missing tool: {tool}")
    output.mkdir(parents=True, exist_ok=False)
    ctest = args.cmake.parent / "ctest.exe"
    original = inputs()
    save(output / "source-inputs.json", original)
    result = {"source_root": str(ROOT), "source_input_count": len(original),
              "source_unchanged": False, "completed": False, "cases": [], "failure": None}
    try:
        for configuration in ("Debug", "Release"):
            for mode in ("cpu", "native"):
                build = output / f"{mode}-{configuration.lower()}"
                build.mkdir()
                configure = ["-S", ROOT, "-B", build, "-G", "Ninja",
                             f"-DCMAKE_MAKE_PROGRAM={args.ninja}", f"-DCMAKE_BUILD_TYPE={configuration}",
                             "-DBUILD_TESTING=ON", "-DSYMOCRAFT_BUILD_BENCHMARK=OFF"]
                if mode == "cpu":
                    configure += ["-DSYMOCRAFT_CPU_ONLY=ON"]
                else:
                    configure += ["-DSYMOCRAFT_BUILD_GAME=ON", "-DSYMOCRAFT_RENDER_D3D12=ON",
                                  f"-DSYMOCRAFT_DXC={args.dxc}", f"-DPython3_EXECUTABLE={sys.executable}"]
                case = {"configuration": configuration, "mode": mode, "build_directory": str(build)}
                result["cases"].append(case)
                case["configure_exit"] = run(args.cmake, configure, build / "configure.log")
                if case["configure_exit"]:
                    raise RuntimeError(f"Configure failed: {mode} {configuration}")
                case["build_exit"] = run(args.cmake, ["--build", build, "--parallel", args.jobs], build / "build.log")
                if case["build_exit"]:
                    raise RuntimeError(f"Build failed: {mode} {configuration}")
                if inputs() != original:
                    raise RuntimeError("Build inputs changed; rebuild from a stable source snapshot.")
                case["ctest_exit"] = run(ctest, ["--test-dir", build, "--output-on-failure", "-j", "4",
                    "--output-junit", build / "ctest.xml"], build / "ctest.log")
                tests = list(ET.parse(build / "ctest.xml").getroot().iter("testcase"))
                failed = [t.attrib["name"] for t in tests if t.find("failure") is not None]
                disabled = sum(t.find("skipped") is not None or t.attrib.get("status") == "disabled" for t in tests)
                case.update(known_failures=failed, disabled=disabled, enabled_passed=len(tests)-len(failed)-disabled)
                if case["ctest_exit"] and (not failed or any(name not in {"foundation.files", "performance.export"} for name in failed)):
                    raise RuntimeError(f"Unexpected CTest failure: {mode} {configuration}")
                if failed:
                    events = []
                    for line in (build / "ctest.log").read_text(encoding="utf-8", errors="replace").splitlines():
                        if line.startswith('{"kind":"publication_test"'):
                            events.append(json.loads(line))
                    unexpected = [e for e in events if e.get("event") == "unexpected_failure" and e.get("suite") in failed]
                    case["legacy_numeric_win32"] = [e.get("numeric_win32") for e in unexpected]
                    if set(e.get("suite") for e in unexpected) != set(failed) or any(e.get("numeric_win32") != 1175 for e in unexpected):
                        raise RuntimeError("Publication failure differs from the recorded Win32 1175 risk.")
        result["source_unchanged"] = inputs() == original
        if not result["source_unchanged"]:
            raise RuntimeError("Sources changed before matrix completion.")
        result["completed"] = True
    except Exception as error:
        result["failure"] = str(error)
        raise
    finally:
        save(output / "matrix-result.json", result)


if __name__ == "__main__":
    main()
