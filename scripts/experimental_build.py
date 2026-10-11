"""MSVC orchestration shared by the isolated native experiment projects."""

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
from verification_common import ROOT, cache, cmake_options, evidence_sources, file_path, file_version, hidden_process, identity, logged, new_directory, option, reject_links, require, sha256, tool_options, toolchain, utc_now, workspace_path, write_json
from scripts_common import integer


def assert_cache(directory, source, configuration, expected=None):
    if not (directory / "CMakeCache.txt").exists():
        return
    saved = cache(directory)
    require(Path(saved.get("CMAKE_HOME_DIRECTORY", "")).resolve() == source.resolve()
            and saved.get("CMAKE_BUILD_TYPE") == configuration,
            "Existing cache belongs to another source/configuration; choose a new build directory.")
    for key, value in (expected or {}).items():
        actual = saved.get(key)
        if key.endswith("_BUILD"):
            require(actual and Path(actual).resolve() == Path(value).resolve(), "Production imports changed; use a new cache.")
        else:
            require(actual == value, f"Cache option {key} changed; use a separate build directory.")


def version(program, arguments, env):
    try:
        result = subprocess.run([str(program), *arguments], capture_output=True, timeout=30, env=env, **hidden_process())
        return {"exit_code": result.returncode, "output": "\n".join((result.stdout + result.stderr).decode("utf-8", "replace").splitlines()[:6])}
    except (OSError, subprocess.SubprocessError) as error:
        return {"exit_code": None, "output": str(error)}


