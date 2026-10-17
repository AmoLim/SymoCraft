import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("timing", Path(__file__).with_name("verify-vulkan-frame-timings.py"))
timing = importlib.util.module_from_spec(spec)
spec.loader.exec_module(timing)


class TimingTests(unittest.TestCase):
    def native_rows(self):
        native = timing.native_observation
        row = dict(renderer_frame_id="7", observed_frame_id="7", native_model="native_calls_v1",
                   frame_slot="0", present_image="0", submit_before_present="1", submit_after_present="0")
        for call in native.CALLS:
            row.update({call + "_calls":"5", call + "_successes":"5", call + "_native_wall_ms":"2"})
        for name in (*native.WRITES, *(role + "_allocations" for role in native.ROLES)):
            row[name] = "9"
        return [row, dict(row, renderer_frame_id="8", observed_frame_id="8")]

    def test_native_old_model_is_unmeasured_not_zero(self):
        native = timing.native_observation
        self.assertIsNone(native.deltas([{"renderer_frame_id":"7"}]))
        self.assertEqual(native.analyze([{}], [], "unused")["calls"], None)

    def test_native_startup_snapshot_excluded_and_logical_visits_are_not_calls(self):
        rows = self.native_rows()
        rows[1]["descriptor_allocate_visits"] = "100"
        values = timing.native_observation.deltas(rows)
        self.assertIsNone(values["7"][1])
        self.assertEqual(values["8"][1]["pool_create"], (0,0,0.))

    def test_native_count_and_wall_differences(self):
        rows = self.native_rows()
        rows[1].update(queue_present_calls="6", queue_present_successes="6", queue_present_native_wall_ms="12.5")
        self.assertEqual(timing.native_observation.deltas(rows)["8"][1]["queue_present"], (1,1,10.5))

    def test_native_invalid_stale_mixed_or_backward_observations_rejected(self):
        for change in ({"native_model":"unknown"}, {"observed_frame_id":"7"}, {"frame_slot":"3"},
                       {"submit_after_present":"-4"}, {"queue_present_calls":"4"},
                       {"queue_present_successes":"7"}, {"queue_present_native_wall_ms":"nan"}):
            rows = self.native_rows(); rows[1].update(change)
            with self.subTest(change=change), self.assertRaises(ValueError):
                timing.native_observation.deltas(rows)

    def allocation_data(self):
        types = [dict(memory_type="2", property_flags="6", heap_index="1", heap_flags="0"),
                 dict(memory_type="4", property_flags="7", heap_index="0", heap_flags="1")]
        record = dict(role="mesh", memory_type="2", allocations="1", binds="1", releases="1",
                      requested_bytes="28", allocated_bytes="256", live_bytes="0", requirements_bits_or="31", requirements_bits_and="31")
        return types, [record]

    def test_native_actual_nonlocal_type_and_compatible_local_candidate(self):
        types, records = self.allocation_data()
        result = timing.native_observation.allocations(types,records)[0]
        self.assertFalse(result["device_local"])
        self.assertEqual(result["memory_type"],2)
        self.assertEqual(result["device_local_types_compatible_with_all_allocations"],[4])

    def test_native_unknown_incompatible_unbalanced_memory_rejected(self):
        for change in ({"memory_type":"5"}, {"requirements_bits_and":"16"}, {"binds":"0"},
                       {"releases":"0"}, {"live_bytes":"256"}, {"requested_bytes":"512"}):
            types, records = self.allocation_data(); records[0].update(change)
            with self.subTest(change=change), self.assertRaises(ValueError):
                timing.native_observation.allocations(types,records)

    def data(self):
        frame = {key: "0" for key in ("cpu_prepare_ms", "cpu_submit_ms", "present_ms", "instrumentation_ms", "gpu_draw_ms",
                                      "vertices", "draw_calls", "rebuilt_chunks", "resource_upload_bytes", "render_upload_bytes", "upload_calls", "edits", "focused")}
        frame.update(renderer_frame_id="7", elapsed_s="15", frame_ms="30", phase="sample")
        row = dict(renderer_frame_id="7", screenshot="0", completed="1", cpu_valid="1", swapchain_creations="1",
                   buffers_created="2", buffers_released="0", wait_calls="1")
        for phase in timing.PHASES:
            row.update({phase + "_wall_ms": "1", phase + "_cpu_ms": ".5", phase + "_visits": "1"})
        return [frame], [row]

    def analyze(self, frames, rows):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with patch.object(timing.budget, "rows", side_effect=[frames, rows]):
                return timing.analyze(root, root / "timings.csv")

    def test_same_id_join_excludes_non_sample_and_preserves_tail(self):
        frames, rows = self.data()
        frames.append(dict(frames[0], renderer_frame_id="8", elapsed_s="60", phase="complete"))
        result = self.analyze(frames, rows)
        self.assertEqual(result["matched_sample_frames"], 1)
        self.assertEqual(result["over_20ms"]["frames"], 1)
        self.assertEqual(result["top_10_frames"][0]["renderer_frame_id"], 7)
        self.assertEqual(result["top_10_frames"][0]["timed_wall_ms"], len(timing.PHASES))

    def test_missing_cpu_is_not_zero(self):
        frames, rows = self.data()
        rows[0]["cpu_valid"] = "0"
        result = self.analyze(frames, rows)
        self.assertEqual(result["cpu_missing_frames"], 1)
        self.assertIsNone(result["top_10_frames"][0]["timed_cpu_ms"])
        self.assertIsNone(result["phases"]["facade_prepare"]["sum_cpu_ms"])

    def test_legacy_layer_work_is_unknown_not_zero(self):
        frames, rows = self.data()
        result = self.analyze(frames, rows)
        self.assertEqual(result["layer_validation"]["model"], "unmeasured_v1")
        self.assertIsNone(result["layer_validation"]["wall_ms"])
        self.assertIsNone(result["layer_validation"]["scanned_vertices"])

    def cached_data(self):
        frames, rows = self.data()
        frames[0]["draw_calls"] = "2"
        rows[0].update(layer_validation_model="cached_max_v1", layer_vertices="0", layer_scan_work_ms="0",
                       layer_scan_work_visits="0", layer_summary_check_work_ms=".02", layer_summary_check_work_visits="1")
        return frames, rows

    def test_cached_summary_is_nested_and_measured(self):
        frames, rows = self.cached_data()
        result = self.analyze(frames, rows)
        self.assertEqual(result["layer_validation"]["wall_ms"]["mean"], .02)
        self.assertEqual(result["layer_validation"]["visits"], 1)
        self.assertEqual(result["top_10_frames"][0]["timed_wall_ms"], len(timing.PHASES))

    def test_cached_missing_or_stale_scan_rejected(self):
        frames, rows = self.cached_data()
        for change in ({"layer_summary_check_work_visits": "0"}, {"layer_summary_check_work_ms": "nan"},
                       {"layer_scan_work_ms": ".1"}, {"layer_validation_model": "unknown"}):
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.analyze(frames, [dict(rows[0], **change)])
        del rows[0]["layer_validation_model"]
        with self.assertRaises(ValueError):
            self.analyze(frames, rows)

    def test_missing_duplicate_and_screenshot_rejected(self):
        frames, rows = self.data()
        for invalid in ([], rows + rows, [dict(rows[0], screenshot="1")], [dict(rows[0], completed="0")]):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                self.analyze(frames, invalid)

    def test_invalid_or_unvisited_stage_rejected(self):
        frames, rows = self.data()
        for key, value in (("facade_prepare_wall_ms", "nan"), ("facade_prepare_wall_ms", "-1"), ("facade_prepare_visits", "0")):
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                self.analyze(frames, [dict(rows[0], **{key: value})])


if __name__ == "__main__":
    unittest.main()
