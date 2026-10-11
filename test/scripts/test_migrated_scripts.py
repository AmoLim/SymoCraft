"""Headless contract/failure regression tests for the 31 migrated entry points."""
import argparse
import contextlib
import copy
import csv
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import verification_common as common
import experimental_build
import platform_verification as platform_probe
from windows_verification import Windows

def load(relative):
    return common.load_script(ROOT / relative)

vendor = load("test/experimental/sdl3/verify-vendor-archive.py")
analysis = load("test/experimental/q06-sampling/analyze-presample.py")
presample = load("test/experimental/q06-sampling/run-presample.py")
baseline = load("test/integration/simulation/CaptureCpuBaseline.py")
incremental = load("test/integration/renderer/CheckRendererIncrementalBuild.py")
publication = load("test/experimental/publication-fix1/collect-runs.py")
offline = load("test/experimental/sdl3/verify-cmake-offline.py")
backend = load("test/integration/renderer/verify-d3d12-backend.py")
game = load("test/integration/renderer/verify-d3d12-game.py")
application = load("test/experimental/platform-sdl3/verify-application.py")
delivery = load("test/integration/verify-sdl-delivery.py")
migration = load("test/integration/VerifyMigrationRuntime.py")

ENTRIES = {
    "experimental/d3d12-r1": ["build", "package", "verify-probes", "contract/build"],
    "experimental/input-comparison": ["run", "verify"],
    "experimental/platform-sdl3": ["build", "run-manual-input", "run-package-input", "verify-allocation", "verify-application", "verify-input", "verify-probes"],
    "experimental/publication-fix1": ["collect-runs"],
    "experimental/q06-sampling": ["analyze-presample", "run-presample"],
    "experimental/sdl3": ["build", "collect-evidence", "collect-input-evidence", "verify-cmake-offline", "verify-probes", "verify-runtime-input", "verify-vendor-archive"],
    "integration": ["CheckIncrementalBuild", "VerifyMigrationRuntime", "verify-sdl-delivery"],
    "integration/renderer": ["CheckRendererIncrementalBuild", "verify-d3d12-backend", "verify-d3d12-game"],
    "integration/simulation": ["CaptureCpuBaseline", "VerifyCpuBaseline"],
}


