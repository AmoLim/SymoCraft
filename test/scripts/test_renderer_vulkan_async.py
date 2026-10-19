"""Reject incomplete asynchronous native evidence without requiring a GPU."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vulkan_async", ROOT/"test/integration/renderer/verify-vulkan-async.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


def report(mode):
    terminal = mode == "async-device-error"
    return dict(schema="symocraft.vulkan-production-lifetime-fault.v1", mode=mode, success=True,
                validation_errors=0, validation_warnings=0, quarantined=terminal,
                native_destroyed=not terminal, loader_released=not terminal, live_bytes=100 if terminal else 0,
                buffers_created=2, buffers_released=2, images_created=1, images_released=1)


class AsyncEvidenceTests(unittest.TestCase):
    def test_pending_poll_requires_actual_unready_work_and_no_wait(self):
        r = dict(report("async-gated"), pending_poll_calls=10, pending_fence=1, pending_timeline=0,
                 poll_added_waits=0, actual_in_flight=True, resolved_frame_id=2)
        probe.validate("async-gated", r, "")
        for key, value in (("pending_fence", 0), ("pending_timeline", 1), ("poll_added_waits", 1),
                           ("actual_in_flight", False), ("resolved_frame_id", 1)):
            with self.assertRaises(ValueError): probe.validate("async-gated", dict(r, **{key: value}), "")

    def test_ring_must_explain_every_id_exactly_once(self):
        r = dict(report("async-overflow"), submitted_ids=list(range(2, 303)), returned_ids=list(range(47, 303)))
        errors = "\n".join(f"Vulkan GPU sample discarded: frame={i}" for i in range(2, 47))
        probe.validate("async-overflow", r, errors)
        for ids in (r["returned_ids"][:-1], [47]*256, list(range(46, 302))):
            with self.assertRaises(ValueError): probe.validate("async-overflow", dict(r, returned_ids=ids), errors)
        with self.assertRaises(ValueError): probe.validate("async-overflow", r, "")

    def test_missing_never_has_duration(self):
        for mode, reason in (("async-unsupported", "unsupported"), ("async-discarded", "discarded")):
            r = dict(report(mode), missing_reason=reason, missing_has_duration=False,
                     simulated_unsupported=True, native_query_calls=0)
            probe.validate(mode, r, "")
            with self.assertRaises(ValueError): probe.validate(mode, dict(r, missing_has_duration=True), "")
            with self.assertRaises(ValueError): probe.validate(mode, dict(r, missing_reason="valid"), "")

    def test_capability_injection_must_be_named(self):
        r = dict(report("async-unsupported"), missing_reason="unsupported", missing_has_duration=False,
                 simulated_unsupported=False, native_query_calls=0)
        with self.assertRaises(ValueError): probe.validate("async-unsupported", r, "")

    def test_query_fault_keeps_original_error_and_owner(self):
        r = dict(report("async-device-error"), injected_query_device_lost=True, first_native_code="-4",
                 first_operation="ReadTimestampQuery", no_further_submit_or_present=True,
                 repeated_shutdown_not_success=True)
        probe.validate("async-device-error", r, "")
        for key, value in (("first_native_code", "0"), ("native_destroyed", True), ("loader_released", True),
                           ("live_bytes", 0), ("no_further_submit_or_present", False)):
            with self.assertRaises(ValueError): probe.validate("async-device-error", dict(r, **{key: value}), "")

    def test_diagnostics_or_incomplete_retirement_are_not_clean(self):
        r = dict(report("async-discarded"), missing_reason="discarded", missing_has_duration=False)
        for key in ("validation_errors", "validation_warnings", "live_bytes"):
            with self.assertRaises(ValueError): probe.validate("async-discarded", dict(r, **{key: 1}), "")
        with self.assertRaises(ValueError): probe.validate("async-discarded", dict(r, buffers_released=1), "")


if __name__ == "__main__": unittest.main()
