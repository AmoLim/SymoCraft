import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("timing", Path(__file__).with_name("verify-d3d12-frame-timings.py"))
timing = importlib.util.module_from_spec(spec)
spec.loader.exec_module(timing)


class TimingTests(unittest.TestCase):
    def data(self):
        frame = {key: "0" for key in ("cpu_prepare_ms", "cpu_submit_ms", "present_ms", "mesh_ms", "pack_ms",
                                      "upload_cpu_ms", "instrumentation_ms", "gpu_draw_ms", "edits", "rebuilt_chunks")}
        frame.update(renderer_frame_id="7", elapsed_s="15", frame_ms="30", phase="sample", vertices="124", upload_calls="1")
        row = dict(renderer_frame_id="7", screenshot="0", completed="1", cpu_valid="1", layer_vertices="100",
                   mesh_buffers_created="2", mesh_buffers_released="0", wait_calls="1")
        for phase in timing.PHASES:
            row.update({phase + "_wall_ms": "1", phase + "_cpu_ms": ".5", phase + "_visits": "1"})
        for work in timing.WORK:
            row.update({work + "_work_ms": ".1", work + "_work_visits": "1"})
        row["mesh_create_total_work_visits"] = "0"
        return [frame], [row]

    def analyze(self, frames, rows):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with patch.object(timing.budget, "rows", side_effect=[frames, rows]):
                return timing.analyze(root, root / "timings.csv")

    def test_same_id_nested_cost_is_not_added_twice(self):
        frames, rows = self.data()
        result = self.analyze(frames, rows)
        self.assertEqual(result["top_10_frames"][0]["exclusive_render_wall_ms"], len(timing.PHASES))
        self.assertEqual(result["sample_update_work_calls"], 1)
        self.assertEqual(result["statistics"]["non_edit"]["frames"], 1)
        self.assertIsNone(result["statistics"]["edit"]["frame_ms"])

    def test_missing_cpu_is_not_zero(self):
        frames, rows = self.data()
        rows[0]["cpu_valid"] = "0"
        result = self.analyze(frames, rows)
        self.assertEqual(result["cpu_missing_frames"], 1)
        self.assertIsNone(result["top_10_frames"][0]["phases_cpu_ms"])

    def srv_rows(self):
        return [dict(renderer_frame_id=str(i + 1), srv_binding_model="frame_slot_delta_v1",
                     srv_heaps_created=str(min(i + 1, 3)), srv_views_written=str(min(i + 1, 3) * 361),
                     srv_slot=str(i % 3), srv_capacity="512") for i in range(6)]

    def test_srv_warmup_is_not_sample_work(self):
        result = timing.srv_operations(self.srv_rows(), {"4", "5", "6"})
        self.assertEqual(result["all_recorded"], {"heap_creations": 3, "view_writes": 1083})
        self.assertEqual(result["sample"], {"heap_creations": 0, "view_writes": 0})
        self.assertEqual(result["sample_slots"], [0, 1, 2])

    def test_srv_changed_binding_is_counted(self):
        rows = self.srv_rows()
        for row in rows[3:]:
            row["srv_views_written"] = "1084"
        self.assertEqual(timing.srv_operations(rows, {"4", "5", "6"})["sample"]["view_writes"], 1)

    def test_srv_legacy_is_unmeasured_not_zero(self):
        result = timing.srv_operations([{"renderer_frame_id": "1"}], {"1"})
        self.assertIsNone(result["sample"])
        self.assertIsNone(result["all_recorded"])

    def test_srv_corrupt_or_mixed_counter_rejected(self):
        for change in ({"srv_heaps_created": "0"}, {"srv_slot": "3"}, {"srv_capacity": "0"},
                       {"srv_binding_model": "unmeasured_v1"}, {"srv_views_written": "nan"}):
            rows = self.srv_rows()
            rows[-1].update(change)
            with self.subTest(change=change), self.assertRaises(ValueError):
                timing.srv_operations(rows, {"4", "5", "6"})

    def cached_data(self):
        frames, rows = self.data()
        frames[0]["draw_calls"] = "2"
        rows[0].update(layer_validation_model="cached_max_v1", layer_vertices="0", layer_scan_work_ms="0",
                       layer_scan_work_visits="0", layer_summary_check_work_ms=".02", layer_summary_check_work_visits="1")
        return frames, rows

    def test_cached_summary_uses_draws_not_vertices(self):
        frames, rows = self.cached_data()
        result = self.analyze(frames, rows)
        self.assertEqual(result["layer_validation"]["model"], "cached_max_v1")
        self.assertEqual(result["layer_validation"]["scanned_vertices"], 0)
        self.assertEqual(result["layer_validation"]["wall_ms"]["mean"], .02)
        self.assertEqual(result["top_10_frames"][0]["exclusive_render_wall_ms"], len(timing.PHASES))

    def test_cached_missing_mixed_or_stale_scan_rejected(self):
        frames, rows = self.cached_data()
        for change in ({"layer_summary_check_work_visits": "0"}, {"layer_summary_check_work_ms": "nan"},
                       {"layer_scan_work_visits": "1"}, {"layer_vertices": "100"}, {"layer_validation_model": "unknown"}):
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.analyze(frames, [dict(rows[0], **change)])
        del rows[0]["layer_validation_model"]
        with self.assertRaises(ValueError):
            self.analyze(frames, rows)

    def test_wrong_upload_or_vertex_mapping_rejected(self):
        frames, rows = self.data()
        for change in ({"layer_vertices": "99"}, {"mesh_update_total_work_visits": "2"}, {"layer_scan_work_visits": "0"}):
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.analyze(frames, [dict(rows[0], **change)])

    def test_duplicate_missing_invalid_incomplete_rejected(self):
        frames, rows = self.data()
        for invalid in ([], rows + rows, [dict(rows[0], completed="0")], [dict(rows[0], screenshot="1")],
                        [dict(rows[0], layer_scan_work_ms="nan")], [dict(rows[0], facade_prepare_visits="0")]):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                self.analyze(frames, invalid)


if __name__ == "__main__":
    unittest.main()
