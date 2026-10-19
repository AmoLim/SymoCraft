import csv
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("budget", Path(__file__).with_name("verify-d3d12-budgets.py"))
budget = importlib.util.module_from_spec(spec)
spec.loader.exec_module(budget)


class BudgetTests(unittest.TestCase):
    def test_backend_selection_preserves_d3d12_default(self):
        required = ["collector", "--executable", "game", "--native-consumer", "consumer",
                    "--report-probe", "probe", "--output-directory", "new-output"]
        with patch.object(budget.sys, "argv", required), patch.object(budget, "collect") as collect:
            collect.return_value = {"budgets_passed": True}
            self.assertEqual(budget.main(), 0)
            self.assertEqual(collect.call_args.args[0].modern_backend, "d3d12")
            self.assertIsNone(collect.call_args.args[0].opengl_executable)
        with patch.object(budget.sys, "argv", required + ["--modern-backend", "vulkan", "--opengl-executable", "gl-game"]), patch.object(budget, "collect") as collect:
            collect.return_value = {"budgets_passed": False}
            self.assertEqual(budget.main(), 4)
            self.assertEqual(collect.call_args.args[0].modern_backend, "vulkan")
            self.assertEqual(collect.call_args.args[0].opengl_executable, "gl-game")

    def test_timeout_keeps_partial_logs_and_failure_state(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            error = budget.subprocess.TimeoutExpired(["game"], 1, output=b"partial", stderr=b"reason")
            with patch.object(budget.subprocess, "run", side_effect=error):
                with self.assertRaises(budget.subprocess.TimeoutExpired):
                    budget.execute(["game"], path, "game", 1)
            self.assertEqual((path / "game.stdout.log").read_bytes(), b"partial")
            self.assertEqual((path / "game.stderr.log").read_bytes(), b"reason")
            record = budget.json.loads((path / "game.process.json").read_text())
            self.assertTrue(record["timed_out"] and record["forced_termination"])
            self.assertIsNone(record["exit_code"])

    def test_vulkan_collection_pairs_nine_rounds_with_separate_gl(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            executables = [root / name for name in ("modern.exe", "gl.exe", "consumer.exe", "probe.exe")]
            for executable in executables:
                executable.touch()
            args = budget.argparse.Namespace(executable=str(executables[0]), opengl_executable=str(executables[1]),
                native_consumer=str(executables[2]), report_probe=str(executables[3]),
                output_directory=str(root / "result"), modern_backend="vulkan")
            summary = dict(p95_ms=5., p99_ms=6., working_set_bytes=1000, private_bytes=2000,
                           focus_class="fully-unfocused")
            def execute(command, directory, prefix, timeout):
                if prefix == "game":
                    (directory / "capture").mkdir()
                    return "[runtime] shutdown complete; exit_code=0\n[performance] exported; valid_run=1"
                return "{}"
            with patch.object(budget, "execute", side_effect=execute) as calls, \
                 patch.object(budget, "analyze", return_value=summary), \
                 patch.object(budget, "identities", return_value={"input": "same"}), \
                 patch.object(budget, "sha", return_value="same"), \
                 patch.object(budget, "windows_hardware", return_value={}), patch("builtins.print"):
                result = budget.collect(args)
            self.assertEqual(result["schema"], "symocraft.vulkan-approved-budget-10-50.v1")
            self.assertTrue(result["collection_valid"] and result["budgets_passed"])
            self.assertEqual(len(result["pairs"]), 9)
            games = [call.args[0] for call in calls.call_args_list if call.args[2] == "game"]
            self.assertEqual(len(games), 18)
            self.assertTrue(all(command[0] == executables[1] and command[2] == "opengl" for command in games[:9]))
            self.assertTrue(all(command[0] == executables[0] and command[2] == "vulkan" for command in games[9:]))
            self.assertTrue(all(command[-4:] == ["--msaa", "4", "--validation", "1"] for command in games[9:]))
            probes = [call.args[0] for call in calls.call_args_list if call.args[2] == "game-report"]
            self.assertEqual([command[2] for command in probes], ["OpenGL"] * 9 + ["Vulkan"] * 9)

    def test_nearest_rank_retains_slow_frames(self):
        values = [1.] * 98 + [20., 100.]
        self.assertEqual(budget.rank(values, .95), 1.)
        self.assertEqual(budget.rank(values, .99), 20.)

    def test_explicit_medium_window_does_not_include_warmup_or_tail(self):
        frames = [dict(phase="sample", elapsed_s=str(second), frame_ms="1", focused="0")
                  for second in range(30, 180)]
        frames += [dict(phase="warmup", elapsed_s="29.9", frame_ms="999", focused="1"),
                   dict(phase="tail", elapsed_s="180", frame_ms="999", focused="1")]
        memory = [dict(elapsed_s=str(second), working_set_bytes="1000", private_bytes="2000")
                  for second in range(30, 180)]
        memory += [dict(elapsed_s="29.9", working_set_bytes="9999", private_bytes="9999"),
                   dict(elapsed_s="180", working_set_bytes="9999", private_bytes="9999")]
        with patch.object(budget, "rows", side_effect=[frames, memory]):
            result = budget.analyze(Path("capture"), warmup=30, sample=150)
        self.assertEqual(result["frames"], 150)
        self.assertEqual(result["p99_ms"], 1)
        self.assertEqual(result["memory_observations"], 150)
        self.assertEqual(result["working_set_bytes"], 1000)
        self.assertEqual(result["private_bytes"], 2000)
        self.assertEqual(result["focus_class"], "fully-unfocused")

    def test_invalid_explicit_window_rejected(self):
        for warmup, sample in ((-1, 150), (30, 0), (30, float("nan"))):
            with self.subTest(warmup=warmup, sample=sample), self.assertRaisesRegex(ValueError, "Invalid observation window"):
                budget.analyze(Path("unused"), warmup, sample)

    def test_absolute_and_relative_are_independent(self):
        gl = dict(p95_ms=5., p99_ms=6., working_set_bytes=1000, private_bytes=2000,
                  focus_class="fully-unfocused")
        modern = dict(gl, p95_ms=5.6)
        comparison = budget.compare(gl, modern)
        self.assertTrue(comparison["conditions"]["p95_absolute"])
        self.assertFalse(comparison["conditions"]["p95_relative"])
        self.assertFalse(comparison["budget_passed_under_verified_conditions"])

    def test_memory_individual_maxima_and_focus_mismatch(self):
        gl = dict(p95_ms=5., p99_ms=6., working_set_bytes=1000, private_bytes=2000,
                  focus_class="fully-unfocused")
        self.assertFalse(budget.compare(gl, dict(gl, private_bytes=2201))["numeric_budget_passed"])
        comparison = budget.compare(gl, dict(gl, focus_class="fully-focused"))
        self.assertTrue(comparison["numeric_budget_passed"])
        self.assertFalse(comparison["budget_passed_under_verified_conditions"])

    def test_memory_window_excludes_startup_and_does_not_fill_missing(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            with (path / "frames.csv").open("w", newline="", encoding="utf-8") as output:
                writer = csv.DictWriter(output, fieldnames=["phase", "elapsed_s", "frame_ms", "focused"])
                writer.writeheader()
                for second in range(10, 60):
                    writer.writerow(dict(phase="sample", elapsed_s=second, frame_ms=1, focused=0))
            memory = [dict(elapsed_s=0, working_set_bytes=9999, private_bytes=9999)]
            memory += [dict(elapsed_s=second, working_set_bytes=1000, private_bytes=2000)
                       for second in range(10, 60)]
            def save():
                with (path / "memory.csv").open("w", newline="", encoding="utf-8") as output:
                    writer = csv.DictWriter(output, fieldnames=memory[0].keys())
                    writer.writeheader()
                    writer.writerows(memory)
            save()
            self.assertEqual(budget.analyze(path)["working_set_bytes"], 1000)
            memory[-1]["private_bytes"] = ""
            save()
            with self.assertRaisesRegex(ValueError, "Missing private_bytes"):
                budget.analyze(path)


if __name__ == "__main__":
    unittest.main()
