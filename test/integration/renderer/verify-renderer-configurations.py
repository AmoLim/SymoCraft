"""Build renderer configurations without relabeling legacy failures as passes."""

import argparse
import json
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "scripts"))
from verification_common import (archive_destination, cache, capture, cmake_options, evidence_sources,
                                 identity, new_directory, read_log, require, tool_options,
                                 toolchain, utc_now, workspace_path, write_json)

MODES = {
    "standalone": ("tools/benchmark", True, []),
    "modern": (".", True, ["RENDER_OPENGL=OFF", "RENDER_D3D12=ON", "BUILD_BENCHMARK=OFF"]),
    "transition": (".", True, ["RENDER_OPENGL=ON", "RENDER_D3D12=ON", "BUILD_BENCHMARK=OFF"]),
    "cpu": (".", True, ["CPU_ONLY=ON"]),
    "benchmark": (".", True, ["BUILD_GAME=OFF", "BUILD_CPU_MODULES=OFF"]),
    "modern-no-tests": (".", False, ["RENDER_OPENGL=OFF", "RENDER_D3D12=ON", "BUILD_BENCHMARK=OFF"]),
    "benchmark-no-tests": (".", False, ["BUILD_GAME=OFF", "BUILD_CPU_MODULES=OFF"]),
    "standalone-no-tests": ("tools/benchmark", False, []),
    "all-off": (".", False, ["RENDER_OPENGL=OFF", "RENDER_D3D12=OFF", "RENDER_VULKAN=OFF", "BUILD_BENCHMARK=OFF"]),
}
LEGACY_MODES = list(MODES)
for name, gl, direct, vulkan in (("d3d12", False, True, False),
                                 ("vulkan", False, False, True),
                                 ("dual", False, True, True),
                                 ("gl", True, False, False)):
    options = [f"RENDER_{key}={'ON' if value else 'OFF'}" for key, value in
               (("OPENGL", gl), ("D3D12", direct), ("VULKAN", vulkan))]
    MODES[f"r4-{name}"] = (".", True, [*options, "BUILD_BENCHMARK=OFF"])
    if name in ("dual", "vulkan"):
        MODES[f"r4-{name}-no-tests"] = (".", False, [*options, "BUILD_BENCHMARK=OFF"])


def check_isolation(build, mode, options):
    graph = (build / "build.ninja").read_text(encoding="utf-8").lower()
    configured = cache(build)
    game = (build / "bin/SymoCraft.exe").exists()
    enabled = {"opengl": game and "RENDER_OPENGL=ON" in options,
               "d3d12": game and "RENDER_D3D12=ON" in options,
               "vulkan": game and "RENDER_VULKAN=ON" in options}
    if mode == "transition":
        enabled["opengl"] = True
    targets = {"opengl": "symocraft_renderer_legacy_opengl", "d3d12": "symocraft_renderer_d3d12",
               "vulkan": "symocraft_renderer_vulkan"}
    assets = build / "bin/assets/shaders"
    for backend, present in enabled.items():
        require((targets[backend] in graph) == present, f"Wrong backend target: {backend}")
        staged = list(assets.rglob("*.glsl")) if backend == "opengl" else list((assets / backend).glob("*"))
        require(len(staged) == ((4 if backend == "opengl" else 5) if present else 0), f"Wrong staged assets: {backend}: {staged}")
    require(("symocraft_glad" in graph) == enabled["opengl"], "Unexpected GLAD dependency")
    if not enabled["vulkan"]:
        require(not any(k.startswith("SYMOCRAFT_VULKAN_") for k in configured), "Disabled Vulkan discovered SDK")
        require("vulkansdk" not in graph and "vulkan-1.lib" not in graph, "Disabled Vulkan leaked SDK/link dependency")
    if not enabled["d3d12"]:
        require(not any(k in configured for k in ("SYMOCRAFT_DXC", "SYMOCRAFT_D3D12_HEADERS", "SYMOCRAFT_DXGI_HEADERS",
                    "SYMOCRAFT_D3D12_LIBRARY", "SYMOCRAFT_DXGI_LIBRARY", "SYMOCRAFT_DXGUID_LIBRARY")), "Disabled D3D12 discovered SDK")
        require(not any(x in graph for x in ("d3d12.lib", "dxgi.lib", "dxguid.lib")), "Disabled D3D12 linked native libraries")
    require("vulkan-1.lib" not in graph, "Vulkan must use dynamic loader dispatch, not an import library")
    if not game:
        require(not any(x in graph for x in ("symocraft_platform", "sdl3-static")),
                "Independent build configured a native platform/window dependency")
    return enabled


