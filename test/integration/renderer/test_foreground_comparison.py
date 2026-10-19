import argparse
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import MagicMock, patch

spec = importlib.util.spec_from_file_location("comparison", Path(__file__).with_name("verify-renderer-comparison.py"))
comparison = importlib.util.module_from_spec(spec)
spec.loader.exec_module(comparison)

class ForegroundComparisonTests(unittest.TestCase):
    def test_foreground_launch_does_not_hide_gui(self):
        self.assertEqual(comparison.foreground_process_options(),
                         {"creationflags": comparison.subprocess.CREATE_NO_WINDOW})

    def test_old_default_stays_unfocused_and_two_backends(self):
        argv = ["collector", "--executable", "game", "--native-consumer", "native", "--report-probe", "probe", "--output-directory", "new"]
        with patch.object(comparison.sys, "argv", argv), patch.object(comparison, "collect") as collect:
            self.assertEqual(comparison.main(), 0)
        args = collect.call_args.args[0]
        self.assertEqual((args.focus_mode, args.warmup_seconds, args.sample_seconds, args.repeats), ("allow-unfocused", 10, 50, 3))
        self.assertEqual(args.backends, ["opengl", "d3d12"])

    def test_foreground_command_preserves_configuration(self):
        for backend in ("opengl", "d3d12", "vulkan"):
            command = comparison.game_command("game", backend, "edit", "capture", 30, 90, "foreground")
            self.assertEqual(command[command.index("--focus-policy") + 1], "strict")
            self.assertEqual(command[command.index("--sample-seconds") + 1], "90")
            self.assertEqual(command[command.index("--warmup-seconds") + 1], "30")
            self.assertEqual(command[-2:], ["--foreground-ready-timeout-seconds", "60"])
            if backend != "opengl": self.assertEqual(command[-6:-2], ["--msaa", "4", "--validation", "1"])
            else: self.assertNotIn("--validation", command)

    def test_owned_foreground_observer_does_not_inject_input(self):
        api, process = MagicMock(), MagicMock()
        process.pid, process.poll.return_value = 42, None
        api.find.return_value = 101
        api.user.IsWindowVisible.return_value = True
        api.user.IsIconic.return_value = False
        api.user.GetForegroundWindow.return_value = 101
        api.pixels.return_value = [1920, 1080]
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "stdout"
            log.touch()
            result = comparison.wait_foreground(process, api, log)
        self.assertTrue(result["foreground"] and result["confirmed_before_ready_log"])
        api.owned.assert_called_once_with(101, 42)
        api.user.SetForegroundWindow.assert_not_called()
        api.user.ShowWindow.assert_not_called()

    def test_focus_timeout_never_returns_success(self):
        with patch.object(comparison.time, "monotonic", side_effect=[0, 2]):
            with self.assertRaisesRegex(ValueError, "Foreground confirmation timed out"):
                comparison.wait_foreground(MagicMock(), MagicMock(), Path("unused"), timeout=1)

    def test_focus_loss_in_warmup_is_not_discarded(self):
        data = [{"phase": "warmup", "focused": "0"}, {"phase": "sample", "focused": "1"}]
        with patch.object(comparison.budget, "rows", return_value=data):
            with self.assertRaisesRegex(ValueError, "unfocused warmup/sample frame"):
                comparison.validate_focus(Path("capture"), {"focus_class": "fully-focused"}, "foreground")
        comparison.validate_focus(Path("unused"), {"focus_class": "mixed"}, "allow-unfocused")

    def test_three_backends_have_27_rounds_and_18_gl_pairs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            game, native, probe = [root / name for name in ("game.exe", "native.exe", "probe.exe")]
            for path in (game, native, probe): path.touch()
            args = argparse.Namespace(executable=game, native_consumer=native, report_probe=probe,
                output_directory=root / "out", backends=["opengl", "d3d12", "vulkan"], warmup_seconds=30,
                sample_seconds=90, repeats=3, focus_mode="foreground")
            summary = dict(frames=1, p95_ms=5., p99_ms=6., working_set_bytes=1000, private_bytes=2000, focus_class="fully-focused")
            def foreground(command, folder, timeout):
                (folder / "capture").mkdir()
                backend = command[command.index("--renderer") + 1]
                text = "[renderer] Vulkan mesh memory: policy=prefer-device-local-host-visible; type=4; flags=0x7; heap=0; host-visible-fallback=false\n[renderer] Vulkan shutdown: validation_warnings=0; validation_errors=0; native_retired=true" if backend == "vulkan" else ""
                (folder / "game.stderr.log").write_text(text, encoding="utf-8")
                return "[benchmark] foreground-ready; preparation_ms=1000\n[runtime] shutdown complete; exit_code=0\n[performance] exported; valid_run=1"
            def execute(command, folder, prefix, timeout):
                if prefix == "native-consumer": return "{}"
                return json.dumps(dict.fromkeys(comparison.SHARED, "same") | {"measured_frames": 1})
            frames = [{"elapsed_s": "30", "focused": "1", "frame_ms": "5"}]
            with patch.object(comparison, "execute_foreground", side_effect=foreground) as games, \
                 patch.object(comparison.budget, "execute", side_effect=execute), \
                 patch.object(comparison.budget, "analyze", return_value=summary), \
                 patch.object(comparison.budget, "rows", return_value=frames), \
                 patch.object(comparison.budget, "identities", return_value={"game": "same"}), \
                 patch.object(comparison.budget, "windows_hardware", return_value={}), patch("builtins.print"):
                result = comparison.collect(args)
            self.assertEqual(games.call_count, 27)
            self.assertEqual([row["backend"] for row in result["rounds"]], ["opengl"] * 9 + ["d3d12"] * 9 + ["vulkan"] * 9)
            self.assertEqual(len(result["pairs"]), 18)
            self.assertTrue(result["collection_valid"] and result["budgets_passed"])
            self.assertFalse(result["formal_acceptance"] or result["automatic_retest"])

if __name__ == "__main__": unittest.main()
