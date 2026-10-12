#!/usr/bin/env python3
"""Verify relocated production candidates without changing their source package."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
import uuid


ROOT = Path(__file__).resolve().parents[3]
ALLOWED = ROOT / "out" / "m3-t3"
BINARY_SUFFIXES = {".exe", ".dll", ".lib", ".pdb", ".obj"}
REQUIRED_FILES = {
    "SymoCraft.exe", "assets/configs/blockFormats.yaml", "assets/textures/texture_atlas.png",
    "assets/shaders/d3d12/blocks.vs.dxil", "assets/shaders/d3d12/blocks.ps.dxil",
    "assets/shaders/d3d12/selection.vs.dxil", "assets/shaders/d3d12/selection.ps.dxil",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def checked_path(path: Path, *, scoped: bool = False) -> Path:
    absolute = Path(os.path.abspath(path))
    for ancestor in (absolute, *absolute.parents):
        try:
            attributes = getattr(ancestor.lstat(), "st_file_attributes", 0)
        except FileNotFoundError:
            continue
        require(not ancestor.is_symlink() and not attributes & 0x400,
                f"Reparse points, junctions and symbolic links are not allowed: {ancestor}")
    if scoped:
        require(absolute.is_relative_to(ALLOWED), "Verification uses only this repository out/m3-t3 directories.")
    return absolute


def identity(path: Path) -> dict:
    path = checked_path(path)
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return {"path": str(path), "bytes": path.stat().st_size, "sha256": digest.hexdigest()}


def package_identity(directory: Path) -> list[dict]:
    directory = checked_path(directory, scoped=True)
    result = []
    pending = [directory]
    while pending:
        for path in sorted(pending.pop().iterdir()):
            path = checked_path(path, scoped=True)
            if path.is_dir():
                pending.append(path)
            elif path.is_file():
                item = identity(path)
                item["path"] = path.relative_to(directory).as_posix()
                result.append(item)
            else:
                raise RuntimeError(f"Package contains an unsupported filesystem entry: {path}")
    return sorted(result, key=lambda item: item["path"])


def read_package_manifest(directory: Path) -> dict:
    directory = checked_path(directory, scoped=True)
    description = json.loads((directory / "package-manifest.json").read_text(encoding="utf-8-sig"))
    require(description["schema_version"] == 1, "Unsupported candidate manifest version.")
    require(description["package_kind"] == "production-game-candidate", "Input is not a production game candidate.")
    configuration = description["configuration"]
    require(configuration in {"Debug", "Release"}, "Candidate configuration is not Debug/Release.")
    require(description["default_renderer"] in {"opengl", "d3d12"}, "Unsupported candidate game default.")
    renderers = sorted(description["available_renderers"])
    require(renderers in (["d3d12"], ["d3d12", "opengl"]), "Unexpected candidate renderer list.")
    require(description["required_debug_layer"] is True and description["validation_default"] is True
            and description["software_adapter_allowed"] is False, "Unexpected capability/validation policy.")
    expected_policy = "developer-local-nonredistributable" if configuration == "Debug" else "app-local-redistributable"
    require(description["runtime_policy"] == expected_policy, "Incorrect candidate runtime redistribution policy.")
    actual = {item["path"].casefold(): item for item in package_identity(directory)
              if item["path"] != "package-manifest.json"}
    declared = {}
    for entry in description["files"]:
        relative = entry["path"]
        require(isinstance(relative, str) and relative and "\\" not in relative
                and relative != "package-manifest.json", "Manifest file path is not normalized and relative.")
        full = checked_path(directory / relative, scoped=True)
        require(full.is_relative_to(directory) and full.relative_to(directory).as_posix() == relative,
                f"Manifest path is not canonical or escapes the package: {relative}")
        require(relative.casefold() not in declared, "Manifest repeats a file path.")
        declared[relative.casefold()] = entry
    require(actual.keys() == declared.keys(), "Candidate file set differs from its manifest.")
    for key, entry in actual.items():
        require(entry == declared[key], f"Package hash/size/path mismatch: {entry['path']}")
    require({item.casefold() for item in REQUIRED_FILES} <= declared.keys(), "Required package file is undeclared.")
    if renderers == ["d3d12"]:
        require(not any(name.endswith(".glsl") or name == "licenses/glad-license.txt" for name in declared),
                "Modern-only candidate contains legacy runtime payload.")
    return description


def runtime_environment() -> dict[str, str]:
    environment = os.environ.copy()
    compiler_variable = re.compile(
        r"^(INCLUDE|LIB|LIBPATH|VCINSTALLDIR|VCTools.*|VSINSTALLDIR|VSCMD_.*|VisualStudioVersion|"
        r"WindowsSDK.*|UniversalCRTSdkDir|UCRTVersion|DXC.*|CMAKE_.*)$", re.IGNORECASE)
    for name in list(environment):
        if compiler_variable.fullmatch(name):
            del environment[name]
    windows = Path(environment["SYSTEMROOT"])
    environment["PATH"] = os.pathsep.join(map(str, (windows / "System32", windows, windows / "System32" / "Wbem")))
    return environment


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package-directory", required=True, type=Path)
    parser.add_argument("--report-probe", required=True, type=Path)
    parser.add_argument("--output-directory", required=True, type=Path)
    parser.add_argument("--include-opengl-comparison", action="store_true")
    parser.add_argument("--process-timeout-seconds", type=int, default=180)
    arguments = parser.parse_args()
    require(30 <= arguments.process_timeout_seconds <= 300, "Process timeout must be 30..300 seconds.")
    for name in ("package_directory", "report_probe", "output_directory"):
        path = checked_path(getattr(arguments, name), scoped=True)
        setattr(arguments, name, path)
    require(arguments.package_directory.is_dir(), "PackageDirectory is missing.")
    require(arguments.report_probe.is_file(), "ReportProbe is missing.")
    require(not arguments.output_directory.exists(), "OutputDirectory must be new; old runs are never overwritten.")
    require(not arguments.output_directory.is_relative_to(arguments.package_directory), "Output must not be inside the original package.")
    require(os.name == "nt", "Native candidate verification requires Windows.")
    return arguments


class Verification:
    def __init__(self, arguments: argparse.Namespace) -> None:
        self.arguments = arguments
        self.package = arguments.package_directory
        self.probe = arguments.report_probe
        self.output = arguments.output_directory
        self.workdir = self.output / "unrelated working directory"
        self.output.mkdir()
        self.workdir.mkdir()
        self.current_case = None
        self.manifest = {
            "schema": "symocraft.t3.r2-step4-candidate-verification", "version": 1, "run_id": str(uuid.uuid4()),
            "started_utc": utc_now(), "finished_utc": None, "success": False,
            "package_directory": str(self.package), "package_manifest": None, "package_identity": [], "package_identity_after": [],
            "report_probe_identity": identity(self.probe), "runner_identity": identity(Path(__file__).resolve()),
            "process_timeout_seconds": arguments.process_timeout_seconds, "working_directory": str(self.workdir), "relocated_package": None,
            "child_environment_policy": "system-only PATH; compiler/SDK/include/library variables removed; original host still has developer tools installed",
            "fixed_complete_world_short_workloads": False, "installed_candidate_directory_exercised": False,
            "release_runtime_on_sdk_absent_machine_exercised": False, "debug_runtime_redistributable": False,
            "isolated_asset_faults": True, "actual_device_lost_exercised": False, "actual_in_flight_timeout_exercised": False,
            "injected_device_lost_exercised": False, "injected_submission_failure_exercised": False, "injected_wait_timeout_exercised": False,
            "manual_gameplay_acceptance": False, "fifteen_minute_session": False, "r3_acceptance": False,
            "opengl_default_changed": False, "cases": [], "artifacts": [],
        }

    def save(self) -> None:
        (self.output / "verification.json").write_text(json.dumps(self.manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    def check(self, condition: bool, message: str) -> None:
        if not condition:
            self.current_case["success"] = False
            self.current_case["issues"].append(message)

    def copy_package(self, destination: Path) -> None:
        destination = checked_path(destination, scoped=True)
        shutil.copytree(self.package, destination)
        read_package_manifest(destination)

    def run_process(self, name: str, executable: Path, arguments: list[str], expected_exit: int) -> dict:
        case = {
            "name": name, "executable": str(executable), "arguments": arguments, "working_directory": str(self.workdir),
            "process_id": None, "elapsed_seconds": None, "exit_code": None, "timed_out": False, "stopped": False,
            "expected_exit": expected_exit, "success": False, "stdout": str(self.output / f"{name}.stdout.log"),
            "stderr": str(self.output / f"{name}.stderr.log"), "observation": None, "issues": [],
        }
        self.current_case = case
        self.manifest["cases"].append(case)
        self.save()
        start = time.monotonic()
        process = None
        output = b""
        errors = b""
        try:
            startup = subprocess.STARTUPINFO()
            startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            startup.wShowWindow = subprocess.SW_HIDE
            process = subprocess.Popen([str(executable), *arguments], cwd=self.workdir, env=runtime_environment(),
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, startupinfo=startup,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
            case["process_id"] = process.pid
            try:
                output, errors = process.communicate(timeout=self.arguments.process_timeout_seconds)
            except subprocess.TimeoutExpired as timeout:
                case["timed_out"] = True
                output, errors = timeout.output or b"", timeout.stderr or b""
                process.kill()
                output, errors = process.communicate(timeout=10)
            case["stopped"] = process.poll() is not None
            case["exit_code"] = process.returncode
            case["success"] = not case["timed_out"] and case["stopped"] and process.returncode == expected_exit
            if case["timed_out"]:
                case["issues"].append("Process exceeded the bounded runtime and was forcibly stopped.")
            if process.returncode != expected_exit:
                case["issues"].append(f"Expected exit {expected_exit}, observed {process.returncode}.")
        except Exception as error:
            case["issues"].append(str(error))
            raise
        finally:
            try:
                if process is not None and process.poll() is None:
                    process.kill()
                    output, errors = process.communicate(timeout=10)
                    case["stopped"] = process.poll() is not None
            finally:
                Path(case["stdout"]).write_bytes(output)
                Path(case["stderr"]).write_bytes(errors)
                case["elapsed_seconds"] = time.monotonic() - start
                self.save()
        return case

    def logs(self) -> tuple[str, str]:
        return tuple(Path(self.current_case[key]).read_text(encoding="utf-8-sig", errors="replace") for key in ("stdout", "stderr"))

    def verify_frames(self, directory: Path, backend: str, workload: str) -> None:
        name = self.current_case["name"]
        with (self.output / f"{name}.probe.json").open("wb") as output, (self.output / f"{name}.probe.stderr.log").open("wb") as errors:
            result = subprocess.run([str(self.probe), str(directory), backend, workload], cwd=self.workdir,
                                    stdout=output, stderr=errors, timeout=30, creationflags=subprocess.CREATE_NO_WINDOW)
        self.check(result.returncode == 0, "Structured report probe rejected the game report.")
        if result.returncode != 0:
            return
        observation = json.loads((self.output / f"{name}.probe.json").read_text(encoding="utf-8-sig"))
        self.current_case["observation"] = observation
        self.check(observation["world_seed"] == 424242, "Actual world seed differs from requested 424242.")
        with (directory / "frames.csv").open(encoding="utf-8-sig", newline="") as source:
            frames = list(csv.DictReader(source))
        observation["frame_rows"] = len(frames)
        if backend == "D3D12":
            ids = [row["renderer_frame_id"] for row in frames]
            self.check(len(ids) == len(set(ids)) and "" not in ids, "Renderer frame IDs are not unique and explicit.")
            uploaded = any(int(row["resource_upload_bytes"]) > 0 for row in frames)
            self.check((workload == "edit") == uploaded, "Persistent geometry upload does not match the workload.")
            self.check(all((row["gpu_draw_ms"] == "") == (row["gpu_missing_reason"] != "") for row in frames),
                       "GPU sample validity and missing reason disagree.")
            self.check(all(int(row["upload_calls"]) == int(row["rebuilt_chunks"]) for row in frames),
                       "Cache operations differ from published mesh rebuilds.")
        if workload == "walk":
            self.check(len({row["x"] for row in frames}) > 1, "Walk workload did not move the actual player.")

    def run(self) -> None:
        try:
            self.manifest["package_manifest"] = read_package_manifest(self.package)
            require(not self.arguments.include_opengl_comparison
                    or "opengl" in self.manifest["package_manifest"]["available_renderers"],
                    "OpenGL comparison requires a transition candidate.")
            self.manifest["package_identity"] = package_identity(self.package)
            relocated = self.output / "package with spaces"
            self.copy_package(relocated)
            self.manifest["relocated_package"] = str(relocated)
            executable = relocated / "SymoCraft.exe"
            self.run_process("d3d12-assets", executable, ["--renderer", "d3d12", "--check-assets"], 0)
            stdout, _ = self.logs()
            self.check("All required assets are present." in stdout, "Asset preflight did not confirm installed assets.")
            self.run_process("d3d12-uncollected-400-frames", executable,
                             ["--renderer", "d3d12", "--smoke-frames", "400", "--scene", "regression", "--seed", "424242",
                              "--msaa", "4", "--validation", "1"], 0)
            stdout, stderr = self.logs()
            self.check("[renderer] backend=D3D12; device=NVIDIA GeForce RTX 5070 Ti;" in stdout and
                       "[runtime] ready; chunks=441" in stdout and "[runtime] loop finished; rendered_frames=400;" in stdout and
                       "[runtime] shutdown complete; exit_code=0" in stdout,
                       "Uncollected ordinary loop did not complete 400 native frames and normal cleanup.")
            self.check("[runtime] fatal:" not in stderr and "Debug Layer warning/error" not in stderr
                       and "[performance] exported;" not in stdout, "Uncollected loop failed or unexpectedly created a collector.")
            self.current_case["observation"] = {"renderer": "D3D12", "world_chunks": 441, "rendered_frames": 400,
                                                "benchmark_collector": False, "output_debug_string_captured": False,
                                                "gpu_ring_drain_exercised": self.current_case["success"]}
            self.manifest["installed_candidate_directory_exercised"] = self.current_case["success"]
            backends = ["d3d12", "opengl"] if self.arguments.include_opengl_comparison else ["d3d12"]
            for backend in backends:
                for workload, duration in (("static", "2"), ("walk", "3"), ("edit", "9")):
                    name = f"{backend}-{workload}"
                    directory = self.output / name
                    arguments = ["--renderer", backend, "--benchmark", workload, "--output", str(directory),
                                 "--seed", "424242", "--width", "1920", "--height", "1080", "--vsync", "0",
                                 "--warmup-seconds", "0", "--sample-seconds", duration, "--focus-policy", "allow-unfocused"]
                    if backend == "d3d12":
                        arguments += ["--msaa", "4", "--validation", "1"]
                    self.run_process(name, executable, arguments, 0)
                    stdout, stderr = self.logs()
                    self.check("[runtime] ready; chunks=441" in stdout and "[runtime] shutdown complete; exit_code=0" in stdout,
                               "Game did not reach the complete world and exit normally.")
                    self.check("[runtime] fatal:" not in stderr and "[renderer] D3D12" not in stderr
                               and "Debug Layer warning/error" not in stderr, "Normal game run retained a fatal/renderer diagnostic.")
                    if self.current_case["success"]:
                        self.verify_frames(directory, "D3D12" if backend == "d3d12" else "OpenGL", workload)
                    self.save()
                    print(f"{name} success={self.current_case['success']} exit={self.current_case['exit_code']}", flush=True)
            self.run_faults(executable)
            read_package_manifest(relocated)
            read_package_manifest(self.package)
            self.manifest["package_identity_after"] = package_identity(self.package)
            require(self.manifest["package_identity"] == self.manifest["package_identity_after"], "Original package changed during verification.")
            require(identity(self.probe) == self.manifest["report_probe_identity"], "Report probe changed during verification.")
            require(identity(Path(__file__).resolve()) == self.manifest["runner_identity"], "Candidate runner changed during verification.")
            self.manifest["success"] = all(case["success"] for case in self.manifest["cases"])
            workload_cases = [case for case in self.manifest["cases"] if case["name"] in {"d3d12-static", "d3d12-walk", "d3d12-edit"}]
            self.manifest["fixed_complete_world_short_workloads"] = len(workload_cases) == 3 and all(case["success"] for case in workload_cases)
        except Exception as error:
            if self.current_case:
                self.check(False, str(error))
            self.manifest["failure"] = str(error)
            raise
        finally:
            self.manifest["finished_utc"] = utc_now()
            self.manifest["artifacts"] = [dict(entry, path=str(self.output / entry["path"])) for entry in package_identity(self.output)
                                          if Path(entry["path"]).name != "verification.json" and Path(entry["path"]).suffix.lower() not in BINARY_SUFFIXES]
            self.save()
        require(self.manifest["success"], "Candidate verification failed; preserved in verification.json.")

    def run_faults(self, executable: Path) -> None:
        faults = (
            ("missing-dxil", "assets/shaders/d3d12/blocks.vs.dxil", None, 2, "asset-presence-preflight", "D3D12 missing, empty or inaccessible asset:"),
            ("corrupt-dxil", "assets/shaders/d3d12/blocks.vs.dxil", b"This isolated fixture is not DXIL.\n", 3, "renderer-construction", "CreateGraphicsPipelineState(mesh) failed"),
            ("corrupt-atlas", "assets/textures/texture_atlas.png", b"This isolated fixture is not a PNG.\n", 3, "atlas-decode", "Cannot decode image:"),
            ("corrupt-block-configuration", "assets/configs/blockFormats.yaml", b"isolated_bad_mapping: [\n", 3, "block-configuration-parse", "Failed to load block configuration:"),
        )
        for name, asset, content, expected_exit, phase, diagnostic in faults:
            fixture = self.output / f"{name} package with spaces"
            self.copy_package(fixture)
            before = package_identity(fixture)
            fault_file = checked_path(fixture / asset, scoped=True)
            require(fault_file.is_relative_to(fixture) and fault_file.relative_to(fixture).as_posix() == asset,
                    "Fault file escaped its isolated package.")
            if content is None:
                fault_file.unlink()
            else:
                fault_file.write_bytes(content)
            after = package_identity(fixture)
            require([entry for entry in before if entry["path"] != asset] == [entry for entry in after if entry["path"] != asset],
                    "Fault injection changed an unrelated package file.")
            self.run_process(name, fixture / "SymoCraft.exe", ["--renderer", "d3d12", "--smoke-frames", "1", "--msaa", "4", "--validation", "1"], expected_exit)
            stdout, stderr = self.logs()
            self.check(diagnostic in stderr, "Original injected-asset failure diagnostic was lost.")
            self.check("[runtime] ready;" not in stdout, "Asset failure incorrectly reached readiness.")
            self.check("OpenGL renderer:" not in stdout, "Asset fault silently fell back to OpenGL.")
            category_match = re.search(r"; category=([A-Za-z]+)", stderr)
            category = category_match.group(1) if category_match else None
            native_match = re.search(r"; native=([^;\r\n]*)", stderr)
            if name == "missing-dxil":
                self.check("blocks.vs.dxil" in stderr and "Monitor size:" not in stdout, "Missing DXIL was not rejected before window creation.")
            else:
                self.check("[runtime] shutdown complete; exit_code=3" in stdout, "Runtime asset failure lacked its final cleanup marker.")
                if name == "corrupt-dxil":
                    self.check("[renderer] D3D12 CreatePipelines:" in stderr and category == "InitializationFailure"
                               and re.search(r"; native=0x[0-9A-Fa-f]{8}", stderr) is not None and "blocks.vs.dxil" in stderr,
                               "Corrupt DXIL lacks backend, category, native code or source asset.")
                else:
                    self.check("[renderer] backend=D3D12; device=NVIDIA GeForce RTX 5070 Ti;" in stdout and
                               "[runtime] fatal:" in stderr and "; requested_backend=d3d12" in stderr,
                               "Decoded/parsed asset fault lacks initialized and selected backend identity.")
            self.current_case["observation"] = {"selected_backend": "D3D12", "failure_phase": phase, "reported_category": category,
                                                "reported_native_code": native_match.group(1) if native_match else None,
                                                "reported_error_family": "RenderError" if category else "asset-preflight" if name == "missing-dxil" else "std::exception",
                                                "actual_device_lost": False, "actual_in_flight_timeout": False, "fault_asset": asset,
                                                "package_identity_before": before, "package_identity_after": after,
                                                "original_package_modified": False}
            self.save()
            print(f"{name} success={self.current_case['success']} exit={self.current_case['exit_code']}", flush=True)
        for name, arguments, expected_exit, diagnostic, backend in (
            ("invalid-backend", ["--renderer", "unknown"], 64, "--renderer", None),
            ("unavailable-vulkan", ["--renderer", "vulkan", "--smoke-frames", "1"], 3, "[renderer] Vulkan SelectBackend:", "Vulkan"),
        ):
            self.run_process(name, executable, arguments, expected_exit)
            stdout, stderr = self.logs()
            self.check(diagnostic in stderr and "Monitor size:" not in stdout and "[runtime] ready;" not in stdout,
                       "Invalid/unimplemented backend was not rejected before window creation.")
            if backend:
                self.check("; category=Unsupported" in stderr, "Unimplemented backend lacks Unsupported category.")
            self.current_case["observation"] = {"failure_phase": "argument-or-backend-selection", "requested_backend": backend,
                                                "actual_device_created": False, "cleanup_not_required_before_window": True}
            self.save()


def main() -> int:
    try:
        Verification(parse_arguments()).run()
        return 0
    except Exception as error:
        print(f"Candidate verification failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
