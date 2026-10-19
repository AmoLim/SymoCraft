"""Install a fresh real production candidate; record origin only after linking."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import struct
import subprocess
import sys
from datetime import datetime, timezone


ROOT = Path(__file__).resolve().parents[1]
ALLOWED_ROOT = ROOT / "out" / "m3-t3"
SOURCE_TREES = (
    "game", "cmake", "assets", "vendor/sdl3", "vendor/glm", "vendor/stb",
    "vendor/yaml-cpp", "vendor/glad", "vendor/KHR", "vendor/fast_noise_lite",
)
ARTIFACT_PATHS = (
    "bin/SymoCraft.exe", "bin/assets/shaders/d3d12/blocks.vs.dxil",
    "bin/assets/shaders/d3d12/blocks.ps.dxil", "bin/assets/shaders/d3d12/selection.vs.dxil",
    "bin/assets/shaders/d3d12/selection.ps.dxil", "CMakeCache.txt", "build.ninja",
    "CMakeFiles/rules.ninja", "compile_commands.json", "cmake_install.cmake",
    "game/app/cmake_install.cmake", "game/modules/renderer/cmake_install.cmake",
)
ASSETS = (
    "configs/blockFormats.yaml", "shaders/vs_BlockShader.glsl", "shaders/fs_BlockShader.glsl",
    "shaders/vs_FrameShader.glsl", "shaders/fs_FrameShader.glsl", "textures/texture_atlas.png",
)
LICENSES = (
    "SDL3-LICENSE.txt", "stb-LICENSE.txt", "GLM-LICENSE.txt", "yaml-cpp-LICENSE.txt",
    "glad-LICENSE.txt", "Apache-2.0-LICENSE.txt", "SOURCES.txt", "FastNoiseLite-license-source.h",
    "Khronos-license-source.h", "RobinHood-license-source.h", "SDL3-hidapi-LICENSE.txt", "SDL3-hidapi-BSD-LICENSE.txt",
    "SDL3-yuv2rgb-LICENSE.txt",
    "SDL3-e_log-license-source.txt", "SDL3-SDL_qsort-license-source.txt",
    "SDL3-SDL_strtokr-license-source.txt", "SDL3-SDL_rotate-license-source.txt",
    "SDL3-stb_image-license-source.txt", "SDL3-miniz-license-source.txt",
    "SDL3-SDL_opengl-license-source.txt", "SDL3-SDL_opengl_glext-license-source.txt",
    "SDL3-vk_platform-license-source.txt", "SDL3-vulkan_win32-license-source.txt",
)
RECORDED_BY = "SymoCraft POST_BUILD after the actual production link"


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def workspace_path(value: str) -> Path:
    path = Path(value)
    return Path(os.path.abspath(path if path.is_absolute() else ROOT / path))


def is_reparse(path: Path) -> bool:
    info = path.lstat()
    return path.is_symlink() or bool(getattr(info, "st_file_attributes", 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT)


def assert_child(path: Path, parent: Path) -> None:
    if path == parent or not path.is_relative_to(parent):
        raise ValueError(f"Use an isolated child of {parent}, not {path}.")
    for ancestor in (path, *path.parents):
        if ancestor.exists() and is_reparse(ancestor):
            raise ValueError(f"Reparse-point paths are not candidate build/package roots: {ancestor}")
        if ancestor == parent:
            break


def file_identity(path: Path, relative: str | None = None, *, allow_empty: bool = False) -> dict:
    if not path.is_file() or is_reparse(path):
        raise ValueError(f"An identity requires a regular file: {path}")
    size = path.stat().st_size
    if not size and not allow_empty:
        raise ValueError(f"A required payload is empty: {path}")
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return {"path": relative or path.as_posix(), "bytes": size, "sha256": digest.hexdigest()}


def assert_identity(path: Path, expected: dict, description: str) -> None:
    actual = file_identity(path, allow_empty=expected["bytes"] == 0)
    if actual["bytes"] != expected["bytes"] or actual["sha256"] != expected["sha256"]:
        raise ValueError(f"{description} changed after the successful production link: {path}. Rebuild before packaging.")


def production_inputs() -> list[dict]:
    files = {ROOT / "CMakeLists.txt", ROOT / "vendor/robin_hood.h", Path(__file__).resolve()}
    for name in SOURCE_TREES:
        directory = ROOT / name
        if not directory.is_dir() or is_reparse(directory):
            raise ValueError(f"Missing or redirected production tree: {directory}")
        for path in directory.rglob("*"):
            if is_reparse(path):
                raise ValueError(f"Redirected production input: {path}")
            if path.is_file():
                files.add(path)
    return [file_identity(path, path.relative_to(ROOT).as_posix(), allow_empty=True)
            for path in sorted(files, key=lambda item: item.as_posix().lower())]


def read_cache(path: Path) -> dict[str, str]:
    values = {}
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        if not line or line.startswith(("#", "//")):
            continue
        definition, separator, value = line.partition("=")
        key, type_separator, _ = definition.partition(":")
        if separator and type_separator:
            values[key] = value
    return values


def file_version(path: Path) -> str | None:
    version = ctypes.WinDLL("version", use_last_error=True)
    version.GetFileVersionInfoSizeW.argtypes = (ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_uint32))
    version.GetFileVersionInfoSizeW.restype = ctypes.c_uint32
    version.GetFileVersionInfoW.argtypes = (ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p)
    version.GetFileVersionInfoW.restype = ctypes.c_int
    version.VerQueryValueW.argtypes = (ctypes.c_void_p, ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_uint32))
    version.VerQueryValueW.restype = ctypes.c_int
    handle = ctypes.c_uint32()
    size = version.GetFileVersionInfoSizeW(str(path), ctypes.byref(handle))
    if not size:
        return None
    data = ctypes.create_string_buffer(size)
    if not version.GetFileVersionInfoW(str(path), 0, size, data):
        raise OSError(ctypes.get_last_error(), f"Cannot inspect file version: {path}")
    address, length = ctypes.c_void_p(), ctypes.c_uint32()
    if not version.VerQueryValueW(data, "\\", ctypes.byref(address), ctypes.byref(length)) or length.value < 16:
        raise OSError(ctypes.get_last_error(), f"Cannot inspect fixed file version: {path}")
    fixed = ctypes.cast(address, ctypes.POINTER(ctypes.c_uint32))
    if fixed[0] != 0xFEEF04BD:
        raise ValueError(f"Invalid file version structure: {path}")
    major_minor, build_revision = fixed[2], fixed[3]
    return f"{major_minor >> 16}.{major_minor & 65535}.{build_revision >> 16}.{build_revision & 65535}"


def tool_identity(name: str, path: Path) -> dict:
    return {"name": name, **file_identity(path), "file_version": file_version(path)}


def write_json(path: Path, value: dict, *, exclusive: bool = False) -> None:
    with path.open("x" if exclusive else "w", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2, ensure_ascii=True)
        stream.write("\n")


def run(arguments: list[str | Path], *, cwd: Path | None = None) -> subprocess.CompletedProcess:
    return subprocess.run([str(item) for item in arguments], cwd=cwd, capture_output=True,
                          text=True, encoding="utf-8", errors="replace", timeout=180, check=False)


def require_x64(path: Path) -> None:
    with path.open("rb") as stream:
        header = stream.read(64)
        if len(header) != 64 or header[:2] != b"MZ":
            raise ValueError(f"Not a Windows PE payload: {path}")
        stream.seek(struct.unpack_from("<I", header, 60)[0])
        pe = stream.read(6)
    if len(pe) != 6 or pe[:4] != b"PE\0\0" or struct.unpack_from("<H", pe, 4)[0] != 0x8664:
        raise ValueError(f"Candidate requires actual x64 executable/runtime bytes: {path}")


def validate_cache(cache: dict[str, str]) -> None:
    for key in ("CMAKE_HOME_DIRECTORY", "CMAKE_BUILD_TYPE", "CMAKE_GENERATOR", "CMAKE_COMMAND",
                "CMAKE_MAKE_PROGRAM", "CMAKE_CXX_COMPILER", "SYMOCRAFT_DXC", "MSVC_REDIST_DIR"):
        if not cache.get(key):
            raise ValueError(f"Missing production cache identity: {key}")
    if (workspace_path(cache["CMAKE_HOME_DIRECTORY"]) != ROOT or cache["CMAKE_GENERATOR"] != "Ninja"
            or cache["CMAKE_BUILD_TYPE"] not in ("Debug", "Release")
            or cache.get("SYMOCRAFT_RENDER_D3D12") != "ON" or cache.get("SYMOCRAFT_BUILD_GAME") != "ON"
            or cache.get("SYMOCRAFT_CPU_ONLY") != "OFF" or cache.get("SYMOCRAFT_BUILD_BENCHMARK") != "OFF"):
        raise ValueError("Candidates require this source tree, single-config Ninja, native game ON, benchmark OFF, and Debug or Release.")


def readiness_is_settled(code: int, text: str) -> bool:
    if code:
        return False
    steps = re.findall(r"^\[\d+/\d+\] (.+)$", text, re.M)
    permitted = {"Re-checking globbed directories...", "Synchronizing required SymoCraft runtime assets"}
    return all(step in permitted for step in steps) and bool(steps or "ninja: no work to do." in text)


def package(build: Path, output: Path, cache: dict, tools: list[dict], artifacts: list[dict], dumpbin: Path) -> dict:
    assert_child(output, ALLOWED_ROOT)
    if output == build or output.is_relative_to(build) or build.is_relative_to(output):
        raise ValueError("Candidate output and source build must be separate fresh directories.")
    if output.exists():
        raise FileExistsError("Use a fresh candidate output directory; existing outputs are never overwritten.")
    identity_path = build / "candidate-build-identities.json"
    identity = json.loads(identity_path.read_text(encoding="utf-8"))
    configuration = cache["CMAKE_BUILD_TYPE"]
    if (identity.get("schema_version") != 1 or identity.get("configuration") != configuration
            or Path(identity["build_directory"]) != build or Path(identity["source_directory"]) != ROOT
            or identity.get("recorded_by") != RECORDED_BY):
        raise ValueError("Build origin is not the matching successful native production link.")
    current_inputs = production_inputs()
    if current_inputs != identity["sources"]:
        raise ValueError("Production input bytes or file set changed after linking; rebuild before packaging.")
    if tools != identity["tools"] or artifacts != identity["build_artifacts"]:
        raise ValueError("Recorded tools or actual build artifact bytes changed; rebuild before packaging.")
    # Ninja dry-run cannot resolve CMake's always-dirty VerifyGlobs step.
    # Execute the readiness check, but never accept an implicit product rebuild.
    dry = run([cache["CMAKE_MAKE_PROGRAM"], "-C", build, "SymoCraft"])
    dry_text = dry.stdout + dry.stderr
    if not readiness_is_settled(dry.returncode, dry_text):
        raise ValueError(f"Production target is stale or reconfiguration is pending; rebuild before packaging.\n{dry_text}")
    for artifact in identity["build_artifacts"]:
        assert_identity(build / artifact["path"], artifact, "Readiness-checked build artifact")
    provenance = output / "provenance"
    provenance.mkdir(parents=True, exist_ok=False)
    installed = run([cache["CMAKE_COMMAND"], "--install", build, "--config", configuration, "--prefix", output])
    (provenance / "install.log").write_text(installed.stdout + installed.stderr, encoding="utf-8")
    if installed.returncode:
        raise RuntimeError(f"Production install failed ({installed.returncode}); preserve the partial candidate for diagnosis.")
    shutil.copy2(identity_path, provenance / identity_path.name)
    (provenance / "ninja-freshness.log").write_text(dry_text, encoding="utf-8")
    executable = output / "SymoCraft.exe"
    assert_identity(executable, artifacts[0], "Installed game executable")
    require_x64(executable)
    for shader in artifacts:
        if shader["path"].endswith(".dxil"):
            assert_identity(output / shader["path"][4:], shader, "Installed shader")
    opengl = cache.get("SYMOCRAFT_RENDER_OPENGL", "ON") == "ON"
    for asset in ASSETS:
        if not opengl and asset.endswith(".glsl"):
            continue
        assert_identity(output / "assets" / asset, file_identity(ROOT / "assets" / asset), "Installed game asset")
    for license_name in LICENSES:
        if not opengl and license_name in {"glad-LICENSE.txt", "Khronos-license-source.h"}:
            continue
        file_identity(output / "licenses" / license_name)
    if not opengl and (any((output / "assets").rglob("*.glsl"))
                       or (output / "licenses/glad-LICENSE.txt").exists()):
        raise ValueError("Disabled legacy OpenGL assets entered the modern-only candidate.")

    runtime_names = ("msvcp140d.dll", "vcruntime140d.dll", "vcruntime140_1d.dll", "ucrtbased.dll") if configuration == "Debug" else (
        "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll")
    for name in runtime_names:
        file_identity(output / name)
    runtime_family = cache.get("SYMOCRAFT_MSVC_REDIST_NAME", "VC143")
    if not re.fullmatch(r"VC14[0-9]", runtime_family):
        raise ValueError(f"Invalid selected MSVC runtime family: {runtime_family}")
    runtime_directory = Path(cache["MSVC_REDIST_DIR"]) / (
        f"Debug_NonRedist/x64/Microsoft.{runtime_family}.DebugCRT" if configuration == "Debug" else f"x64/Microsoft.{runtime_family}.CRT")
    runtime_origins = []
    dlls = sorted(output.glob("*.dll"))
    for dll in dlls:
        is_debug = bool(re.fullmatch(r"(msvcp.*d(?:_[^.]+)?|vcruntime.*d|ucrtbased|concrt.*d)\.dll", dll.name, re.I))
        if (configuration == "Debug") != is_debug:
            raise ValueError(f"Wrong-configuration runtime payload: {dll.name}")
        origin = runtime_directory / dll.name
        if dll.name.lower() == "ucrtbased.dll":
            origin = Path(cache["SYMOCRAFT_DXC"]).parent / "ucrt/ucrtbased.dll"
        require_x64(dll)
        assert_identity(dll, file_identity(origin), "Installed matching-toolset runtime")
        runtime_origins.append({"packaged": dll.name, "origin": file_identity(origin), "file_version": file_version(dll)})

    imports = []
    for binary in (executable, *dlls):
        result = run([dumpbin, "/DEPENDENTS", binary])
        text = result.stdout + result.stderr
        (provenance / f"imports-{binary.name}.log").write_text(text, encoding="utf-8")
        if result.returncode:
            raise RuntimeError(f"Actual dependency inspection failed: {binary.name}")
        for dependency in re.findall(r"^\s+([A-Za-z0-9_.-]+\.dll)\s*$", text, re.I | re.M):
            if re.fullmatch(r"(dxcompiler|dxil|d3dcompiler[^.]*|SDL3)\.dll", dependency, re.I):
                raise ValueError(f"Unexpected SDK/compiler/dynamic SDL dependency: {dependency}")
            local = output / dependency
            if local.is_file():
                resolution, resolved = "app-local", local
            elif dependency.lower().startswith(("api-ms-win-", "ext-ms-")):
                resolution, resolved = "Windows-API-set-contract", None
            else:
                resolved = Path(os.environ["SystemRoot"]) / "System32" / dependency
                if not resolved.is_file():
                    raise ValueError(f"Unresolved actual dependency: {binary.name} -> {dependency}")
                if dependency.lower().startswith(("msvcp", "vcruntime", "concrt", "ucrtbased")):
                    raise ValueError(f"CRT dependency is not app-local: {binary.name} -> {dependency}")
                resolution = "Windows-System32-on-builder"
            imports.append({"binary": binary.name, "dependency": dependency, "resolution": resolution,
                            "resolved_path": resolved.as_posix() if resolved else None})
    actual_game_imports = {row["dependency"].lower() for row in imports if row["binary"] == executable.name}
    if not {"d3d12.dll", "dxgi.dll"}.issubset(actual_game_imports):
        raise ValueError("This executable does not actually import the production D3D12/DXGI backend.")
    write_json(provenance / "dependencies.json", {
        "schema_version": 1, "configuration": configuration, "runtime_origins": runtime_origins, "imports": imports,
        "limitations": "Static executable and app-local DLL imports inspected on the builder. API sets, dynamically loaded GPU drivers, and the Windows Graphics Tools Debug Layer require the destination Windows installation; not a clean-machine runtime test.",
    }, exclusive=True)
    runtime_policy = "developer-local-nonredistributable" if configuration == "Debug" else "app-local-redistributable"
    readme = f"""SymoCraft production transition game candidate ({configuration}, Windows x64)