def sources():
    return evidence_sources([ROOT / p for p in ("game", "cmake", "tools/benchmark", "test", "assets/shaders/hlsl")],
                            [ROOT / "CMakeLists.txt", ROOT / "scripts/package-d3d12-candidate.py"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-parent", required=True)
    parser.add_argument("--evidence-directory", required=True)
    parser.add_argument("--modes", nargs="+", choices=MODES, default=LEGACY_MODES)
    parser.add_argument("--configurations", nargs="+", choices=("Debug", "Release"), default=["Debug", "Release"])
    parser.add_argument("--dxc", default="C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64/dxc.exe")
    parser.add_argument("--vulkan-headers")
    parser.add_argument("--spirv-dxc")
    parser.add_argument("--jobs", type=int, choices=range(1, 17), default=8)
    tool_options(parser)
    args = parser.parse_args()
    builds = workspace_path(args.build_parent, ROOT / "out")
    evidence = (workspace_path(args.evidence_directory, ROOT / "out") if any(m.startswith("r4-") for m in args.modes)
                else archive_destination(args.evidence_directory))
    require(not builds.exists(), "Choose a fresh build parent.")
    tools = toolchain(args)
    # Disabled backends must not rely on the SDK environment hint or PATH discovery.
    tools["env"].pop("VULKAN_SDK", None)
    tools["env"]["PATH"] = ";".join(p for p in tools["env"]["PATH"].split(";") if "vulkansdk" not in p.lower())
    new_directory(builds)
    new_directory(evidence)
    original = sources()
    write_json(evidence / "source-inputs.json", original)
    report = {"schema": "symocraft.t3.renderer-configuration-matrix", "version": 2,
              "started_utc": utc_now(), "completed": False, "r2_complete": False,
              "r3_acceptance": False, "hardware_faults_exercised": False,
              "formal_visual_or_performance_acceptance": False, "source_unchanged": False,
              "tools": [identity(Path(tools[k])) for k in ("cmake", "ninja", "compiler", "ctest")],
              "cases": [], "failure": None}

    def run(case, name, program, arguments, timeout=600):
        record = capture(program, arguments, ROOT, evidence / f"{case['name']}-{name}.log",
                         env=tools["env"], timeout=timeout)
        record.update(program=str(program), arguments=list(map(str, arguments)))
        case[name] = record
        write_json(evidence / "matrix-result.json", report)
        print(f"{case['name']} {name}: exit={record['exit_code']} timeout={record['timed_out']}", flush=True)
        require(not record["timed_out"], f"{case['name']} {name} timed out")
        return record["exit_code"]

    try:
        for configuration in args.configurations:
            for mode in args.modes:
                source, testing, options = MODES[mode]
                name = f"{mode}-{configuration.lower()}"
                build = builds / name
                case = {"name": name, "build_directory": str(build), "testing": testing, "passed": False}
                report["cases"].append(case)
                configure = ["-S", ROOT / source, "-B", build, *cmake_options(tools, configuration, testing),
                             *(f"-DSYMOCRAFT_{option}" for option in options)]
                if "RENDER_D3D12=ON" in options:
                    configure.append(f"-DSYMOCRAFT_DXC={args.dxc}")
                if "RENDER_VULKAN=ON" in options:
                    require(args.vulkan_headers and args.spirv_dxc, "Enabled Vulkan needs explicit headers and SPIR-V compiler")
                    configure += [f"-DSYMOCRAFT_VULKAN_HEADERS={args.vulkan_headers}", f"-DSYMOCRAFT_VULKAN_DXC={args.spirv_dxc}"]
                exit_code = run(case, "configure", tools["cmake"], configure)
                if mode == "all-off":
                    case["passed"] = exit_code != 0 and "The game requires at least one implemented renderer" in read_log(evidence / f"{name}-configure.log")
                    require(case["passed"], "All-off game did not fail with the named error")
                    continue
                require(exit_code == 0, f"Configure failed: {name}")
                require(run(case, "build", tools["cmake"], ["--build", build, "--parallel", args.jobs], 1200) == 0, f"Build failed: {name}")
                if testing:
                    xml = evidence / f"{name}-ctest.xml"
                    exit_code = run(case, "ctest", tools["ctest"], ["--test-dir", build, "--output-on-failure",
                                    "--no-tests=error", "-j", "4", "--output-junit", xml])
                    tests = list(ET.parse(xml).getroot().iter("testcase"))
                    failed = [t.attrib["name"] for t in tests if t.find("failure") is not None]
                    disabled = sum(t.find("skipped") is not None or t.attrib.get("status") == "disabled" for t in tests)
                    case["tests"] = {"failed": failed, "disabled": disabled, "passed": len(tests) - len(failed) - disabled}
                    if failed:
                        events = [json.loads(line) for line in read_log(evidence / f"{name}-ctest.log").splitlines()
                                  if line.startswith('{"kind":"publication_test"')]
                        unexpected = [e for e in events if e.get("event") == "unexpected_failure" and e.get("suite") in failed]
                        case["legacy_failures"] = unexpected
                        require(set(failed) <= {"foundation.files", "performance.export"}
                                and set(e.get("suite") for e in unexpected) == set(failed)
                                and all(e.get("numeric_win32") == 1175 for e in unexpected), f"Unexpected test failures: {name}: {failed}")
                    require(exit_code == 0 or bool(failed), f"CTest failed without named cases: {name}")
                    passed = exit_code == 0
                else:
                    exit_code = run(case, "test_inventory", tools["ctest"], ["--test-dir", build, "--show-only=json-v1"])
                    inventory = json.loads(read_log(evidence / f"{name}-test_inventory.log"))
                    require(exit_code == 0 and inventory["tests"] == [], "Tests-off build configured test cases")
                    passed = True
                enabled = check_isolation(build, mode, options)
                case["enabled_backends"] = enabled
                if any(enabled.values()):
                    exe = build / "bin/SymoCraft.exe"
                    for backend, present in enabled.items():
                        expected = 0 if present else 3
                        require(run(case, f"preflight-{backend}", exe, ["--renderer", backend, "--check-assets"], 30) == expected,
                                f"Selection/fallback regression: {backend}")
                        if not present:
                            require("category=Unsupported" in read_log(evidence / f"{name}-preflight-{backend}.log"), "Missing named unavailable-backend error")
                require(sources() == original, "Sources changed during matrix; preserve partial results and rerun")
                case["passed"] = passed
        report["source_unchanged"] = sources() == original
        require(report["source_unchanged"], "Sources changed before matrix completion")
        report["completed"] = True
        report["all_passed"] = all(c["passed"] for c in report["cases"])
    except Exception as error:
        report["failure"] = str(error)
        raise
    finally:
        report["finished_utc"] = utc_now()
        write_json(evidence / "matrix-result.json", report)


if __name__ == "__main__":
    main()
