"""Shared evidence, subprocess and toolchain helpers for native verification."""

import argparse
from contextlib import contextmanager
from datetime import datetime, timezone
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

from scripts_common import ROOT, cli, file_path, hidden_process, new_directory, option, read_log, sha256, stop_process, write_json


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def load_json(path):
    def invalid_constant(value):
        raise ValueError(f"Nonfinite JSON number is invalid: {value}")
    return json.loads(Path(path).read_text(encoding="utf-8-sig"), parse_constant=invalid_constant)


def workspace_path(value, parent=None):
    reject_links(ROOT / value)
    path = (ROOT / value).resolve()
    if parent is not None:
        parent = Path(parent).resolve()
        require(path != parent and path.is_relative_to(parent), f"Path must be below {parent}: {path}")
    return path


def archive_destination(value):
    """Require a fresh, unlinked destination inside the sole archive root."""
    path = workspace_path(value, ROOT / "docs/verification/archive")
    require(not path.exists(), f"Choose a new archive directory: {path}")
    return path


def copy_evidence(source, target):
    """Copy exact bytes exclusively; retain source and partial output on failure."""
    source, target = Path(source), Path(target)
    reject_links(source)
    reject_links(target)
    source = file_path(source)
    before = source.stat()
    expected = sha256(source)
    require((before.st_size, before.st_mtime_ns) == (source.stat().st_size, source.stat().st_mtime_ns),
            f"Input changed during identity capture: {source}")
    target.parent.mkdir(parents=True, exist_ok=True)
    reject_links(target)
    with source.open("rb") as incoming, target.open("xb") as outgoing:
        shutil.copyfileobj(incoming, outgoing)
    after = source.stat()
    require(target.stat().st_size == before.st_size and sha256(target) == expected
            and (before.st_size, before.st_mtime_ns) == (after.st_size, after.st_mtime_ns)
            and sha256(source) == expected, f"Input changed during archive: {source}")
    shutil.copystat(source, target)
    return {"bytes": before.st_size, "sha256": expected}


def reject_links(path):
    path = Path(os.path.abspath(path))
    for item in (path, *path.parents):
        if item.exists():
            require(not item.is_symlink() and not getattr(item.lstat(), "st_file_attributes", 0) & 0x400,
                    f"Linked/reparse paths are not supported: {item}")


def files(directory):
    directory = Path(directory)
    reject_links(directory)
    pending, result = [directory], []
    while pending:
        for item in pending.pop().iterdir():
            reject_links(item)
            if item.is_dir():
                pending.append(item)
            elif item.is_file():
                result.append(item)
    return sorted(result)


def identity(path, root=None):
    path = file_path(path)
    return {"path": path.relative_to(root).as_posix() if root else str(path),
            "bytes": path.stat().st_size, "sha256": sha256(path)}


def assert_hash(path, expected, description="Input"):
    require(sha256(path).lower() == expected.lower(), f"{description} changed: {path}")


def snapshot_identity(paths, root=ROOT):
    result = {}
    for value in paths:
        path = root / value
        for item in files(path) if path.is_dir() else [file_path(path)]:
            record = identity(item, root)
            result[record["path"]] = record
    return [result[key] for key in sorted(result)]


def capture(program, arguments, cwd, stdout, stderr=None, timeout=120, env=None, interactive=False):
    """Keep raw bytes, including on timeout; terminate only the owned process."""
    started = time.monotonic()
    with Path(stdout).open("wb") as output:
        errors = Path(stderr).open("wb") if stderr else None
        process = None
        timed_out = False
        try:
            process = subprocess.Popen([str(program), *map(str, arguments)], cwd=cwd, env=env,
                                       stdout=output, stderr=errors or subprocess.STDOUT,
                                       **({} if interactive else hidden_process()))
            try:
                process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                timed_out = True
                stop_process(process)
        finally:
            stop_process(process)
            if errors:
                errors.close()
    return {"exit_code": process.returncode, "pid": process.pid, "timed_out": timed_out,
            "elapsed_seconds": time.monotonic() - started}


