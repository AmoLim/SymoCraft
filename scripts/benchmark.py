"""Collect benchmark runs with strict validity checks and retained evidence."""

import argparse
import csv
import ctypes
from datetime import datetime
import math
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess
import sys

from scripts_common import cli, file_path, hidden_process, integer, new_directory, option, read_log, run_logged, sha256, stop_process, write_json


def stats(values):
    values = sorted(values)
    if not values:
        return {"count": 0}
    return {"count": len(values), "mean": sum(values) / len(values),
            "p95": values[math.ceil(len(values) * 0.95) - 1],
            "p99": values[math.ceil(len(values) * 0.99) - 1], "max": values[-1]}


def try_read(reader):
    try:
        return reader()
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        return f"unavailable: {error}"


def windows_hardware():
    import winreg
    from ctypes import wintypes

    def registry(path, names):
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, path) as key:
            return {name: winreg.QueryValueEx(key, name)[0] for name in names}

    kernel = ctypes.WinDLL("kernel32", use_last_error=True)

    def check(success):
        if not success:
            raise ctypes.WinError(ctypes.get_last_error())

    def cpu():
        values = registry(r"HARDWARE\DESCRIPTION\System\CentralProcessor\0", ("ProcessorNameString",))
        size = wintypes.DWORD()
        kernel.GetLogicalProcessorInformationEx(0, None, ctypes.byref(size))
        if not size.value:
            raise ctypes.WinError(ctypes.get_last_error())
        buffer = ctypes.create_string_buffer(size.value)
        check(kernel.GetLogicalProcessorInformationEx(0, buffer, ctypes.byref(size)))
        offset, cores = 0, 0
        while offset < size.value:
            relationship, length = struct.unpack_from("<II", buffer.raw, offset)
            if length < 8 or offset + length > size.value:
                raise ValueError("Invalid processor topology record.")
            cores += relationship == 0
            offset += length
        return {"Name": values["ProcessorNameString"], "NumberOfCores": cores,
                "NumberOfLogicalProcessors": os.cpu_count()}

    def memory():
        class MemoryStatus(ctypes.Structure):
            _fields_ = [("length", wintypes.DWORD), ("load", wintypes.DWORD),
                        *[(name, ctypes.c_ulonglong) for name in ("physical", "available", "pagefile", "availablePagefile", "virtual", "availableVirtual", "extended")]]

        status = MemoryStatus()
        status.length = ctypes.sizeof(status)
        check(kernel.GlobalMemoryStatusEx(ctypes.byref(status)))
        return {"TotalPhysicalMemory": status.physical}

    def battery():
        class PowerStatus(ctypes.Structure):
            _fields_ = [("ac", wintypes.BYTE), ("flags", wintypes.BYTE), ("percent", wintypes.BYTE),
                        ("reserved", wintypes.BYTE), ("life", wintypes.DWORD), ("fullLife", wintypes.DWORD)]

        status = PowerStatus()
        check(kernel.GetSystemPowerStatus(ctypes.byref(status)))
        return {"ACLineStatus": status.ac, "BatteryFlag": status.flags,
                "EstimatedChargeRemaining": None if status.percent == 255 else status.percent}

    def displays():
        values = []
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\Video") as key:
            for index in range(winreg.QueryInfoKey(key)[0]):
                name = winreg.EnumKey(key, index)
                try:
                    record = registry(r"SYSTEM\CurrentControlSet\Control\Video" + "\\" + name + r"\0000", ("DriverDesc", "DriverVersion"))
                    values.append({"Name": record["DriverDesc"], "DriverVersion": record["DriverVersion"]})
                except OSError:
                    continue
        return values or "unavailable: display driver registry values absent"

    return {"cpu": try_read(cpu), "memory": try_read(memory), "battery": try_read(battery),
            "displayControllers": try_read(displays),
            "activePowerScheme": try_read(lambda: subprocess.run(["powercfg", "/getactivescheme"],
                capture_output=True, text=True, check=True, timeout=15, **hidden_process()).stdout.strip())}


def machine_record(args, executable):
    missing = "unavailable: no portable standard-library sensor"
    hardware = {"cpu": {"Name": platform.processor(), "NumberOfLogicalProcessors": os.cpu_count()},
                "memory": missing, "battery": missing, "displayControllers": missing, "activePowerScheme": missing}
    if os.name == "nt":
        hardware.update(windows_hardware())
    return {"startedAt": datetime.now().astimezone().isoformat(), "collectorScriptVersion": 3,
            "pythonVersion": platform.python_version(), "label": args.MachineLabel,
            "powerMode": args.PowerMode, "gpuMode": args.GpuMode, "clockSettings": args.ClockSettings,
            "cpuTemperature": "missing; no CPU temperature sensor configured",
            "operatingSystem": {"Caption": platform.system(), "Version": platform.version(),
                                "BuildNumber": str(sys.getwindowsversion().build) if os.name == "nt" else platform.release()},
            **hardware, "hardwareSource": "Python platform, Windows registry and Win32 APIs (not CIM); battery uses native flags",
            "executable": str(executable), "executableSha256": sha256(executable).upper(),
            "scriptSha256": sha256(__file__).upper(),
            "helperSha256": sha256(Path(__file__).with_name("scripts_common.py")).upper(),
            "seed": args.Seed, "width": args.Width, "height": args.Height, "vsync": args.VSync,
            "warmupSeconds": args.WarmupSeconds, "sampleSeconds": args.SampleSeconds, "repeats": args.Repeats,
            "requestedScenarios": args.Scenarios,
            "gpuSensorScope": "nvidia-smi samples all GPUs; device-wide temperature/memory, not process VRAM",
            "sourceNote": "Keep build identity with this directory. This script does not infer the source commit from an executable."}


