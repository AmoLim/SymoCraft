"""Headless positive/negative checks for native D3D12 retirement evidence."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("d3d12_lifetime", ROOT/"test/integration/renderer/verify-d3d12-lifetime-async.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


def report(mode):
    r = dict(schema="symocraft.d3d12-lifetime-async.v1", mode=mode, success=True,
             public_facade_and_production_backend=True, device="NVIDIA GeForce RTX 5070 Ti",
             validation_enabled=True, samples=1, identity="CPU fixture identity", driver="fixture",
             info_queue_checks=1, validation_warnings=0, validation_errors=0, validation_corruptions=0,
             native_cleared=True, quarantined=False, live_resource_bytes=0, peak_resource_bytes=100,
             mesh_buffers_created=2, mesh_buffers_released=2, textures_created=1, textures_released=1, sample_drops=0)
    if mode == "lifetime":
        r["events"] = [dict(step=i, pending_submit_fence=5, completed_before_release=3, gate_before_release=0,
            watchdog_fired=False, allocation_failure_preserved_old_version=True, old_versions_retained=True,
            old_mesh_retired=True, capacity_creates=48, deleted_texture_retired=i==2) for i in range(3)]
    elif mode == "async-pending":
        r.update(pending_poll_calls=10, poll_added_waits=0, poll_added_query_reads=0, pending_submit_fence=5,
                 completed_before_release=3, resolved_frame_id=9007199254740993, watchdog_fired=False)
    else:
        r.update(submitted_ids=[9007199254740993+3*i for i in range(301)],
                 returned_ids=[9007199254740993+3*i for i in range(45, 301)], sample_drops=45)
    return r


def errors():
    return "\n".join(f"D3D12 GPU sample discarded: frame={9007199254740993+3*i}" for i in range(45))


class D3D12EvidenceTests(unittest.TestCase):
    def test_complete_observations_are_accepted(self):
        for mode in probe.MODES: probe.validate(mode, report(mode), errors() if mode == "async-overflow" else "")

    def test_actual_pending_work_and_watchdog_are_required(self):
        for key, value in (("pending_submit_fence", 3), ("completed_before_release", 5),
                           ("gate_before_release", 1), ("watchdog_fired", True)):
            bad = report("lifetime"); bad["events"][1][key] = value
            with self.assertRaises(ValueError): probe.validate("lifetime", bad, "")

    def test_mutation_capacity_and_retirement_are_independent(self):
        for key, value in (("allocation_failure_preserved_old_version", False), ("old_versions_retained", False),
                           ("old_mesh_retired", False), ("capacity_creates", 47), ("deleted_texture_retired", False)):
            bad = report("lifetime"); bad["events"][2][key] = value
            with self.assertRaises(ValueError): probe.validate("lifetime", bad, "")
        bad = report("lifetime"); bad["events"].pop()
        with self.assertRaises(ValueError): probe.validate("lifetime", bad, "")

    def test_normal_cleanup_cannot_be_quarantine_or_leak(self):
        for key, value in (("native_cleared", False), ("quarantined", True), ("live_resource_bytes", 1),
                           ("mesh_buffers_released", 1), ("textures_released", 0)):
            with self.assertRaises(ValueError): probe.validate("lifetime", dict(report("lifetime"), **{key:value}), "")

    def test_poll_cannot_wait_read_pending_or_round_frame_id(self):
        for key, value in (("poll_added_waits", 1), ("poll_added_query_reads", 1), ("pending_poll_calls", 9),
                           ("resolved_frame_id", float(9007199254740993)), ("watchdog_fired", True)):
            with self.assertRaises(ValueError): probe.validate("async-pending", dict(report("async-pending"), **{key:value}), "")

    def test_overflow_must_explain_every_exact_id(self):
        r = report("async-overflow")
        for field, value in (("returned_ids", r["returned_ids"][:-1]), ("returned_ids", [r["returned_ids"][0]]*256),
                             ("submitted_ids", [float(x) for x in r["submitted_ids"]]), ("sample_drops", 44)):
            with self.assertRaises(ValueError): probe.validate("async-overflow", dict(r, **{field:value}), errors())
        with self.assertRaises(ValueError): probe.validate("async-overflow", r, "")

    def test_diagnostics_and_missing_native_identity_are_rejected(self):
        for key, value in (("validation_warnings", 1), ("validation_errors", 1), ("validation_corruptions", 1),
                           ("info_queue_checks", 0), ("validation_enabled", False), ("device", "fixture"),
                           ("public_facade_and_production_backend", False), ("success", False)):
            with self.assertRaises(ValueError): probe.validate("lifetime", dict(report("lifetime"), **{key:value}), "")


if __name__ == "__main__": unittest.main()
