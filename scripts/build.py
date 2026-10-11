"""Configure, build, test and install with an isolated MSVC x64 environment."""

import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

from scripts_common import ROOT, cli, hidden_process, integer, option


def resolve_tool(explicit, name, clion, relative):
    if explicit or clion:
        path = Path(explicit) if explicit else Path(clion) / relative
        path = path.resolve(strict=True)
        if not path.is_file():
            raise ValueError(f"Tool is not a file: {path}")
        return str(path)
    path = shutil.which(name)
    if not path:
        raise ValueError(f"Cannot find {name}. Supply --clion-path (or CLION_HOME), or the explicit tool path.")
    return path


def developer_environment(visual_studio, toolset, inherited=None):
    env = dict(os.environ if inherited is None else inherited)
    script = Path(visual_studio) / "Common7/Tools/VsDevCmd.bat"
    if not script.is_file():
        raise ValueError(f"Visual Studio developer command is missing: {script}")
    if toolset and not re.fullmatch(r"14\.[0-9]+(?:\.[0-9]+)?", toolset):
        raise ValueError("ToolsetVersion must be 14.N or 14.N.N.")
    # cmd is required only to load Microsoft's supplied developer batch file.
    # Disable delayed expansion so spaces, Unicode and exclamation marks survive.
    if any(char in str(script) for char in ('"', '%', '\r', '\n')):
        raise ValueError("Unsupported characters in the Visual Studio installation path.")
    arguments = "-arch=x64 -host_arch=x64"
    if toolset:
        arguments += f" -vcvars_ver={toolset}"
    marker = "__SYMOCRAFT_DEVELOPER_ENVIRONMENT__"
    command = f'call "{script}" {arguments} >nul && echo {marker} && set'
    invocation = subprocess.list2cmdline([env.get("COMSPEC", "cmd.exe")]) + ' /d /u /v:off /s /c "' + command + '"'
    result = subprocess.run(invocation,
                            env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            **hidden_process())
    if result.returncode:
        diagnostic = (result.stdout + result.stderr).decode("utf-16-le", errors="replace").strip()
        raise RuntimeError(f"Visual Studio developer command failed (exit {result.returncode}): {diagnostic}")
    output = result.stdout.decode("utf-16-le")
    if marker not in output:
        raise RuntimeError("Visual Studio did not provide a developer environment.")
    entries = output.split(marker, 1)[1].splitlines()
    return {key.upper(): value for line in entries if "=" in line and not line.startswith("=")
            for key, value in [line.split("=", 1)]}


def build_commands(args, cmake, ninja, compiler, ctest):
    preset = "windows-" + args.Configuration.lower()
    directory = (ROOT / (args.BuildDirectory or f"out/build/{preset}")).resolve()
    commands = [[cmake, "--preset", preset, "-B", str(directory), f"-DCMAKE_MAKE_PROGRAM={ninja}",
                 f"-DCMAKE_C_COMPILER={compiler}", f"-DCMAKE_CXX_COMPILER={compiler}",
                 f"-DPython3_EXECUTABLE={sys.executable}"]]
    if args.Action != "Configure":
        target = [str(directory)] if args.BuildDirectory else ["--preset", preset]
        commands.append([cmake, "--build", *target, "--parallel", str(args.Jobs)])
    if args.Action in ("Test", "Install"):
        # Custom directories must not send CTest logs into the preset binaryDir.
        target = ["--test-dir", str(directory), "-C", args.Configuration] if args.BuildDirectory else ["--preset", preset]
        commands.append([ctest, *target, "--output-on-failure", "--no-tests=error"])
    if args.Action == "Install":
        commands.append([cmake, "--install", str(directory)])
    return commands


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    option(result, "Configuration", choices=("Debug", "Release"), default="Debug")
    option(result, "Action", choices=("Configure", "Build", "Test", "Install"), default="Test")
    option(result, "CLionPath", default=os.environ.get("CLION_HOME"))
    for name in ("CMakePath", "NinjaPath", "VisualStudioPath", "BuildDirectory", "ToolsetVersion"):
        option(result, name)
    option(result, "Jobs", type=integer(1, 64), default=8)
    return result


def main():
    args = parser().parse_args()
    if os.name != "nt" or struct.calcsize("P") != 8:
        raise RuntimeError("Use 64-bit Python on Windows with MSVC 2022 and the Windows SDK installed.")
    cmake = resolve_tool(args.CMakePath, "cmake.exe", args.CLionPath, "bin/cmake/win/x64/bin/cmake.exe")
    ninja = resolve_tool(args.NinjaPath, "ninja.exe", args.CLionPath, "bin/ninja/win/x64/ninja.exe")
    ctest = str(Path(cmake).with_name("ctest.exe").resolve(strict=True))
    studio = args.VisualStudioPath
    if not studio:
        vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
        result = subprocess.run([str(vswhere), "-latest", "-products", "*", "-version", "[17.0,18.0)",
                                 "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                 "-property", "installationPath", "-utf8"],
                                capture_output=True, encoding="utf-8", check=True, **hidden_process())
        studio = result.stdout.strip()
        if not studio:
            raise RuntimeError("No complete Visual Studio 2022 C++ installation found. Install the C++ tools and Windows SDK.")
    studio = Path(studio).resolve(strict=True)
    env = developer_environment(studio, args.ToolsetVersion)
    env["PATH"] = os.pathsep.join((str(Path(cmake).parent), str(Path(ninja).parent), env.get("PATH", "")))
    env["VSLANG"] = "1033"
    compiler = shutil.which("cl.exe", path=env["PATH"])
    if not compiler:
        raise RuntimeError("The Visual Studio x64 developer environment did not provide cl.exe.")
    print(f"Visual Studio: {studio}\nCompiler: {compiler}", flush=True)
    commands = [[cmake, "--version"], [ninja, "--version"], *build_commands(args, cmake, ninja, compiler, ctest)]
    for command in commands:
        print("Running: " + subprocess.list2cmdline(command), flush=True)
        subprocess.run(command, cwd=ROOT, env=env, check=True)


if __name__ == "__main__":
    cli(main)
