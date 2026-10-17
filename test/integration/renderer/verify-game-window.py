"""Observe the ordinary production game loop across an owned native minimize/restore."""
import argparse
import ctypes
from pathlib import Path
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from scripts_common import hidden_process, stop_process
from verification_common import (ROOT, clean_environment, cli, identity, load_script, new_directory,
                                 read_log, require, utc_now, workspace_path, write_json)
from windows_verification import Windows, wait_ready


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-directory", required=True)
    parser.add_argument("--output-directory", required=True)
    parser.add_argument("--backend", choices=("vulkan", "d3d12"), default="vulkan")
    args = parser.parse_args()
    output = new_directory(workspace_path(args.output_directory, ROOT/"out"))
    build = workspace_path(args.build_directory, ROOT/"out")
    executable = build/"bin/SymoCraft.exe"
    helper = load_script(ROOT/"test/integration/renderer/verify-draw-window.py")
    api = Windows()
    api.kernel.GetProcessTimes.argtypes = [api.w.HANDLE, *([ctypes.POINTER(api.w.FILETIME)]*4)]
    api.kernel.GetProcessTimes.restype = api.w.BOOL
    command = [str(executable), "--renderer", args.backend, "--msaa", "4", "--validation", "1", "--seed", "424242", "--scene", "regression", "--smoke-frames", "10000"]
    result = {"status": "running", "started_utc": utc_now(), "command": command,
              "executable": identity(executable), "cache": identity(build/"CMakeCache.txt"),
              "ordinary_game_entry": True, "performance_session": False, "performance_budget": False,
              "finite_frame_diagnostic": 10000, "interactive_focus_verified": False,
              "human_input_or_esc": False, "forced_termination": False}
    env = clean_environment()
    for key in ("VK_INSTANCE_LAYERS", "VK_APIDUMP_DETAILED", "VK_APIDUMP_FLUSH"):
        env.pop(key, None)
    traced = args.backend == "vulkan"
    if traced:
        env.update(VK_INSTANCE_LAYERS="VK_LAYER_LUNARG_api_dump", VK_APIDUMP_DETAILED="false", VK_APIDUMP_FLUSH="true")
    result["observation_environment"] = {k: v for k, v in env.items() if k.startswith("VK_APIDUMP") or k == "VK_INSTANCE_LAYERS"}
    stdout, stderr = output/"game.stdout.log", output/"game.stderr.log"
    process = None

    def wait(predicate, description, timeout=5):
        deadline = time.monotonic()+timeout
        while time.monotonic()<deadline:
            require(process.poll() is None, "Game exited during "+description)
            if predicate(): return
            time.sleep(.05)
        raise ValueError("Timed out waiting for "+description)

    def cpu_seconds():
        times = [api.w.FILETIME() for _ in range(4)]
        require(api.kernel.GetProcessTimes(api.w.HANDLE(int(process._handle)), *map(ctypes.byref, times)), "GetProcessTimes failed")
        return sum((t.dwHighDateTime<<32)|t.dwLowDateTime for t in times[2:])/10_000_000

    try:
        with stdout.open("wb") as out, stderr.open("wb") as errors:
            process = subprocess.Popen(command, cwd=output, env=env, stdout=out, stderr=errors, **hidden_process())
            result["pid"] = process.pid
            hwnd = wait_ready(process, api, stdout, timeout=60)
            api.owned(hwnd, process.pid)
            require("[runtime] ready; chunks=441;" in read_log(stdout), "Incomplete game world")
            api.user.ShowWindow(hwnd, 9)
            api.user.SetForegroundWindow(hwnd)
            result["initial_foreground"] = api.user.GetForegroundWindow() == hwnd
            if traced: wait(lambda: "vkQueuePresentKHR(" in read_log(stdout), "initial actual present")
            result["initial_client_pixels"] = api.pixels(hwnd, process.pid)
            api.resize(hwnd, process.pid)
            # The adapter requests an outer rectangle; decorated client pixels are smaller.
            wait(lambda: api.pixels(hwnd, process.pid) != result["initial_client_pixels"], "real resized client extent")
            result["resized_client_pixels"] = api.pixels(hwnd, process.pid)
            require(all(result["resized_client_pixels"]), "Resize produced an empty client")
            api.owned(hwnd, process.pid); api.user.ShowWindow(hwnd, 6)
            wait(lambda: bool(api.user.IsIconic(hwnd)), "actual minimize")
            time.sleep(.5)
            begin = stdout.stat().st_size
            cpu_begin, start = cpu_seconds(), time.monotonic()
            while time.monotonic()-start < 10:
                api.owned(hwnd, process.pid)
                require(api.user.IsIconic(hwnd) and process.poll() is None, "Game left actual minimized state")
                time.sleep(.1)
            duration = time.monotonic()-start
            result["pause"] = {"seconds": duration, "process_cpu_seconds": cpu_seconds()-cpu_begin,
                               "stdout_begin": begin, "stdout_end": stdout.stat().st_size}
            if traced:
                data = stdout.read_bytes()[begin:result["pause"]["stdout_end"]].decode("utf-8", errors="replace")
                counts = {name: helper.CALL.findall(data).count(name) for name in helper.QUEUE_CALLS}
                require(not any(counts.values()), "Ordinary paused game issued native presentation work")
                result["pause"]["native_calls"] = counts
            end = stdout.stat().st_size
            api.owned(hwnd, process.pid); api.user.ShowWindow(hwnd, 9); api.user.SetForegroundWindow(hwnd)
            wait(lambda: not api.user.IsIconic(hwnd), "restored native window")
            result["restored_foreground"] = api.user.GetForegroundWindow() == hwnd
            require(api.pixels(hwnd, process.pid) == result["resized_client_pixels"], "Client size changed after restore")
            if traced:
                wait(lambda: "vkQueuePresentKHR(" in stdout.read_bytes()[end:].decode("utf-8", errors="replace"), "resumed native present")
            else:
                time.sleep(.5)
            api.close(hwnd, process.pid)
            process.wait(timeout=20)
        result["exit_code"] = process.returncode
        require(process.returncode == 0 and "[runtime] shutdown complete; exit_code=0" in read_log(stdout), "Ordinary game did not shut down cleanly")
        if traced:
            result["validation"] = helper.diagnostics(read_log(stderr), True)
            calls = helper.CALL.findall(read_log(stdout))
            result["native_counts"] = {name: calls.count(name) for name in (*helper.QUEUE_CALLS, "vkCreateInstance", "vkDestroyInstance", "vkCreateDevice", "vkDestroyDevice", "vkCreateWin32SurfaceKHR", "vkDestroySurfaceKHR", "vkDestroySwapchainKHR")}
            for create,destroy in (("vkCreateInstance","vkDestroyInstance"),("vkCreateDevice","vkDestroyDevice"),("vkCreateWin32SurfaceKHR","vkDestroySurfaceKHR")):
                require(calls.count(create) == calls.count(destroy) == 1, "Ordinary game hot-recreated or leaked a native owner")
            require(calls.count("vkCreateSwapchainKHR") == calls.count("vkDestroySwapchainKHR"), "Ordinary game swapchain generations did not retire")
        else:
            require(not read_log(stderr).strip(), "D3D12 reported normal-game diagnostics")
        result["status"] = "passed"
    except BaseException as error:
        result["status"] = "failed"; result["error"] = str(error); raise
    finally:
        if process is not None:
            result["forced_termination"] = process.poll() is None
            stop_process(process)
        result["finished_utc"] = utc_now()
        write_json(output/"result.json", result)
    print(args.backend, "ordinary game window passed", result["pause"], flush=True)


if __name__ == "__main__":
    cli(main)
