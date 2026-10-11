"""Build and exercise the actual isolated CPU baseline and its dependencies."""
import argparse
from pathlib import Path
import re
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, capture, cli, cmake_options, file_version, load_json, new_directory, option, read_log, require, toolchain, tool_options, write_json


def verify(args):
    root = Path(args.RepositoryRoot).resolve(strict=True)
    output = new_directory(args.OutputDirectory)
    tools = toolchain(args)
    result = {"configuration": args.Configuration, "compiler": tools["compiler"],
              "compiler_version": file_version(tools["compiler"]), "windows_sdk": tools["env"].get("WINDOWSSDKVERSION"),
              "success": False, "steps": []}
    def run(name, program, arguments):
        log = output / (name + ".log")
        record = capture(program, arguments, root, log, env=tools["env"], timeout=600)
        result["steps"].append({"name": name, "program": str(program), "arguments": list(map(str, arguments)),
                                "exit_code": record["exit_code"], "log": str(log)})
        require(not record["timed_out"] and record["exit_code"] == 0, f"{name} failed; see {log}")
        return read_log(log)
    try:
        run("cmake-version", tools["cmake"], ["--version"])
        run("ninja-version", tools["ninja"], ["--version"])
        build = output / "build"
        options = cmake_options(tools, args.Configuration) + ["-DSYMOCRAFT_CPU_ONLY=ON"]
        run("configure", tools["cmake"], ["-S", root, "-B", build, *options])
        run("build", tools["cmake"], ["--build", build, "--parallel", "8"])
        units = load_json(build / "compile_commands.json")
        require(sum(x["file"].replace("\\", "/").endswith("/game/modules/simulation/src/physics_system.cpp") for x in units) == 1,
                "Production physics_system.cpp must be compiled exactly once.")
        run("baseline-repeat", tools["ctest"], ["--test-dir", build, "-R", r"^simulation\.legacy_(physics|gameplay)_baseline$",
            "--repeat", "until-fail:3", "--output-on-failure", "--no-tests=error", "--output-junit", output / "baseline-repeat.xml"])
        for target in ("legacy_physics_baseline_tests", "legacy_gameplay_baseline_tests"):
            commands = run(target + "-commands", tools["ninja"], ["-C", build, "-t", "commands", target])
            require(not re.search(r"symocraft_(platform|renderer|glad|startup|app_)|game[/\\]app[/\\]|SDL3|glfw|d3d12|dxgi|vulkan", commands, re.I),
                    f"Graphics/window dependency in actual baseline commands: {target}")
            run(target + "-output", build / "test/unit/simulation" / (target + ".exe"), [root / "assets/configs/blockFormats.yaml"])
        no_tests = output / "no-tests"
        run("no-tests-configure", tools["cmake"], ["-S", root, "-B", no_tests, *cmake_options(tools, args.Configuration, False), "-DSYMOCRAFT_CPU_ONLY=ON"])
        run("no-tests-build", tools["cmake"], ["--build", no_tests, "--target", "symocraft_simulation", "--parallel", "8"])
        require(not re.search(r"(_tests|fixture|test/unit|test/integration)", read_log(no_tests / "CMakeFiles/TargetDirectories.txt"), re.I), "Test targets leaked into BUILD_TESTING=OFF.")
        require(not re.search(r"legacy_(physics|gameplay)_baseline|cpu_baseline_fixture|[/\\]test[/\\]", read_log(no_tests / "compile_commands.json"), re.I), "Test compilation leaked into BUILD_TESTING=OFF.")
        for directory in (build, no_tests):
            require(not re.search(r"symocraft_(platform|glad|renderer_(legacy_opengl|d3d12|vulkan))\.dir|SDL3[^/]*\.dir|vendor/sdl3", read_log(directory / "CMakeFiles/TargetDirectories.txt"), re.I),
                    f"Native target configured in CPU build: {directory}")
        run("ctest", tools["ctest"], ["--test-dir", build, "--output-on-failure", "--no-tests=error", "--output-junit", output / "ctest.xml"])
        result["boundary_checks"] = ["actual baseline commands have no renderer/app/window/SDK dependencies", "production physics_system.cpp compiled exactly once", "BUILD_TESTING=OFF has no test targets or compilations", "CPU configurations have no native targets"]
        result["success"] = True
    finally:
        write_json(output / "result.json", result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    option(parser, "Configuration", required=True, choices=("Debug", "Release"))
    option(parser, "OutputDirectory", required=True)
    option(parser, "RepositoryRoot", default=str(ROOT))
    tool_options(parser)
    verify(parser.parse_args())


if __name__ == "__main__":
    cli(main)