def logged(program, arguments, log, cwd=ROOT, env=None, timeout=600):
    record = capture(program, arguments, cwd, log, timeout=timeout, env=env)
    require(not record["timed_out"] and record["exit_code"] == 0,
            f"Command failed (exit {record['exit_code']}, timeout={record['timed_out']}): see {log}")
    return read_log(log)


def clean_environment():
    env = dict(os.environ)
    for name in ("SDL_VIDEO_DRIVER", "SDL_OPENGL_LIBRARY", "SDL_VULKAN_LIBRARY", "SDL3_DYNAMIC_API", "SDL_WINDOWS_RAW_KEYBOARD"):
        env.pop(name, None)
    return env


def load_script(path):
    path = Path(path)
    spec = importlib.util.spec_from_file_location(path.stem.replace("-", "_"), path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def file_version(path):
    if os.name != "nt":
        return None
    import ctypes
    from ctypes import wintypes
    api = ctypes.WinDLL("version", use_last_error=True)
    api.GetFileVersionInfoSizeW.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(wintypes.DWORD)]
    api.GetFileVersionInfoSizeW.restype = wintypes.DWORD
    size = api.GetFileVersionInfoSizeW(str(path), None)
    if not size:
        return None
    data = ctypes.create_string_buffer(size)
    api.GetFileVersionInfoW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p]
    api.VerQueryValueW.argtypes = [ctypes.c_void_p, wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(wintypes.UINT)]
    require(api.GetFileVersionInfoW(str(path), 0, size, data), "Cannot read file version.")
    pointer, length = ctypes.c_void_p(), wintypes.UINT()
    require(api.VerQueryValueW(data, "\\", ctypes.byref(pointer), ctypes.byref(length)), "Missing fixed file version.")
    words = ctypes.cast(pointer, ctypes.POINTER(wintypes.DWORD))
    return ".".join(map(str, (words[2] >> 16, words[2] & 65535, words[3] >> 16, words[3] & 65535)))


def safe_child(root, relative):
    root = Path(root).resolve()
    require("\\" not in str(relative) and ":" not in str(relative), "Manifest paths must use portable relative names.")
    relative = Path(relative)
    require(not relative.is_absolute() and not relative.drive, "Manifest paths must be relative.")
    reject_links(root / relative)
    path = (root / relative).resolve()
    require(path != root and path.is_relative_to(root), "Manifest path escapes its directory.")
    return path


def verify_payload(root, entries):
    require(bool(entries), "Empty package manifest.")
    seen = set()
    for entry in entries:
        path = safe_child(root, entry["path"])
        require(path not in seen, "Duplicate package manifest path.")
        seen.add(path)
        if "bytes" in entry:
            require(path.stat().st_size == entry["bytes"], f"Payload size changed: {path}")
        assert_hash(path, entry["sha256"], "Packaged payload")


def platform_imports(executable, source_directory=None, probe_sources=()):
    imports = load_json(Path(executable).parent / "production-imports.json")
    assert_hash(Path(imports["production_build"]) / "CMakeCache.txt", imports["production_cache_sha256"])
    for name, key in (("window.cpp", "current_window_source_sha256"), ("input_state.h", "current_input_state_source_sha256")):
        assert_hash(ROOT / "game/modules/platform/src" / name, imports[key])
    for entry in imports["imports"]:
        assert_hash(entry["archive"], entry["sha256"])
    for name in probe_sources:
        require((Path(source_directory) / name).stat().st_mtime <= Path(executable).stat().st_mtime,
                "Probe source is newer than the executable; rebuild first.")
    return imports


def json_probe(executable, arguments, output, name, timeout=30, env=None, cwd=None):
    stdout, stderr = output / (name + ".stdout.log"), output / (name + ".stderr.log")
    record = {"exit_code": None, "timed_out": False, "report": None, "harness_error": None,
              "stdout": str(stdout), "stderr": str(stderr)}
    try:
        record.update(capture(executable, arguments, cwd or output, stdout, stderr, timeout, env))
        record["report"] = load_json(stdout)
    except (ValueError, OSError) as error:
        record["harness_error"] = str(error)
    return record, read_log(stderr) if stderr.exists() else ""


