"""Headless positive/negative checks for native D3D12 retirement evidence."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("d3d12_lifetime", ROOT/"test/integration/renderer/verify-d3d12-lifetime-async.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


def report(mode):
    r = dict(schema="symocraft.d3d12-lifetime.v2", mode=mode, success=True,
             public_facade_and_production_backend=True, device="NVIDIA GeForce RTX 5070 Ti",
             validation_enabled=True, samples=1, identity="CPU fixture identity", driver="fixture",
             info_queue_checks=1, validation_warnings=0, validation_errors=0, validation_corruptions=0,
             native_cleared=True, quarantined=False, live_resource_bytes=0, peak_resource_bytes=100,
             mesh_buffers_created=2, mesh_buffers_released=2, textures_created=1, textures_released=1)
    if mode == "lifetime":
        r["events"] = [dict(step=i, pending_submit_fence=5, completed_before_release=3, gate_before_release=0,
            watchdog_fired=False, allocation_failure_preserved_old_version=True, old_versions_retained=True,
            old_mesh_retired=True, capacity_creates=48, deleted_texture_retired=i==2) for i in range(3)]
    return r




class D3D12EvidenceTests(unittest.TestCase):
    def test_complete_observations_are_accepted(self):
        for mode in probe.MODES: probe.validate(mode, report(mode), "")

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



    def test_diagnostics_and_missing_native_identity_are_rejected(self):
        for key, value in (("validation_warnings", 1), ("validation_errors", 1), ("validation_corruptions", 1),
                           ("info_queue_checks", 0), ("validation_enabled", False), ("device", "fixture"),
                           ("public_facade_and_production_backend", False), ("success", False)):
            with self.assertRaises(ValueError): probe.validate("lifetime", dict(report("lifetime"), **{key:value}), "")


if __name__ == "__main__": unittest.main()