def run(kind, source):
    parser = argparse.ArgumentParser(description="Build isolated Windows x64/MSVC native experiment; Python 3.10+.")
    option(parser, "Configuration", choices=["Debug", "Release"], default="Debug")
    option(parser, "Action", choices=["Configure", "Build", "Test"] if kind == "sdl3" else ["Configure", "Build"], default="Test" if kind == "sdl3" else "Build")
    option(parser, "BuildDirectory", required=kind == "contract")
    option(parser, "Jobs", type=integer(1, 64), default=8)
    tool_options(parser)
    if kind in {"sdl3", "platform"}:
        option(parser, "Vulkan", action="store_true")
        option(parser, "HeadersRoot")
        option(parser, "InputComparison" if kind == "sdl3" else "InputValidation", action="store_true")
    if kind in {"platform", "d3d12"}:
        option(parser, "ProductionBuild", required=kind == "platform")
    if kind == "d3d12":
        option(parser, "BuildPlatform", action="store_true")
        option(parser, "DxcPath")
    args = parser.parse_args()
    source = Path(source).resolve()
    configuration = args.Configuration
    production, expected, additional = None, {}, []
    if kind == "sdl3":
        require(not args.InputComparison or configuration == "Release", "Input comparison requires the frozen Release camera.")
        mode = configuration.lower() + ("-vulkan" if args.Vulkan else "-gl")
        default = "out/m3-t2/build/sdl3-" + mode
        expected["SYMOCRAFT_SDL3_PROBE_VULKAN"] = "ON" if args.Vulkan else "OFF"
        additional = [f"-DCMAKE_INSTALL_PREFIX={(ROOT / ('out/m3-t2/install/sdl3-' + mode)).as_posix()}",
                      f"-DSYMOCRAFT_SDL3_INPUT_COMPARISON={'ON' if args.InputComparison else 'OFF'}"]
    elif kind == "platform":
        default = f"out/m3-t2/{'s3' if args.InputValidation else 's2'}/probe-{configuration.lower()}-001"
        production = workspace_path(args.ProductionBuild, ROOT / "out")
        expected["SYMOCRAFT_PLATFORM_PRODUCTION_BUILD"] = str(production)
        expected["SYMOCRAFT_PLATFORM_PROBE_VULKAN"] = "ON" if args.Vulkan else "OFF"
        expected["SYMOCRAFT_PLATFORM_PROBE_INPUT"] = "ON" if args.InputValidation else "OFF"
    elif kind == "d3d12":
        default = f"out/m3-t3/r1-{configuration.lower()}-001"
        production = workspace_path(args.ProductionBuild or f"out/m3-t3/platform-{configuration.lower()}", ROOT / "out")
        expected["SYMOCRAFT_R1_PRODUCTION_BUILD"] = str(production)
        if args.BuildPlatform:
            require(production.is_relative_to(ROOT / "out/m3-t3"), "BuildPlatform only writes isolated out/m3-t3 caches.")
        else:
            file_path(production / "CMakeCache.txt")
        assert_cache(production, ROOT, configuration)
    else:
        default = None
    build = workspace_path(args.BuildDirectory or default, ROOT / ("out/m3-t3" if kind == "contract" else "out"))
    reject_links(build)
    require(build != production, "Keep experiment and production builds separate.")
    if kind == "platform":
        file_path(production / "CMakeCache.txt")
        require(cache(production).get("CMAKE_BUILD_TYPE") == configuration, "Production configuration does not match probe configuration.")
    if kind in {"sdl3", "platform"}:
        require(bool(args.HeadersRoot) == args.Vulkan, "Provide HeadersRoot only with Vulkan, and explicitly when enabled.")
        headers = workspace_path(args.HeadersRoot) if args.Vulkan else None
        if headers:
            file_path(headers / "vulkan/vulkan_core.h")
        additional.append(f"-DSYMOCRAFT_VULKAN_HEADERS_ROOT={headers.as_posix() if headers else ''}")
    assert_cache(build, source, configuration, expected)
    tools = toolchain(args)
    build.mkdir(parents=True, exist_ok=True)
    logs = new_directory(build / ("python-build-" + utc_now().replace(":", "").replace("+", "")))
    common = cmake_options(tools, configuration)
    if kind == "d3d12":
        dxc = args.DxcPath or shutil.which("dxc.exe", path=tools["env"]["PATH"])
        if not dxc:
            candidates = sorted((Path(tools["env"].get("WINDOWSSDKDIR", "")) / "bin").glob("*/x64/dxc.exe"), reverse=True)
            dxc = candidates[0] if candidates else None
        require(dxc, "DXC unavailable; provide DxcPath. There is no FXC fallback.")
        dxc = file_path(dxc)
        additional.append(f"-DSYMOCRAFT_R1_DXC={dxc.as_posix()}")
        if args.BuildPlatform:
            logged(tools["cmake"], ["-S", ROOT, "-B", production, *cmake_options(tools, configuration, False),
                   "-DSYMOCRAFT_BUILD_GAME=ON", "-DSYMOCRAFT_BUILD_BENCHMARK=OFF"], logs / "platform-configure.log", env=tools["env"])
            logged(tools["cmake"], ["--build", production, "--target", "symocraft_platform", "--parallel", args.Jobs], logs / "platform-build.log", env=tools["env"])
    logged(tools["cmake"], ["-S", source, "-B", build, *common, *[f"-D{k}={str(v).replace(chr(92), '/')}" for k, v in expected.items()], *additional], logs / "configure.log", env=tools["env"])
    if args.Action != "Configure" or kind == "contract":
        logged(tools["cmake"], ["--build", build, "--parallel", args.Jobs], logs / "build.log", env=tools["env"], timeout=1200)
    if args.Action == "Test" or kind == "contract":
        logged(tools["ctest"], ["--test-dir", build, "-C", configuration, "--output-on-failure", "--no-tests=error"], logs / "ctest.log", env=tools["env"])
    if kind == "d3d12" and args.Action == "Build":
        executable = file_path(build / "SymoCraftD3D12R1Probe.exe")
        env = tools["env"]
        tc = {"compiler_path": tools["compiler"], "compiler_file_version": file_version(tools["compiler"]),
              "msvc_tools_version": env.get("VCTOOLSVERSION"), "visual_studio_path": tools["studio"], "visual_studio_version": env.get("VISUALSTUDIOVERSION"),
              "windows_sdk_path": env.get("WINDOWSSDKDIR"), "windows_sdk_version": env.get("WINDOWSSDKVERSION"),
              "windows_sdk_library_version": env.get("WINDOWSSDKLIBVERSION"), "universal_crt_version": env.get("UCRTVERSION"),
              "dxc_path": str(dxc), "dxc_file_version": file_version(dxc), "dxc_version": version(dxc, ["--version"], env),
              "cmake_path": tools["cmake"], "cmake_version": version(tools["cmake"], ["--version"], env),
              "ninja_path": tools["ninja"], "ninja_version": version(tools["ninja"], ["--version"], env)}
        if tc["dxc_version"]["exit_code"] != 0:
            tc["dxc_version"] = version(dxc, ["-help"], env)
        document = {"built_utc": utc_now(), "executable": str(executable), "executable_sha256": sha256(executable),
                    "production_imports_sha256": sha256(build / "production-imports.json"), "toolchain": tc,
                    "sources": evidence_sources([source], [Path(__file__), ROOT / "scripts/build.py"]),
                    "shaders": [identity(build / "shaders" / name) for name in ("probe-vs.cso", "probe-ps.cso")]}
        write_json(logs / "build-identities.json", document)
        write_json(build / "build-identities.json", document)
    print(f"Completed {kind} {configuration}; logs: {logs}")
