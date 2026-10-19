"""CPU evidence rejection tests; not native Vulkan results."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vulkan_wsi", ROOT/"test/integration/renderer/verify-vulkan-wsi.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


def report(mode):
    r = dict(schema="symocraft.vulkan-wsi.v1", mode=mode, success=True, controlled_injection=True,
             public_facade_and_production_backend=True,
             performance_acceptance=False, native_retired=True, validation_warnings=0, validation_errors=0)
    if mode.startswith("missing-"):
        r.update(extension_rejections=1, feature_rejections=1, device_creations=0,
                 unsupported_before_device=True, error_operation="SelectDevice", error_message="swapchainMaintenance1 required")
    else:
        r.update(device="NVIDIA GeForce RTX 5070 Ti", identity="fixture", driver="fixture", api="fixture",
                 device_creations=1, swapchain_creations=4, sample_ids_complete=True,
                 validation_enabled=True, samples=1, present_mode="Immediate")
        calls = int(mode != "acquire-out-of-date")
        r["events"] = [dict(native_acquire_calls=calls, native_submit_calls=calls, native_present_calls=calls,
            submit_fence_signaled=True, rebuilt_once=True, recovered_pixels_equal=True,
            actual_acquire=0, effective_acquire=-1000001004 if mode == "acquire-out-of-date" else
            1000001003 if mode == "acquire-suboptimal" else 0, actual_present=0,
            effective_present=1000001003 if mode == "present-suboptimal" else
            -1000001004 if mode == "present-out-of-date" else 0) for _ in range(3)]
        if mode == "acquire-out-of-date":
            for event in r["events"]:
                for key in ("actual_acquire", "actual_present", "effective_present"): event[key] = None
    return r


class WsiEvidenceTests(unittest.TestCase):
    def test_complete_controlled_reports(self):
        for mode in probe.MODES: probe.validate(mode, report(mode))

    def test_real_and_injected_scope_not_conflated(self):
        for key,value in (("controlled_injection",False),("performance_acceptance",True),("success",False)):
            r=report("acquire-suboptimal");r[key]=value
            with self.assertRaises(ValueError): probe.validate("acquire-suboptimal",r)

    def test_capability_rejection_before_device(self):
        for key,value in (("device_creations",1),("feature_rejections",0),("error_operation","CreateDevice")):
            r=report("missing-feature");r[key]=value
            with self.assertRaises(ValueError): probe.validate("missing-feature",r)

    def test_acquire_out_of_date_does_not_signal_or_submit(self):
        for key in ("native_acquire_calls","native_submit_calls","native_present_calls"):
            r=report("acquire-out-of-date");r["events"][0][key]=1
            with self.assertRaises(ValueError): probe.validate("acquire-out-of-date",r)

    def test_effective_status_and_recovery_are_required(self):
        for mode in probe.MODES[2:]:
            for key,value in (("submit_fence_signaled",False),("rebuilt_once",False),("recovered_pixels_equal",False),
                              ("effective_acquire",0) if mode.startswith("acquire") else ("effective_present",0)):
                r=report(mode);r["events"][1][key]=value
                with self.assertRaises(ValueError): probe.validate(mode,r)

    def test_uncalled_native_result_is_not_fabricated(self):
        for key in ("actual_acquire", "actual_present", "effective_present"):
            r=report("acquire-out-of-date");r["events"][0][key]=0
            with self.assertRaises(ValueError): probe.validate("acquire-out-of-date",r)

    def test_retirement_diagnostics_and_complete_ids(self):
        for key,value in (("native_retired",False),("validation_errors",1),("validation_warnings",1),
                          ("sample_ids_complete",False),("swapchain_creations",3)):
            r=report("present-suboptimal");r[key]=value
            with self.assertRaises(ValueError): probe.validate("present-suboptimal",r)


if __name__ == "__main__": unittest.main()

