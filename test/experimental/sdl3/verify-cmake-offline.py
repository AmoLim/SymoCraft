"""Audit actual CMake 3.22 SDL configure/build/test/install without GUI runs."""
import argparse
from datetime import datetime
from pathlib import Path
import re
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
import json
from verification_common import ROOT, cache, cli, cmake_options, file_path, files, identity, load_json, logged, new_directory, option, read_log, require, sha256, snapshot_identity, tool_options, toolchain, workspace_path, write_json
from scripts_common import integer

NETWORK = re.compile(r"FetchContent|ExternalProject|file\s*\(\s*DOWNLOAD|DOWNLOAD_COMMAND|\b(curl|wget)\b|\bgit(?:\.exe)?\s+(clone|fetch|pull)", re.I)


def audit_trace(path):
    network, processes, count = [], [], 0
    with Path(path).open(encoding="utf-8-sig") as stream:
        for line in stream:
            entry = json.loads(line)
            if "cmd" not in entry:
                continue
            count += 1
            command, arguments = entry["cmd"].lower(), entry["args"]
            record = {"file": entry["file"], "line": entry["line"], "command": command, "args": arguments}
            if ((command == "file" and arguments and arguments[0].upper() == "DOWNLOAD") or
                command in {"externalproject_add", "fetchcontent_declare", "fetchcontent_populate", "fetchcontent_makeavailable"} or
                (command == "execute_process" and re.search(r"https?://|\b(curl|wget|Invoke-WebRequest)\b|\bgit(?:\.exe)?\b[^;]*(clone|fetch|pull)", " ".join(arguments), re.I))):
                network.append(record)
            if command == "execute_process":
                processes.append({"file": entry["file"], "line": entry["line"], "args": arguments})
    return {"executed_commands": count, "network_or_population_commands": network, "execute_process_calls": processes}


def inputs():
    paths = ["vendor/sdl3", "vendor/glad", "vendor/KHR", "game/modules/platform/include", "cmake/ThirdPartyHeaders.cmake",
             "scripts/experimental_build.py", "scripts/verification_common.py", "scripts/scripts_common.py", "scripts/build.py"]
    paths += ["test/experimental/sdl3/" + name for name in ("CMakeLists.txt", "main.cpp", "input_candidate.h", "input_candidate_tests.cpp", "runtime_input_tests.cpp", "build.py", "verify-cmake-offline.py")]
    return {entry["path"]: entry["sha256"].upper() for entry in snapshot_identity(paths)}