class MigratedTests(unittest.TestCase):
    def setUp(self):
        (ROOT / "out").mkdir(exist_ok=True)
        temporary = tempfile.TemporaryDirectory(dir=ROOT / "out", prefix="migration-tests-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)

    def file(self, relative, data=b"fixture"):
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def test_all_31_cli_entry_points_help(self):
        count = 0
        for parent, names in ENTRIES.items():
            for name in names:
                path = ROOT / "test" / parent / (name + ".py")
                with self.subTest(path=path):
                    result = subprocess.run([sys.executable, str(path), "--help"], capture_output=True, timeout=10)
                    self.assertEqual(result.returncode, 0, result.stderr.decode("utf-8", "replace"))
                    self.assertIn(b"usage:", result.stdout)
                    count += 1
        self.assertEqual(count, 31)

    def test_script_identity_is_own_file_not_common_helper(self):
        for module in (offline, presample, publication, backend, game, delivery, application):
            self.assertNotEqual(Path(module.__file__).resolve(), (ROOT / "scripts/verification_common.py").resolve())

    def test_archive_identical_different_missing_extra(self):
        self.file("vendor/a.txt", b"a")
        self.file("vendor/b.txt", b"different")
        self.file("vendor/extra.txt")
        archive = self.root / "source.zip"
        with zipfile.ZipFile(archive, "w") as stream:
            for name, data in (("a.txt", b"a"), ("b.txt", b"b"), ("missing.txt", b"missing")):
                stream.writestr("SDL3/" + name, data)
        result = vendor.verify(archive, self.root / "vendor")
        self.assertEqual((result["identical"], result["different"], result["missing"], result["extra"]), (1, 1, 1, ["extra.txt"]))
        self.assertEqual(result["files"][0]["status"], "identical")

    def test_archive_rejects_traversal_multiple_roots_duplicate(self):
        directory = self.root / "vendor"
        directory.mkdir()
        for index, names in enumerate((["SDL/../escape"], ["SDL/file", "OTHER/file2"], ["SDL/file", "SDL/file"], ["SDL/back\\slash"])):
            archive = self.root / f"bad{index}.zip"
            with zipfile.ZipFile(archive, "w") as stream:
                for name in names:
                    info = zipfile.ZipInfo()
                    info.filename = name
                    stream.writestr(info, b"a")
            with self.subTest(names=names), self.assertRaises(ValueError):
                vendor.verify(archive, directory)

    def test_payload_path_size_hash_duplicates_and_links(self):
        path = self.file("package/a.txt")
        root = path.parent
        valid = common.identity(path, root)
        common.verify_payload(root, [valid])
        for entry in ({**valid, "path": "../a.txt"}, {**valid, "bytes": 0}, {**valid, "sha256": "0" * 64}):
            with self.assertRaises((ValueError, OSError)):
                common.verify_payload(root, [entry])
        with self.assertRaises(ValueError):
            common.verify_payload(root, [valid, valid])
        with self.assertRaises(ValueError):
            common.verify_payload(root, [])

    def test_workspace_boundary_uses_path_components(self):
        with self.assertRaises(ValueError):
            common.workspace_path(str(ROOT / "outside"), ROOT / "out")
        with self.assertRaises(ValueError):
            common.workspace_path(str(ROOT / "out"), ROOT / "out")

    def test_json_nonfinite_numbers_fail_instead_of_passing(self):
        path = self.file("report.json", b'{"value":NaN}')
        with self.assertRaises(ValueError):
            common.load_json(path)

    def test_manifest_rejects_windows_stream_and_separator_ambiguity(self):
        for name in ("file:stream", "folder\\file", "C:/file"):
            with self.assertRaises(ValueError):
                common.safe_child(self.root, name)

    def test_process_raw_bytes_unicode_nonzero_timeout(self):
        output = self.root / "\u6d4b\u8bd5 space"
        output.mkdir()
        stdout, stderr = output / "stdout.log", output / "stderr.log"
        result = common.capture(sys.executable, ["-c", "import sys;sys.stdout.buffer.write(b'\\xff\\r\\n');sys.stderr.buffer.write(b'err');sys.exit(7)"], output, stdout, stderr, 10)
        self.assertEqual(result["exit_code"], 7)
        self.assertEqual(stdout.read_bytes(), b"\xff\r\n")
        self.assertEqual(stderr.read_bytes(), b"err")
        result = common.capture(sys.executable, ["-c", "import time;time.sleep(60)"], output, stdout, stderr, .1)
        self.assertTrue(result["timed_out"])
        self.assertIsNotNone(result["exit_code"])

    def test_process_interrupt_cleanup_and_parent_environment(self):
        child = unittest.mock.Mock()
        child.poll.return_value = None
        child.wait.side_effect = [KeyboardInterrupt(), None]
        before = dict(os.environ)
        with patch.object(common.subprocess, "Popen", return_value=child):
            with self.assertRaises(KeyboardInterrupt):
                common.capture("fixture", [], self.root, self.root / "stdout", env=common.clean_environment())
        child.kill.assert_called_once()
        self.assertEqual(dict(os.environ), before)

    def test_timestamp_restored_even_after_failure(self):
        path = self.file("header.h")
        os.utime(path, (1, 1))
        original = path.stat().st_mtime_ns
        with self.assertRaisesRegex(ValueError, "expected"):
            with common.probe_header(path) as record:
                self.assertNotEqual(path.stat().st_mtime_ns, original)
                raise ValueError("expected")
        self.assertEqual(path.stat().st_mtime_ns, original)
        self.assertTrue(record["restored_mtime"])

    def test_timestamp_does_not_restore_concurrent_content_edit(self):
        path = self.file("header.h")
        os.utime(path, (1, 1))
        with self.assertRaisesRegex(ValueError, "concurrently"):
            with common.probe_header(path):
                path.write_bytes(b"user edit")
        self.assertEqual(path.read_bytes(), b"user edit")
        self.assertNotEqual(path.stat().st_mtime, 1)

    def test_cpu_capture_preserves_dirty_bytes_and_comparison_scopes(self):
        root = self.root / "repo"
        for group, paths in baseline.GROUPS.items():
            for name in paths:
                target = root / name
                if target.suffix:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(b"source\r\n")
                else:
                    target.mkdir(parents=True, exist_ok=True)
                    (target / "input.cpp").write_bytes(b"source\r\n")
        subprocess.run(["git", "init", str(root)], capture_output=True, check=True)
        subprocess.run(["git", "-C", str(root), "add", "."], capture_output=True, check=True)
        subprocess.run(["git", "-C", str(root), "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid", "commit", "-m", "fixture"], capture_output=True, check=True)
        generated = root / "test/integration/simulation/__pycache__/fixture.pyc"
        generated.parent.mkdir(parents=True)
        generated.write_bytes(b"interpreter-specific cache")
        args = argparse.Namespace(RepositoryRoot=str(root), OutputDirectory=str(root / "out/first"), CompareDirectory=None, CompareScopes=["production", "dependencies", "assets"])
        result = baseline.capture(args)
        self.assertFalse(any("__pycache__" in entry["path"] or entry["path"].endswith(".pyc") for entry in result["files"]))
        for entry in result["files"]:
            if entry["scope"] != "dependencies":
                self.assertEqual((root / entry["path"]).read_bytes(), (root / "out/first/sources" / entry["path"]).read_bytes())
        args.CompareDirectory, args.OutputDirectory = args.OutputDirectory, str(root / "out/second")
        (root / "game/modules/world/input.cpp").write_bytes(b"user change")
        with self.assertRaises(ValueError):
            baseline.capture(args)
        self.assertFalse(Path(args.OutputDirectory).exists())

    def nine_rounds(self):
        rounds = []
        for scene in ("static", "walk", "edit"):
            for repeat in (1, 2, 3):
                memory = {"max_observed_bytes": 1024 ** 2 * 100, "first_bytes": 1024 ** 2 * 100, "last_bytes": 1024 ** 2 * 90}
                rounds.append({"scene": scene, "repeat": repeat, "passed": True,
                               "metrics": {"frame_count": "100", "frame_p95_ms": str(repeat), "frame_p99_ms": str(repeat + 1), "first_frame_swap_return_from_main_entry_ms": "100", "gpu_count": "90", "gpu_missing_sample_frames": "10"},
                               "memory": {"passed": True, "samples": 30, "working_set_bytes": memory, "private_bytes": memory}})
        common.write_json(self.root / "summary.json", {"passed": True, "actual_game_processes": 9, "passed_count": 9, "rounds": rounds})
        common.write_json(self.root / "manifest.json", {})
        return rounds

    def test_analysis_policy_rounding_no_acceptance(self):
        self.nine_rounds()
        report = analysis.analyze(self.root, "2026-10-09")
        self.assertEqual(report["scenes"][0]["p95_ms"], {"min": 1, "median": 2, "max": 3, "spread": 2})
        self.assertEqual(report["rounds"][0]["ws_growth_mib"], -10)
        self.assertEqual(report["rounds"][0]["gpu_missing_ratio"], .1)
        self.assertEqual(report["options"][1]["first_frame_limit_ms"], 150)
        self.assertEqual(report["options"][1]["ws_observed_max_limit_mib"], 128)
        self.assertFalse(report["budget_approved"])
        self.assertEqual(analysis.analyze(self.root, "2026-10-09", True)["options"], [])

    def test_analysis_rejects_incomplete_bad_date_and_nonfinite(self):
        self.nine_rounds()
        with self.assertRaises(ValueError):
            analysis.analyze(self.root, "2026-02-30")
        for value in ("NaN", "inf", "-1"):
            with self.assertRaises(ValueError):
                analysis.number(value)
        report = common.load_json(self.root / "summary.json")
        report["rounds"].pop()
        common.write_json(self.root / "summary.json", report)
        with self.assertRaises(ValueError):
            analysis.analyze(self.root, "2026-10-09")

    def memory_csv(self, rows):
        path = self.root / "memory.csv"
        with path.open("w", newline="", encoding="utf-8") as stream:
            writer = csv.writer(stream)
            writer.writerow(["elapsed_s", "working_set_bytes", "private_bytes"])
            writer.writerows(rows)
        return path

    def test_memory_statistics_nearest_rank_measured_window(self):
        stats = presample.memory_statistics(self.memory_csv([(i, i + 100, i + 200) for i in range(45)]))
        self.assertTrue(stats["passed"])
        self.assertEqual(stats["samples"], 30)
        self.assertEqual(stats["working_set_bytes"]["p95_bytes"], 138)
        self.assertEqual(stats["working_set_bytes"]["first_bytes"], 110)
        self.assertEqual(stats["working_set_bytes"]["last_bytes"], 139)

    def test_memory_statistics_rejects_gaps_missing_order_and_fraction(self):
        for rows in ([(10, 1, 2)], [(i, 1, 2) for i in range(10, 40) if i not in (20, 21)], [(i, "", 2) for i in range(10, 40)], [(i, .5, 2) for i in range(10, 40)], [(i, 1, 2) for i in reversed(range(10, 40))]):
            self.assertFalse(presample.memory_statistics(self.memory_csv(rows))["passed"])

    def test_cmake_cache_source_config_import_options_guard(self):
        directory = self.root / "build"
        directory.mkdir()
        source = self.root / "source"
        source.mkdir()
        (directory / "CMakeCache.txt").write_text(f"CMAKE_HOME_DIRECTORY:INTERNAL={source}\nCMAKE_BUILD_TYPE:STRING=Debug\nSYMOCRAFT_PLATFORM_PRODUCTION_BUILD:PATH={self.root}\nSYMOCRAFT_PLATFORM_PROBE_VULKAN:BOOL=OFF\n")
        experimental_build.assert_cache(directory, source, "Debug", {"SYMOCRAFT_PLATFORM_PRODUCTION_BUILD": str(self.root), "SYMOCRAFT_PLATFORM_PROBE_VULKAN": "OFF"})
        for config, expected in (("Release", {}), ("Debug", {"SYMOCRAFT_PLATFORM_PROBE_VULKAN": "ON"}), ("Debug", {"SYMOCRAFT_PLATFORM_PRODUCTION_BUILD": str(source)})):
            with self.assertRaises(ValueError):
                experimental_build.assert_cache(directory, source, config, expected)

    def test_cmake_trace_detects_network_not_local_process(self):
        path = self.root / "trace.jsonl"
        entries = [{"version": {"major": 1}}, *[{"cmd": cmd, "args": args, "file": "fixture", "line": i} for i, (cmd, args) in enumerate([
            ("file", ["DOWNLOAD", "https://example.invalid"]), ("FetchContent_Declare", ["lib"]), ("execute_process", ["git", "fetch"]), ("execute_process", ["git", "describe"])])]]
        path.write_text("\n".join(json.dumps(x) for x in entries))
        report = offline.audit_trace(path)
        self.assertEqual(report["executed_commands"], 4)
        self.assertEqual(len(report["network_or_population_commands"]), 3)
        self.assertEqual(len(report["execute_process_calls"]), 2)

    def test_incremental_compilation_owners_and_wrong_app_sources(self):
        log = "Building CXX object game\\app\\CMakeFiles\\SymoCraft.dir\\src\\game_renderer.cpp.obj\nBuilding CXX object game/modules/renderer/CMakeFiles/symocraft_renderer.dir/src/error.cpp.obj\n"
        counts = incremental.compiled_owners(log)
        self.assertEqual(counts["appRuntime"], 1)
        self.assertEqual(counts["coreRenderer"], 1)
        self.assertEqual(incremental.app_sources(log), ["SymoCraft:game_renderer.cpp"])
        with self.assertRaises(ValueError):
            incremental.assert_app(counts, incremental.app_sources(log), ["SymoCraft:application.cpp"])

    def test_incremental_shader_contract_rejects_extra_compilation(self):
        log = "\n".join("Compiling D3D12 " + name + " Shader Model 6.0" for name in ("blocks.vs", "blocks.ps", "selection.vs", "selection.ps"))
        incremental.check_probe("private-d3d12-shader", log, incremental.compiled_owners(log), [], [], [], False)
        with self.assertRaises(ValueError):
            incremental.check_probe("private-d3d12-shader", log + "\nBuilding CXX object unrelated.cpp.obj", incremental.compiled_owners(log), [], [], [], False)

    def test_input_partial_is_not_pass(self):
        record = {"timed_out": False, "exit_code": 2, "report": {"status": "partial", "window_destroyed": True, "sdl_shutdown_completed": True}}
        self.assertEqual(platform_probe.input_valid(record, ""), (False, True))
        self.assertEqual(platform_probe.input_valid(record, "cleanup: failure"), (False, False))

    def test_capability_negative_requires_actual_fixture_cleanup(self):
        case = platform_probe.capability_cases(False, "fixture.dll")[-1]
        self.assertTrue(case[-1])
        record = {"exit_code": 1, "timed_out": False, "report": {"status": "fail", "video_active_after_failure": False, "restricted_fixture_loaded": True,
                  "fixture_failures_injected": 1, "fixture_windows_created": 1, "fixture_windows_destroyed": 1, "fixture_contexts_created": 1, "fixture_contexts_destroyed": 0, "fixture_video_quit_calls": 1}}
        self.assertFalse(platform_probe.capability_valid(record, case[3], case, True))
        record["report"]["fixture_contexts_destroyed"] = 1
        self.assertTrue(platform_probe.capability_valid(record, case[3], case, True))

    def test_allocation_missing_nonempty_snapshot_cannot_pass(self):
        r = {"status": "pass", "failures_injected": 1, "windows_shown": 1, "windows_destroyed": 1, "calibrated_growth_capacity": 100,
             "expected_diagnostic": "Window input event adaptation failed", "capture_rethrow_diagnostic": "Window input event adaptation failed"}
        r.update({key: True for key in ("window_destroyed", "gl_context_released", "external_video_owner_preserved", "sdl_shutdown_completed", "actual_context_observed", "context_gone_before_window_destroy_event", "previous_snapshot_preserved", "capture_rethrows_saved_failure")})
        record = {"timed_out": False, "exit_code": 0, "report": r}
        self.assertFalse(platform_probe.allocation_valid(record, "", "event-growth-wait"))
        r["previous_nonempty_snapshot_observed"] = True
        self.assertTrue(platform_probe.allocation_valid(record, "", "event-growth-wait"))

    def diagnostics(self):
        base = {"kind": "publication_test", "suite": "foundation.files", "run_id": "id", "pid": 1, "tid": 2, "config": "Debug", "source_sha": "source", "exe_sha": "exe", "phase": "write", "expected": "pass", "outcome": "pass", "utc_filetime": 1}
        entries = [{**base, "event": "begin"}, {"kind": "publish_api", "api": "ReplaceFileW", "flags": 0, "win32": 0, "success": True, "pid": 1, "tid": 2, "utc_filetime": 1, "sequence": 1},
                   {**base, "event": "success"}, {**base, "event": "suite_success", "operation_events": 1, "expected_rejections": 0, "expected_standard_rejections": 0}]
        return entries

    def parse_diagnostics(self, entries, enabled=True):
        return publication.read_diagnostics("\n".join(json.dumps(x) for x in entries), "", "foundation.files", "id", 1, enabled, "source", "exe", "Debug")

    def test_publication_same_stream_context_and_counters(self):
        report = self.parse_diagnostics(self.diagnostics())
        self.assertEqual(report["status"], "complete")
        self.assertEqual(report["api_successes"], 1)
        self.assertIsNotNone(report["api_events"][0]["context"])
        entries = self.diagnostics()
        entries[-1]["operation_events"] = 2
        self.assertEqual(self.parse_diagnostics(entries)["status"], "incomplete")

    def test_publication_rejects_boolean_string_code_pid_unassociated(self):
        for code in (True, "5", -1, 2 ** 32, .5):
            self.assertFalse(publication.numeric_code(code))
        self.assertTrue(publication.numeric_code(5))
        for field, value in (("win32", "0"), ("pid", 3), ("tid", 4), ("success", False)):
            entries = self.diagnostics()
            entries[1][field] = value
            self.assertEqual(self.parse_diagnostics(entries)["status"], "incomplete")
        self.assertEqual(self.parse_diagnostics(self.diagnostics(), False)["status"], "incomplete")

    def test_backend_json_roundtrip_identity_and_checks(self):
        report = {"schema": "symocraft.d3d12-backend-smoke.v1", "run_id": "id", "mode": "normal", "expected_device_name": "GPU", "success": True,
                  "json_emitter_probe": {"all_control_characters": "".join(map(chr, range(32))), "quote_backslash": '"\\', "null": None, "unicode_utf8": "\u00e9\u4e2d", "nonfinite_rejected": True}, "checks": [{"passed": True}]}
        self.assertEqual(backend.validate_report(json.loads(json.dumps(report)), "id", "normal", "GPU"), [])
        with self.assertRaises(ValueError):
            backend.validate_report(report, "stale", "normal", "GPU")
        report["checks"][0]["passed"] = False
        self.assertEqual(len(backend.validate_report(report, "id", "normal", "GPU")), 1)

    def test_game_frame_upload_gpu_and_walk_invariants(self):
        row = {"renderer_frame_id": "1", "resource_upload_bytes": "0", "gpu_draw_ms": "1", "gpu_missing_reason": "", "upload_calls": "0", "rebuilt_chunks": "0", "x": "0"}
        game.validate_frames([row], "d3d12", "static")
        for workload, rows in (("walk", [row]), ("edit", [row]), ("static", [row, row]), ("static", [{**row, "gpu_missing_reason": "missing"}])):
            with self.assertRaises(ValueError):
                game.validate_frames(rows, "d3d12", workload)

    def test_application_forced_exit_partial_or_missing_protocol_not_pass(self):
        row = {"timed_out": False, "forced_termination": False, "error": None, "protocol": None, "actual_owned_action": True, "exit_code": 0}
        self.assertTrue(application.application_valid("paused-close", row, "[runtime] shutdown complete;", ""))
        row["forced_termination"] = True
        self.assertFalse(application.application_valid("paused-close", row, "[runtime] shutdown complete;", ""))
        row["forced_termination"] = False
        self.assertFalse(application.application_valid("allow-unfocused", row, "[runtime] shutdown complete;", ""))

    def test_delivery_fault_copy_only_mutates_dedicated_package(self):
        source = self.root / "source"
        self.file("source/assets/texture.png", b"original")
        self.file("source/msvcp140.dll", b"crt")
        before = delivery.package_identity(source)
        destination = self.root / "copy"
        delivery.copy_package(source, destination, True)
        (destination / "assets/texture.png").write_bytes(b"fault")
        self.assertFalse((destination / "msvcp140.dll").exists())
        self.assertEqual(delivery.package_identity(source), before)

    def test_delivery_real_gl_frame_and_shutdown_markers(self):
        stdout = "OpenGL vendor: GPU\nOpenGL renderer: GPU\nOpenGL version: 4.6\n[runtime] ready;\n[runtime] loop finished; rendered_frames=120\n[runtime] shutdown complete; exit_code=0"
        record = {"exit_code": 0, "timed_out": False}
        self.assertTrue(delivery.finite_valid(record, stdout, "", 0, 120)[0])
        self.assertFalse(delivery.finite_valid(record, stdout, "", 0, 121)[0])
        self.assertFalse(delivery.finite_valid(record, stdout, "OpenGL diagnostic", 0, 120)[0])

    def test_windows_adapter_initializes_real_64_bit_signatures(self):
        if os.name != "nt":
            self.skipTest("Windows-only API adapter")
        api = Windows()
        self.assertIsNone(api.find(os.getpid(), "nonexistent fixture title"))
        self.assertIsInstance(api.children(os.getpid()), list)
        with self.assertRaises(ValueError):
            api.owned(None, os.getpid())

    def test_portable_cli_can_import_helpers_outside_repository(self):
        package = self.root / "portable"
        package.mkdir()
        import shutil
        for relative in ("test/experimental/d3d12-r1/verify-probes.py", "test/experimental/platform-sdl3/run-package-input.py", "scripts/verification_common.py", "scripts/scripts_common.py", "scripts/manual_input.py"):
            shutil.copy2(ROOT / relative, package / Path(relative).name)
        for name in ("verify-probes.py", "run-package-input.py"):
            result = subprocess.run([sys.executable, str(package / name), "--help"], cwd=package, capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_migration_rejects_evidence_inside_package_before_copy(self):
        executable = self.file("package/game.exe")
        args = argparse.Namespace(OldExecutable=str(executable), NewExecutable=str(executable), DebugExecutable=str(executable), OutputDirectory=str(executable.parent / "evidence"), IncludeWorldRegression=False)
        with self.assertRaises(ValueError):
            migration.verify(args)
        self.assertFalse((executable.parent / "evidence").exists())


if __name__ == "__main__":
    unittest.main()