def evidence_sources(directories, extra=()):
    paths = list(map(Path, extra))
    for directory in directories:
        paths.extend(p for p in files(directory) if p.suffix in {".h", ".hpp", ".cpp", ".hlsl", ".hlsli", ".cmake", ".py"}
                     or p.name == "CMakeLists.txt")
    paths.extend(Path(__file__).parent / name for name in ("scripts_common.py", "verification_common.py"))
    return [identity(p) for p in sorted(set(paths))]


def toolchain(args):
    from build import developer_environment, resolve_tool
    require(os.name == "nt", "This native toolchain verification requires Windows x64/MSVC.")
    clion = getattr(args, "CLionPath", None)
    cmake = resolve_tool(getattr(args, "CMakePath", None), "cmake.exe", clion, "bin/cmake/win/x64/bin/cmake.exe")
    ninja = resolve_tool(getattr(args, "NinjaPath", None), "ninja.exe", clion, "bin/ninja/win/x64/ninja.exe")
    studio = getattr(args, "VisualStudioPath", None)
    if not studio:
        vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
        studio = subprocess.run([str(vswhere), "-latest", "-products", "*", "-version", "[17.0,18.0)",
                                 "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath", "-utf8"],
                                capture_output=True, encoding="utf-8", check=True, timeout=30, **hidden_process()).stdout.strip()
    require(studio, "MSVC 2022 installation not found.")
    env = developer_environment(studio, getattr(args, "ToolsetVersion", None))
    env["PATH"] = os.pathsep.join((str(Path(sys.executable).parent), str(Path(cmake).parent), str(Path(ninja).parent), env.get("PATH", "")))
    env["VSLANG"] = "1033"
    compiler = shutil.which("cl.exe", path=env["PATH"])
    require(compiler, "MSVC developer environment does not provide cl.exe.")
    return {"cmake": cmake, "ninja": ninja, "ctest": str(Path(cmake).with_name("ctest.exe")),
            "compiler": compiler, "studio": str(studio), "env": env}


def tool_options(parser):
    for name in ("CLionPath", "CMakePath", "NinjaPath", "VisualStudioPath"):
        option(parser, name)
    option(parser, "ToolsetVersion", default="14.38")


def cache(directory):
    result = {}
    for line in read_log(Path(directory) / "CMakeCache.txt").splitlines():
        if line and not line.startswith(("#", "//")) and ":" in line and "=" in line:
            key, value = line.split("=", 1)
            result[key.split(":", 1)[0]] = value
    return result


def cmake_options(tools, configuration, testing=True):
    return ["-G", "Ninja", f"-DCMAKE_BUILD_TYPE={configuration}", f"-DCMAKE_TRY_COMPILE_CONFIGURATION={configuration}",
            f"-DCMAKE_MAKE_PROGRAM={Path(tools['ninja']).as_posix()}", f"-DCMAKE_C_COMPILER={Path(tools['compiler']).as_posix()}",
            f"-DCMAKE_CXX_COMPILER={Path(tools['compiler']).as_posix()}", f"-DBUILD_TESTING={'ON' if testing else 'OFF'}",
            f"-DPython3_EXECUTABLE={Path(sys.executable).as_posix()}"]


def touch_probe(path):
    reject_links(path)
    stat = path.stat()
    require(stat.st_mtime <= time.time() + 1, "Probe header has a future timestamp; settle the checkout first.")
    while time.time() < stat.st_mtime + 2:
        time.sleep(0.25)
    os.utime(path, None)
    require(abs(path.stat().st_mtime - stat.st_mtime) >= 1, "Probe timestamp did not change enough.")
    return stat


@contextmanager
def probe_header(path):
    path = Path(path)
    original, digest = path.stat(), sha256(path)
    record = {"sha256": digest.upper(), "original_mtime_utc": datetime.fromtimestamp(original.st_mtime, timezone.utc).isoformat(),
              "probe_mtime_utc": None, "restored_mtime": False}
    try:
        touch_probe(path)
        record["probe_mtime_utc"] = datetime.fromtimestamp(path.stat().st_mtime, timezone.utc).isoformat()
        yield record
    finally:
        require(sha256(path) == digest, "A probe header changed concurrently; its timestamp was not restored. Rebuild and rerun.")
        os.utime(path, ns=(original.st_atime_ns, original.st_mtime_ns))
        record["restored_mtime"] = True