def matches(paths):
    return [{"Path": str(path), "LineNumber": number, "Line": line} for path in paths
            for number, line in enumerate(read_log(path).splitlines(), 1) if NETWORK.search(line)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    tool_options(parser)
    parser.set_defaults(CMakePath="out/m3-t2/tools/cmake-3.22.6/portable/cmake-3.22.6-windows-x86_64/bin/cmake.exe")
    option(parser, "BuildRoot")
    option(parser, "Configurations", nargs="+", choices=["Debug", "Release"], default=["Debug", "Release"])
    option(parser, "Modes", nargs="+", choices=["gl", "vulkan"], default=["gl", "vulkan"])
    option(parser, "HeadersRoot", default="vendor/sdl3/src/video/khronos")
    option(parser, "Jobs", type=integer(1, 64), default=4)
    args = parser.parse_args()
    args.CMakePath = str(workspace_path(args.CMakePath))
    tools = toolchain(args)
    headers = workspace_path(args.HeadersRoot)
    if "vulkan" in args.Modes:
        file_path(headers / "vulkan/vulkan_core.h")
    output = new_directory(workspace_path(args.BuildRoot or ("out/m3-t2/cmake-3.22-offline-audit-" + datetime.now().strftime("%Y%m%d-%H%M%S")), ROOT / "out/m3-t2"))
    env = dict(tools["env"])
    for key in ("http_proxy", "https_proxy", "all_proxy", "HTTP_PROXY", "HTTPS_PROXY", "ALL_PROXY"):
        env[key] = "http://127.0.0.1:1"
    env.update(NO_PROXY="", no_proxy="", GIT_ALLOW_PROTOCOL="file")
    summary = {"scope": "Isolated SDL preparation project, not production migration acceptance",
               "network_limit": "Process-only proxy refusal and Git file-only transport are partial constraints, not a network sandbox. Trace/static/generated-rule inspection does not block tools ignoring those settings.",
               "cmake_path": tools["cmake"], "cmake_sha256": sha256(tools["cmake"]).upper(), "cmake_version": "",
               "ninja_path": tools["ninja"], "ninja_sha256": sha256(tools["ninja"]).upper(), "ninja_version": "",
               "compiler": tools["compiler"], "compiler_sha256": sha256(tools["compiler"]).upper(), "msvc_toolset": args.ToolsetVersion,
               "windows_sdk_version": env.get("WINDOWSSDKVERSION"), "vulkan_headers": str(headers), "vulkan_headers_are_installed_sdk": False,
               "continuous_input_comparison": "Explicitly OFF; no frozen GLFW/camera archive prerequisite",
               "executed_ctest_filter": "^sdl3-input-candidate$; runtime/window targets compile but are not launched",
               "proxy": "http://127.0.0.1:1", "git_allow_protocol": "file", "static_cmake_files": [], "cases": [], "completed": False, "error": None}
    for key, path, pattern in [("sdl_version_macros", ROOT / "vendor/sdl3/include/SDL3/SDL_version.h", r"^#define SDL_(MAJOR|MINOR|MICRO)_VERSION "),
                               ("sdl_revision_macros", ROOT / "vendor/sdl3/include/SDL3/SDL_revision.h", r"^#define SDL_REVISION "),
                               ("vulkan_header_version", headers / "vulkan/vulkan_core.h", r"^#define VK_HEADER_VERSION ")]:
        summary[key] = [line for line in read_log(path).splitlines() if re.search(pattern, line)] if path.exists() else []
    try:
        summary["cmake_version"] = logged(tools["cmake"], ["--version"], output / "cmake-version.log", env=env)
        require(re.match(r"cmake version 3\.22\.", summary["cmake_version"]), "This audit requires actual CMake 3.22.x.")
        summary["ninja_version"] = logged(tools["ninja"], ["--version"], output / "ninja-version.log", env=env)
        before = inputs()
        write_json(output / "inputs-sha256.json", before)
        cmake_files = [Path(__file__).with_name("CMakeLists.txt"), ROOT / "test/experimental/input-comparison/comparison.cmake", ROOT / "cmake/ThirdPartyHeaders.cmake", ROOT / "vendor/sdl3/CMakeLists.txt"]
        cmake_files += [p for p in files(ROOT / "vendor/sdl3/cmake") if p.name == "CMakeLists.txt" or re.search(r"\.cmake(?:\.in)?$", p.name)]
        summary["static_cmake_files"] = list(map(str, sorted(set(cmake_files))))
        write_json(output / "static-download-audit.json", matches(cmake_files))
        for configuration in args.Configurations:
            for mode in args.Modes:
                name = configuration.lower() + "-" + mode
                case_root = new_directory(output / name)
                build, install = case_root / "build", case_root / "install"
                query = build / ".cmake/api/v1/query/client-sdl3-audit"
                query.mkdir(parents=True)
                write_json(query / "query.json", {"requests": [{"kind": "codemodel", "version": 2}, {"kind": "cache", "version": 2}, {"kind": "cmakeFiles", "version": 1}]})
                case = {"name": name, "configuration": configuration, "mode": mode, "completed": False, "changed_inputs": [], "trace": None,
                        "targets": [], "source_coverage": {}, "ctest_count": 0, "synthetic_checks": 0, "install_files": [], "cache_options": {}}
                summary["cases"].append(case)
                trace = case_root / "configure-trace.jsonl"
                arguments = ["--trace-expand", "--trace-format=json-v1", f"--trace-redirect={trace}", "-S", Path(__file__).parent, "-B", build, *cmake_options(tools, configuration),
                             f"-DCMAKE_INSTALL_PREFIX={install.as_posix()}", f"-DSYMOCRAFT_SDL3_PROBE_VULKAN={'ON' if mode == 'vulkan' else 'OFF'}",
                             f"-DSYMOCRAFT_VULKAN_HEADERS_ROOT={headers.as_posix() if mode == 'vulkan' else ''}", "-DSYMOCRAFT_SDL3_INPUT_COMPARISON=OFF",
                             "-DFETCHCONTENT_FULLY_DISCONNECTED=ON", "-DFETCHCONTENT_UPDATES_DISCONNECTED=ON"]
                logged(tools["cmake"], arguments, case_root / "configure.log", env=env)
                case["trace"] = audit_trace(trace)
                write_json(case_root / "trace-audit.json", case["trace"])
                require(not case["trace"]["network_or_population_commands"], "Executed download/population command.")
                rules = matches([build / "build.ninja", build / "CMakeFiles/rules.ninja"])
                write_json(case_root / "generated-rule-download-audit.json", rules)
                require(not rules, "Possible generated download rule.")
                targets = [load_json(p) for p in sorted((build / ".cmake/api/v1/reply").glob("target-*.json"))]
                case["targets"] = sorted({t["name"] for t in targets})
                coverage = {"manifest_covered": set(), "generated_build_inputs": set(), "missing_manifest_inputs": set()}
                for target in targets:
                    for source in target.get("sources", []):
                        path = (Path(__file__).parent / source["path"]).resolve()
                        if path.is_relative_to(build):
                            coverage["generated_build_inputs"].add(str(path))
                        elif path.is_relative_to(ROOT) and path.relative_to(ROOT).as_posix() in before:
                            coverage["manifest_covered"].add(path.relative_to(ROOT).as_posix())
                        else:
                            coverage["missing_manifest_inputs"].add(str(path))
                case["source_coverage"] = {k: sorted(v) for k, v in coverage.items()}
                require(not coverage["missing_manifest_inputs"], "File API found untracked source inputs.")
                case["cache_options"] = {k: v for k, v in cache(build).items() if re.match(r"^(SDL_|SYMOCRAFT_|CMAKE_BUILD_TYPE$|CMAKE_MSVC_RUNTIME_LIBRARY$)", k)}
                logged(tools["cmake"], ["--build", build, "--parallel", args.Jobs], case_root / "build.log", env=env, timeout=1200)
                text = logged(tools["ctest"], ["--test-dir", build, "-C", configuration, "-R", "^sdl3-input-candidate$", "-V", "--output-on-failure", "--no-tests=error"], case_root / "ctest.log", env=env)
                count = re.search(r"out of (\d+)", text)
                checks = re.search(r"synthetic input candidate:\s*(\d+) checks passed", text)
                require(count and checks, "CTest did not report its real test/check counts.")
                case.update(ctest_count=int(count[1]), synthetic_checks=int(checks[1]))
                logged(tools["cmake"], ["--install", build, "--config", configuration], case_root / "install.log", env=env)
                case["install_files"] = [identity(p, install) for p in files(install)]
                after = inputs()
                case["changed_inputs"] = sorted(k for k in before.keys() | after.keys() if before.get(k) != after.get(k))
                write_json(case_root / "inputs-after-sha256.json", after)
                require(not case["changed_inputs"], "Inputs changed during audit.")
                case["completed"] = True
                write_json(output / "summary.json", summary)
        summary["completed"] = True
    except (ValueError, OSError, RuntimeError) as error:
        summary["error"] = str(error)
        raise
    finally:
        write_json(output / "summary.json", summary)


if __name__ == "__main__":
    cli(main)