def analyze(run, scenario, repeat, arguments, process, sensor_status):
    stdout = read_log(run / "stdout.log")
    stderr = read_log(run / "stderr.log")
    failures = []
    if process["timedOut"]:
        failures.append("process-timeout")
    if process["exitCode"] != 0:
        failures.append(f"process-exit-code-{process['exitCode']}")
    if "[performance] exported; valid_run=1" not in stdout:
        failures.append("valid-export-marker-missing")
    if "[runtime] shutdown complete; exit_code=0" not in stdout:
        failures.append("successful-shutdown-marker-missing")
    if "OpenGL diagnostic" in stderr:
        failures.append("opengl-diagnostic")
    result = {"scenario": scenario, "repeat": repeat, "arguments": arguments, **process,
              "gpuSensorStatus": sensor_status}
    csv_path = run / "capture/frames.csv"
    if csv_path.is_file():
        try:
            with csv_path.open(encoding="utf-8-sig", newline="") as source:
                rows = list(csv.DictReader(source))
            sample = [row for row in rows if row["phase"] == "sample"]
            for field, column in (("frameMs", "frame_ms"), ("gpuDrawMs", "gpu_draw_ms"), ("uploadBytes", "upload_bytes"),
                                  ("uploadCpuMs", "upload_cpu_ms"), ("meshMs", "mesh_ms"), ("presentMs", "present_ms"),
                                  ("playerX", "x"), ("playerY", "y")):
                numbers = [float(row[column]) for row in sample if column != "gpu_draw_ms" or row[column] != ""]
                if not all(math.isfinite(number) for number in numbers):
                    raise ValueError(f"Non-finite values in {column}")
                result[field] = stats(numbers)
            result["editFrames"] = sum(float(row["edits"]) > 0 for row in sample)
            result["focusLostFrames"] = sum(row["focused"] != "1" for row in rows)
            if not sample:
                failures.append("no-sample-frames")
            if result["focusLostFrames"]:
                failures.append(f"window-not-focused: {result['focusLostFrames']} frames")
        except (ValueError, KeyError, csv.Error) as error:
            failures.append(f"frames-csv-invalid: {error}")
    else:
        failures.append("frames-csv-missing")
    result.update(failureReasons=failures, passed=not failures)
    return result


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    option(result, "Executable", required=True)
    option(result, "OutputDirectory", required=True)
    option(result, "Scenarios", nargs="+", choices=("static", "walk", "edit"), default=["static", "walk", "edit"])
    for name, low, high, default in (("WarmupSeconds", 0, 600, 60), ("SampleSeconds", 1, 600, 180),
                                     ("Repeats", 1, 10, 3), ("Width", 320, 8192, 1920), ("Height", 320, 8192, 1080),
                                     ("Seed", 0, 2**32 - 1, 424242)):
        option(result, name, type=integer(low, high), default=default)
    option(result, "VSync", type=int, choices=(0, 1), default=0)
    for name in ("MachineLabel", "PowerMode", "GpuMode", "ClockSettings"):
        option(result, name, default="unrecorded")
    option(result, "SkipGpuSensors", action="store_true")
    return result


def collect(args):
    executable = file_path(args.Executable)
    output = new_directory(args.OutputDirectory)
    write_json(output / "machine.json", machine_record(args, executable))
    smi = None if args.SkipGpuSensors else shutil.which("nvidia-smi")
    results = []
    for scenario in args.Scenarios:
        for repeat in range(1, args.Repeats + 1):
            run = new_directory(output / f"{scenario}-{repeat}")
            arguments = ["--benchmark", scenario, "--output", "capture", "--seed", str(args.Seed),
                         "--width", str(args.Width), "--height", str(args.Height), "--vsync", str(args.VSync),
                         "--warmup-seconds", str(args.WarmupSeconds), "--sample-seconds", str(args.SampleSeconds)]
            monitor = None
            with (run / "gpu-sensors.csv").open("wb") if smi else open(os.devnull, "wb") as sensor_out, \
                 (run / "gpu-sensors.stderr.log").open("wb") if smi else open(os.devnull, "wb") as sensor_err:
                try:
                    if smi:
                        monitor = subprocess.Popen([smi, "--query-gpu=timestamp,uuid,name,driver_version,temperature.gpu,power.draw,clocks.current.graphics,clocks.current.memory,memory.used,memory.total,utilization.gpu",
                                                    "--format=csv", "--loop-ms=1000"], stdout=sensor_out, stderr=sensor_err, **hidden_process())
                    print(f"Running {scenario} {repeat}/{args.Repeats}; {args.WarmupSeconds} s warmup + {args.SampleSeconds} s sampling. Keep the game focused; Esc cancels.", flush=True)
                    process = run_logged(executable, arguments, run, args.WarmupSeconds + args.SampleSeconds + 120)
                    status = "missing" if monitor is None else (
                        "collector-exited; inspect sensor stderr" if monitor.poll() is not None else "collected; inspect CSV for unsupported fields")
                    result = analyze(run, scenario, repeat, arguments, process, status)
                    results.append(result)
                    write_json(run / "result.json", result)
                    write_json(output / "results.json", results)
                    if not result["passed"]:
                        raise RuntimeError(f"Benchmark invalid or failed ({'; '.join(result['failureReasons'])}); inspect {run}. Remaining runs were not started.")
                finally:
                    stop_process(monitor)
    print(f"Finished {len(results)} runs. Raw data and machine conditions: {output}")
    return results


def main():
    collect(parser().parse_args())


if __name__ == "__main__":
    cli(main)
