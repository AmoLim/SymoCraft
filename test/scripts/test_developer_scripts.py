"""Headless regression coverage for the daily Python script entry points."""

import contextlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import scripts_common as common
import build
import benchmark


def load(name):
    spec = importlib.util.spec_from_file_location(name.replace("-", "_"), ROOT / "scripts" / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


runtime = load("verify-runtime")
faults = load("verify-runtime-faults")
capture = load("capture-migration-inputs")


class ScriptTests(unittest.TestCase):
    def setUp(self):
        (ROOT / "out").mkdir(exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="python-scripts-", dir=ROOT / "out")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.exe = self.root / "game \u6d4b\u8bd5.exe"
        self.exe.write_bytes(b"fixture")

    def arguments(self, parser, directory="evidence", extra=()):
        return parser.parse_args(["--executable", str(self.exe), "--output-directory", str(self.root / directory), *extra])

    def fake_runtime(self, executable, arguments, directory, timeout):
        (directory / "stdout.log").write_text(self.stdout, encoding="utf-8")
        (directory / "stderr.log").write_text(self.stderr, encoding="utf-8")
        return {"processId": 42, "exitCode": self.code, "timedOut": self.timed_out, "elapsedSeconds": 0.1}

    def runtime_case(self, stdout, stderr="", code=0, timed_out=False, extra=(), directory="evidence"):
        self.stdout, self.stderr, self.code, self.timed_out = stdout, stderr, code, timed_out
        args = self.arguments(runtime.parser(), directory, extra)
        with patch.object(runtime, "run_logged", side_effect=self.fake_runtime):
            return runtime.probe(args)

    def test_runtime_success_preserves_explicit_seed_zero(self):
        result = self.runtime_case("[runtime] ready;\n[runtime] loop finished; rendered_frames=120\n[runtime] shutdown complete; exit_code=0\n",
                                   extra=("--seed", "0"))
        self.assertTrue(result["passed"])
        self.assertEqual(result["arguments"], ["--smoke-frames", "120", "--seed", "0"])
        self.assertEqual(json.loads((self.root / "evidence/result.json").read_text())["renderedFrames"], 120)

    def test_runtime_rejects_wrong_frame_count(self):
        self.assertFalse(self.runtime_case("[runtime] ready;[runtime] loop finished; rendered_frames=1\n[runtime] shutdown complete;")["passed"])

    def test_runtime_rejects_missing_markers_and_gl_diagnostic(self):
        self.assertFalse(self.runtime_case("", directory="missing")["passed"])
        self.assertFalse(self.runtime_case("[runtime] ready;[runtime] loop finished; rendered_frames=120\n[runtime] shutdown complete;",
                                            "OpenGL diagnostic", directory="diagnostic")["passed"])

    def test_runtime_timeout_and_expected_failure(self):
        self.assertFalse(self.runtime_case("", timed_out=True, directory="timeout")["passed"])
        self.assertTrue(self.runtime_case("[runtime] shutdown complete; exit_code=3", code=3,
                                         extra=("--expected-exit-code", "3"), directory="expected")["passed"])
        self.assertTrue(self.runtime_case("", code=2, extra=("--expected-exit-code", "2"), directory="preflight")["passed"])

    def test_runtime_existing_output_is_untouched(self):
        directory = common.new_directory(self.root / "evidence")
        sentinel = directory / "sentinel.txt"
        sentinel.write_text("unchanged")
        with self.assertRaises(ValueError):
            runtime.probe(self.arguments(runtime.parser()))
        self.assertEqual(sentinel.read_text(), "unchanged")

    def test_regression_parameters_fail_before_output_creation(self):
        args = self.arguments(runtime.parser(), extra=("--checkpoint", "spawn"))
        with self.assertRaisesRegex(ValueError, "regression"):
            runtime.probe(args)
        self.assertFalse(Path(args.OutputDirectory).exists())

    def test_runtime_legacy_options(self):
        args = runtime.parser().parse_args(["-Executable", "game.exe", "-OutputDirectory", "out", "-Seed", "0",
                                           "-Scene", "regression", "-TestEdits"])
        self.assertTrue(args.TestEdits)
        self.assertEqual(args.Seed, 0)

    def test_statistics_nearest_rank_and_empty(self):
        self.assertEqual(benchmark.stats([]), {"count": 0})
        self.assertEqual(benchmark.stats([30, 10]), {"count": 2, "mean": 20, "p95": 30, "p99": 30, "max": 30})
        self.assertEqual(benchmark.stats(range(1, 101))["p95"], 95)

    def test_cli_bounds(self):
        with contextlib.redirect_stderr(io.StringIO()):
            for flag, value in (("--width", "1"), ("--seed", "-1"), ("--repeats", "11"), ("--sample-seconds", "0")):
                with self.assertRaises(SystemExit):
                    self.arguments(benchmark.parser(), extra=(flag, value))
        args = build.parser().parse_args(["--clion-path", "clion", "--cmake-path", "cmake"])
        self.assertEqual(args.CLionPath, "clion")
        self.assertEqual(benchmark.parser().parse_args(["-Executable", "x", "-OutputDirectory", "y", "--vsync", "1"]).VSync, 1)

    def test_benchmark_malformed_csv_retains_failure_report(self):
        run = common.new_directory(self.root / "run")
        (run / "stdout.log").write_text("[performance] exported; valid_run=1\n[runtime] shutdown complete; exit_code=0")
        (run / "stderr.log").write_text("")
        (run / "capture").mkdir()
        (run / "capture/frames.csv").write_text("phase,frame_ms\nsample,not-a-number\n")
        result = benchmark.analyze(run, "static", 1, [], {"exitCode": 0, "timedOut": False}, "missing")
        self.assertFalse(result["passed"])
        self.assertIn("frames-csv-invalid", result["failureReasons"][0])
        (run / "capture/frames.csv").unlink()
        result = benchmark.analyze(run, "static", 1, [], {"exitCode": 0, "timedOut": False}, "missing")
        self.assertEqual(result["failureReasons"], ["frames-csv-missing"])

    def test_benchmark_sensor_is_stopped_when_game_launch_fails(self):
        args = self.arguments(benchmark.parser(), extra=("--scenarios", "static", "--repeats", "1"))
        sensor = unittest.mock.Mock()
        sensor.poll.return_value = None
        with patch.object(benchmark, "machine_record", return_value={}), patch.object(benchmark.shutil, "which", return_value="smi"), \
             patch.object(benchmark.subprocess, "Popen", return_value=sensor), \
             patch.object(benchmark, "run_logged", side_effect=OSError("launch failed")):
            with self.assertRaises(OSError):
                benchmark.collect(args)
        sensor.kill.assert_called_once()
        sensor.wait.assert_called_once()

    def test_real_process_logs_and_timeout_cleanup(self):
        run = common.new_directory(self.root / "real process \u6d4b\u8bd5")
        process = common.run_logged(sys.executable, ["-c", "import sys; print('ok'); print('error', file=sys.stderr); sys.exit(3)"], run, 10)
        self.assertEqual(process["exitCode"], 3)
        self.assertFalse(process["timedOut"])
        self.assertIn("ok", common.read_log(run / "stdout.log"))
        process = common.run_logged(sys.executable, ["-c", "import time; time.sleep(30)"], run, 0.1)
        self.assertTrue(process["timedOut"])
        self.assertIsNotNone(process["exitCode"])

    def test_keyboard_interrupt_stops_owned_process(self):
        run = common.new_directory(self.root / "interrupted")
        child = unittest.mock.Mock()
        child.wait.side_effect = [KeyboardInterrupt(), None]
        child.poll.return_value = None
        with patch.object(common.subprocess, "Popen", return_value=child):
            with self.assertRaises(KeyboardInterrupt):
                common.run_logged("fixture", [], run, 10)
        child.kill.assert_called_once()
        self.assertEqual(child.wait.call_count, 2)

    def test_build_custom_directory_does_not_use_test_preset(self):
        args = build.parser().parse_args(["--build-directory", "out/custom", "--action", "Install"])
        commands = build.build_commands(args, "cmake", "ninja", "cl", "ctest")
        self.assertEqual(len(commands), 4)
        self.assertIn("--test-dir", commands[2])
        self.assertNotIn("--preset", commands[2])
        self.assertIn("--no-tests=error", commands[2])
        self.assertIn(f"-DPython3_EXECUTABLE={sys.executable}", commands[0])
        self.assertEqual(commands[3][:2], ["cmake", "--install"])

    def test_build_default_actions(self):
        for action, count in (("Configure", 1), ("Build", 2), ("Test", 3), ("Install", 4)):
            args = build.parser().parse_args(["--action", action])
            commands = build.build_commands(args, "cmake", "ninja", "cl", "ctest")
            self.assertEqual(len(commands), count)
            if count >= 3:
                self.assertEqual(commands[2][1:3], ["--preset", "windows-debug"])

    def test_developer_environment_isolated_and_unicode_safe(self):
        studio = self.root / "VS \u6d4b\u8bd5!"
        (studio / "Common7/Tools").mkdir(parents=True)
        (studio / "Common7/Tools/VsDevCmd.bat").write_text("fixture")
        inherited = {"PATH": "original", "COMSPEC": "cmd.exe"}
        output = "__SYMOCRAFT_DEVELOPER_ENVIRONMENT__\r\nPath=tools\r\nINCLUDE=C:\\a=b\\\u6d4b\u8bd5\r\n=C:=ignored\r\n".encode("utf-16-le")
        with patch.object(build.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, output, b"")) as launch:
            environment = build.developer_environment(studio, "14.38", inherited)
        self.assertEqual(inherited["PATH"], "original")
        self.assertEqual(environment["PATH"], "tools")
        self.assertEqual(environment["INCLUDE"], "C:\\a=b\\\u6d4b\u8bd5")
        self.assertIn("/v:off", launch.call_args.args[0])
        self.assertIn("-vcvars_ver=14.38", launch.call_args.args[0])
        with self.assertRaises(ValueError):
            build.developer_environment(studio, "14.38 & echo unwanted", inherited)

    def test_snapshot_real_git_and_inventory_preserve_bytes(self):
        workspace = self.root / "snapshot source"
        workspace.mkdir()
        for directory in ("game", "cmake", "test", "scripts", "tools/benchmark", "vendor", "assets"):
            (workspace / directory).mkdir(parents=True)
            (workspace / directory / "input.txt").write_text("source \u6d4b\u8bd5\n", encoding="utf-8")
        for relative in ("CMakeLists.txt", "CMakePresets.json", "tools/benchmark/README.md", "docs/third-party.md"):
            path = workspace / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"original\r\n")
        (workspace / "test/experimental").mkdir()
        (workspace / "test/experimental/ignored.txt").write_text("ignored")
        for args in (("init",), ("add", "."), ("-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid", "commit", "-m", "fixture")):
            subprocess.run(["git", *args], cwd=workspace, capture_output=True, check=True)
        (workspace / "game/input.txt").write_bytes(b"dirty\r\n")
        result = capture.capture(Path("out/evidence"), root=workspace)
        output = Path(result["output"])
        manifest = json.loads((output / "build-inputs.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["count"], 11)
        captured_paths = {file["path"] for file in manifest["files"]}
        self.assertEqual(len(captured_paths), manifest["count"])
        self.assertIn("tools/benchmark/README.md", captured_paths)
        self.assertIn("docs/third-party.md", captured_paths)
        self.assertNotIn("docs/project/benchmark/\u4f7f\u7528\u4e0e\u4ea4\u4ed8.md", captured_paths)
        self.assertNotIn("docs/legacy/third-party-inventory.md", captured_paths)
        self.assertEqual((output / "source/game/input.txt").read_bytes(), b"dirty\r\n")
        self.assertNotIn("experimental", str(manifest["files"]))
        patch_bytes = subprocess.run(["git", "diff", "--binary", "--", ".", ":!vendor/sdl3"], cwd=workspace, capture_output=True, check=True).stdout
        self.assertEqual((output / "tracked-changes.patch").read_bytes(), patch_bytes)
        for file in manifest["files"]:
            self.assertEqual(common.sha256(output / "source" / file["path"]), file["sha256"])
        with self.assertRaises(ValueError):
            capture.capture(Path("out/evidence"), root=workspace)
        with self.assertRaises(ValueError):
            capture.capture(Path("../outside"), root=workspace)

    def test_faults_only_modify_copied_package(self):
        assets = self.root / "assets"
        for _, asset, _, _ in faults.CASES:
            path = assets / asset
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("original")
        before = {asset: common.sha256(assets / asset) for _, asset, _, _ in faults.CASES}

        def fake_probe(command, **kwargs):
            executable = Path(command[command.index("--executable") + 1])
            evidence = common.new_directory(command[command.index("--output-directory") + 1])
            case = next(case for case in faults.CASES if case[0] == executable.parent.parent.name)
            self.assertEqual((executable.parent / "assets" / case[1]).read_text(), case[2])
            common.write_json(evidence / "result.json", {"exitCode": 3, "timedOut": False, "ready": False, "shutdown": True, "passed": True})
            (evidence / "stdout.log").write_text("OpenGL vendor: fixture\nOpenGL renderer: fixture\nOpenGL version: fixture\n[runtime] shutdown complete; exit_code=3\n")
            (evidence / "stderr.log").write_text("[runtime] fatal: " + case[3])
            return subprocess.CompletedProcess(command, 0, "", "")

        with patch.object(faults.subprocess, "run", side_effect=fake_probe):
            result = faults.verify(self.exe, self.root / "faults")
        self.assertTrue(result["passed"])
        self.assertEqual(len(result["cases"]), 3)
        for asset, digest in before.items():
            self.assertEqual(common.sha256(assets / asset), digest)
        with self.assertRaises(ValueError):
            faults.verify(self.exe, assets / "unsafe-output")
        self.assertFalse((assets / "unsafe-output").exists())


if __name__ == "__main__":
    unittest.main()