The game defaults to D3D12. Explicit --renderer d3d12 selects the same path.
Legacy OpenGL is {'compiled in; select it with --renderer opengl' if opengl else 'not compiled in; explicit OpenGL is rejected without fallback'}.
Enabled paths use one game loop and full world assets. Preserve every package
byte and package-manifest.json. Store run evidence outside this immutable package.

From the package directory:
  .\\SymoCraft.exe --check-assets
  .\\SymoCraft.exe --check-assets --renderer d3d12
  .\\SymoCraft.exe
  .\\SymoCraft.exe --renderer d3d12 --msaa 4 --validation 1

D3D12 requires a real capable hardware adapter and Windows Graphics Tools for the
Debug Layer. No WARP, backend fallback, or silent validation disablement is used.
Compiled DXIL and app-local matching MSVC CRT are included; no DXC, Python,
compiler, development SDK, SDL DLL, or source checkout is needed at runtime.
Normal Windows graphics libraries and installed hardware drivers remain required.
Debug Layer binaries are not redistributed here.

Runtime policy: {runtime_policy}.
Debug CRT files come from Debug_NonRedist and the selected SDK's debug UCRT. Debug
candidates are for this developer's local diagnosis only, not external
redistribution. Use the Release candidate for portable review.

provenance/candidate-build-identities.json binds the actual successful production
link, executable, four shaders, selected production source trees, and build tools.
Builder paths are provenance only, not runtime lookup paths. dependencies.json
and import logs record actual static imports, not a clean-machine runtime test or
a complete dynamic driver dependency inventory. Package hashes are relative.

