"""Shared CLI and process helpers for the developer scripts (Python 3.10+)."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time

if sys.version_info < (3, 10):
    raise SystemExit("Python 3.10+ is required for the developer scripts.")

ROOT = Path(__file__).resolve().parent.parent


def option(parser, name, **kwargs):
    """Accept Python-style flags and the old PowerShell parameter spelling."""
    import re

    flag = re.sub(r"(?<!^)(?=[A-Z][a-z])|(?<=[a-z])(?=[A-Z])", "-", name).lower()
    flag = {"CLionPath": "clion-path", "CMakePath": "cmake-path", "VSync": "vsync"}.get(name, flag)
    return parser.add_argument("--" + flag, "-" + name, dest=name, **kwargs)


def integer(low, high):
    def parse(value):
        try:
            number = int(value)
        except ValueError as error:
            raise argparse.ArgumentTypeError("Expected an integer.") from error
        if not low <= number <= high:
            raise argparse.ArgumentTypeError(f"Expected a value in [{low}, {high}].")
        return number

    return parse


def file_path(value):
    path = Path(value).resolve(strict=True)
    if not path.is_file():
        raise ValueError(f"Expected a file: {path}")
    return path


def new_directory(value):
    path = Path(value).resolve()
    if path.exists():
        raise ValueError(f"Choose a new output directory; previous runs are not overwritten: {path}")
    path.mkdir(parents=True)
    return path


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_json(path, value):
    Path(path).write_text(json.dumps(value, ensure_ascii=False, indent=2, allow_nan=False) + "\n",
                          encoding="utf-8")


def read_log(path):
    return Path(path).read_text(encoding="utf-8-sig", errors="replace")


def hidden_process():
    if os.name != "nt":
        return {}
    info = subprocess.STARTUPINFO()
    info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    info.wShowWindow = subprocess.SW_HIDE
    return {"startupinfo": info, "creationflags": subprocess.CREATE_NO_WINDOW}


def stop_process(process):
    if process is not None:
        if process.poll() is None:
            process.kill()
        process.wait()


def run_logged(executable, arguments, directory, timeout):
    directory = Path(directory)
    started = time.monotonic()
    with (directory / "stdout.log").open("wb") as stdout, (directory / "stderr.log").open("wb") as stderr:
        process = subprocess.Popen([str(executable), *arguments], cwd=directory,
                                   stdout=stdout, stderr=stderr, **hidden_process())
        timed_out = False
        try:
            try:
                process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                timed_out = True
                stop_process(process)
        finally:
            stop_process(process)
    return {"processId": process.pid, "exitCode": process.returncode,
            "timedOut": timed_out, "elapsedSeconds": time.monotonic() - started}


def cli(main):
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"Error: {error}", file=sys.stderr)
        raise SystemExit(1) from error
    except KeyboardInterrupt:
        print("Cancelled; owned child processes stopped, evidence retained.", file=sys.stderr)
        raise SystemExit(130)
