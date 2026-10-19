"""Headless checks that reject incomplete or mislabeled native lifetime evidence."""
import copy
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vulkan_faults", ROOT/"test/integration/renderer/verify-vulkan-lifetime-faults.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


def report(mode):
    retained = mode in probe.RETAINED
    return {"schema": "symocraft.vulkan-production-lifetime-fault.v1", "mode": mode, "success": True,
            "spontaneous_hardware_device_loss_or_real_oom": False, "validation_warnings": 0, "validation_errors": 0,
            "quarantined": retained, "native_destroyed": not retained, "loader_released": not retained,
            "live_bytes": 100 if retained else 0, "buffers_created": 2, "buffers_released": 2,
            "images_created": 1, "images_released": 1}


def errors(mode):
    return "terminal retention" if mode in probe.RETAINED else "native_retired=true"


class NativeEvidenceTests(unittest.TestCase):
    def test_allocation_requires_old_contents_and_complete_retirement(self):
        r = report("allocation"); r.update(old_contents_preserved=True, failed_create_rolled_back=True)
        probe.validate("allocation", r, errors("allocation"), "")
        for field, value in (("old_contents_preserved", False), ("failed_create_rolled_back", False), ("live_bytes", 1), ("buffers_released", 1)):
            bad = {**r, field: value}
            with self.assertRaises(ValueError): probe.validate("allocation", bad, errors("allocation"), "")

    def test_warning_or_error_is_never_clean(self):
        r = report("pipeline")
        for key in ("validation_warnings", "validation_errors"):
            with self.assertRaises(ValueError): probe.validate("pipeline", {**r, key: 1}, errors("pipeline"), "")
        with self.assertRaises(ValueError): probe.validate("pipeline", r, "[renderer] Vulkan validation: bad", "")

    def test_unproven_owner_cannot_claim_cleanup(self):
        r = report("submit"); r.update(first_category=7, first_native_code="-13", failed_rejections=4,
            repeated_shutdown_not_success=True, no_further_submit_or_present=True)
        probe.validate("submit", r, errors("submit"), "")
        for field, value in (("quarantined", False), ("native_destroyed", True), ("loader_released", True), ("live_bytes", 0),
                             ("first_native_code", "0"), ("no_further_submit_or_present", False)):
            with self.assertRaises(ValueError): probe.validate("submit", {**r, field: value}, errors("submit"), "")

    def test_injection_cannot_be_hardware_loss(self):
        with self.assertRaises(ValueError):
            probe.validate("pipeline", {**report("pipeline"), "spontaneous_hardware_device_loss_or_real_oom": True}, errors("pipeline"), "")

    def test_timeout_requires_real_incomplete_fence_and_closed_gate(self):
        r = report("texture-timeout"); r.update(actual_in_flight=True, fence_at_timeout=1, timeline_at_timeout=0,
            owned_work_completed_after_failure=True, isolated_timeline_gate_feature=True,
            first_category=8, first_native_code="2", failed_rejections=4,
            repeated_shutdown_not_success=True, no_further_submit_or_present=True)
        probe.validate("texture-timeout", r, errors("texture-timeout"), "")
        for field, value in (("fence_at_timeout", 0), ("timeline_at_timeout", 1), ("owned_work_completed_after_failure", False)):
            with self.assertRaises(ValueError): probe.validate("texture-timeout", {**r, field: value}, errors("texture-timeout"), "")

    def test_lifetime_requires_all_three_actual_retirements(self):
        r = report("lifetime"); r["capacity_creates"] = 144
        r["events"] = [dict(step=i, fence_before_and_after_mutation=1, timeline_before_and_after_mutation=0,
            old_mesh_retained=True, old_texture_retained=True, old_mesh_retired=True, diagnostic_deferred_present=True,
            deleted_texture_retired=i==2) for i in range(3)]
        probe.validate("lifetime", r, errors("lifetime"), "")
        bad = copy.deepcopy(r); bad["events"][1]["old_mesh_retired"] = False
        with self.assertRaises(ValueError): probe.validate("lifetime", bad, errors("lifetime"), "")
        with self.assertRaises(ValueError): probe.validate("lifetime", {**r, "events": r["events"][:2]}, errors("lifetime"), "")

    def test_game_must_retain_loader_after_real_window_cleanup(self):
        r = report("game-submit"); r.update(game_exit_code=3, entry_cleanup_returned=True, platform_reinitialized=True,
            actual_loader_mapped_after_window_free=True)
        probe.validate("game-submit", r, errors("game-submit")+" shutdown failed", "")
        typed = errors("game-submit") + "\n[renderer] Vulkan Shutdown: terminal retention; category=Timeout; native=; diagnostic="
        probe.validate("game-submit", r, typed, "")
        with self.assertRaises(ValueError): probe.validate("game-submit", r, typed.replace("category=Timeout", "category=InvalidInput"), "")
        with self.assertRaises(ValueError): probe.validate("game-submit", r, typed.replace("Vulkan Shutdown:", "Vulkan Resize:"), "")
        with self.assertRaises(ValueError): probe.validate("game-submit", r, errors("game-submit"), "")
        with self.assertRaises(ValueError): probe.validate("game-submit", {**r, "game_exit_code": 0}, errors("game-submit"), "")

    def test_missing_or_mismatched_report_fails(self):
        for r in ({}, {**report("pipeline"), "mode": "lifetime"}, {**report("pipeline"), "success": False}):
            with self.assertRaises(ValueError): probe.validate("pipeline", r, errors("pipeline"), "")


if __name__ == "__main__":
    unittest.main()