Packaging does not itself approve a stage or the 14-operation / 15-minute manual
matrix. Prior approvals and verification apply only to their recorded identity.
"""
    (output / "README.txt").write_text(readme, encoding="utf-8")
    for path in output.rglob("*"):
        if path.is_file() and (path.suffix.lower() in (".lib", ".obj", ".pdb")
                               or re.fullmatch(r"(dxc|dxcompiler|dxil)\.(exe|dll)", path.name, re.I)):
            raise ValueError(f"Development build payload must not enter the candidate: {path}")
    if production_inputs() != identity["sources"]:
        raise ValueError("Production sources changed while installing or inspecting the candidate.")
    for artifact in identity["build_artifacts"]:
        assert_identity(build / artifact["path"], artifact, "Build artifact")
    files = [file_identity(path, path.relative_to(output).as_posix()) for path in
             sorted(output.rglob("*"), key=lambda item: item.as_posix().lower()) if path.is_file()]
    manifest = {
        "schema_version": 1, "package_kind": "production-game-candidate", "configuration": configuration,
        "created_utc": utc_now(), "default_renderer": "d3d12",
        "available_renderers": ["opengl", "d3d12"] if opengl else ["d3d12"],
        "required_debug_layer": True, "validation_default": True, "software_adapter_allowed": False,
        "runtime_policy": runtime_policy,
        "origin_build": {"directory": build.as_posix(), "source_directory": ROOT.as_posix(),
                         "build_identity_sha256": file_identity(identity_path)["sha256"], "executable_sha256": artifacts[0]["sha256"]},
        "files": files,
        "scope": "Actual production game and installed assets. Packaging implies no stage acceptance, manual result, clean-machine portability claim, or injected-failure-as-hardware-failure claim.",
    }
    manifest_path = output / "package-manifest.json"
    write_json(manifest_path, manifest, exclusive=True)
    return {"package_directory": output.as_posix(), "configuration": configuration, "payload_files": len(files),
            "manifest_sha256": file_identity(manifest_path)["sha256"]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", required=True)
    parser.add_argument("--output-directory")
    parser.add_argument("--record-build-identity", action="store_true")
    args = parser.parse_args()
    if os.name != "nt":
        parser.error("Production candidates require the actual Windows x64 MSVC builder.")
    build = workspace_path(args.build_directory)
    assert_child(build, ALLOWED_ROOT)
    cache = read_cache(build / "CMakeCache.txt")
    validate_cache(cache)
    dumpbin = Path(cache["CMAKE_CXX_COMPILER"]).parent / "dumpbin.exe"
    tools = [tool_identity(name, Path(path)) for name, path in (
        ("CMake", cache["CMAKE_COMMAND"]), ("Ninja", cache["CMAKE_MAKE_PROGRAM"]),
        ("MSVC", cache["CMAKE_CXX_COMPILER"]), ("Dumpbin", dumpbin),
        ("DXC-build-only", cache["SYMOCRAFT_DXC"]), ("Python-build-only", sys.executable))]
    artifacts = [file_identity(build / relative, relative) for relative in ARTIFACT_PATHS]
    if args.record_build_identity:
        if args.output_directory:
            parser.error("--record-build-identity does not install or accept an output directory.")
        identity = {
            "schema_version": 1, "recorded_utc": utc_now(), "source_directory": ROOT.as_posix(),
            "build_directory": build.as_posix(), "configuration": cache["CMAKE_BUILD_TYPE"], "generator": "Ninja",
            "executable": "bin/SymoCraft.exe", "shader_profile": "Shader Model 6.0", "recorded_by": RECORDED_BY,
            "source_identity_scope": "All files under game, cmake, assets and selected vendored SDL3/GLM/stb/yaml-cpp/GLAD/KHR/FastNoiseLite trees, vendor/robin_hood.h, root CMakeLists, and this packager. Not a complete SDK, driver, or operating-system snapshot.",
            "sources": production_inputs(), "tools": tools, "build_artifacts": artifacts,
        }
        path = build / "candidate-build-identities.json"
        write_json(path, identity)
        print(f"Recorded production candidate identities: {path}")
        return 0
    if not args.output_directory:
        parser.error("Packaging requires --output-directory.")
    print(json.dumps(package(build, workspace_path(args.output_directory), cache, tools, artifacts, dumpbin), indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"Candidate packaging failed: {error}", file=sys.stderr)
        raise SystemExit(1)
