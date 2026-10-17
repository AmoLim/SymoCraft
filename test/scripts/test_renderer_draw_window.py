"""Headless regression checks for production drawing/window evidence validation."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("draw_window", ROOT/"test/integration/renderer/verify-draw-window.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


def call(name):
    return name + "(arguments) returns VkResult VK_SUCCESS (0):\n"


def world_trace():
    return ("".join(call(x) for x in ("vkCreateInstance", "vkCreateDevice", "vkCreateWin32SurfaceKHR"))
            + "".join(call("vkCreateSwapchainKHR") + call("vkDestroySwapchainKHR") for _ in range(6))
            + "".join(call(x) for x in probe.QUEUE_CALLS[:3])
            + "BATCH2_MARK_PAUSE_BEGIN\nBATCH2_MARK_PAUSE_END\n"
            + "".join(call(x) for x in ("vkDestroyInstance", "vkDestroyDevice", "vkDestroySurfaceKHR")))


class DrawWindowTests(unittest.TestCase):
    def test_real_trace_and_zero_pause(self):
        result = probe.native_trace(world_trace(), "world-window")
        self.assertEqual(result["calls"]["vkQueueSubmit"], 1)
        self.assertEqual(result["native_suboptimal"], 0)

    def test_no_trace_is_not_native_proof(self):
        with self.assertRaises(ValueError):
            probe.native_trace("BATCH2_MARK_PAUSE_BEGIN\nBATCH2_MARK_PAUSE_END", "world-window")

    def test_any_queue_or_rebuild_during_pause_fails(self):
        for name in probe.QUEUE_CALLS:
            with self.subTest(name=name), self.assertRaises(ValueError):
                probe.native_trace(world_trace().replace("BATCH2_MARK_PAUSE_END", call(name)+"BATCH2_MARK_PAUSE_END"), "world-window")

    def test_recreated_or_unretired_owner_fails(self):
        for name in ("vkCreateDevice", "vkCreateInstance", "vkCreateWin32SurfaceKHR"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                probe.native_trace(world_trace()+call(name), "world-window")

    def test_missing_or_duplicate_markers_fail(self):
        for log in (world_trace().replace("BATCH2_MARK_PAUSE_END", ""), world_trace()+"BATCH2_MARK_PAUSE_BEGIN"):
            with self.assertRaises(ValueError):
                probe.native_trace(log, "world-window")

    def test_invalid_cpu_frame_must_not_reach_any_native_api(self):
        log = ("".join(call(x) for x in probe.QUEUE_CALLS)+"BATCH2_MARK_INVALID_FRAME_BEGIN\nBATCH2_MARK_INVALID_FRAME_END\n"
               + "BATCH2_MARK_PENDING_RESIZE_BEGIN\n"+call("vkGetPhysicalDeviceSurfaceCapabilitiesKHR")+"BATCH2_MARK_PENDING_RESIZE_END")
        probe.native_trace(log, "drawing")
        with self.assertRaises(ValueError):
            probe.native_trace(log.replace("BATCH2_MARK_INVALID_FRAME_END",call("vkGetFenceStatus")+"BATCH2_MARK_INVALID_FRAME_END"), "drawing")
        with self.assertRaises(ValueError):
            probe.native_trace(log.replace("BATCH2_MARK_PENDING_RESIZE_END",call("vkDestroySwapchainKHR")+"BATCH2_MARK_PENDING_RESIZE_END"), "drawing")

    def test_only_exact_observer_notice_is_classified(self):
        clean = "[renderer] Vulkan shutdown: validation_warnings=0; validation_errors=0; native_retired=true\n"
        self.assertEqual(probe.diagnostics(clean, False)["warnings"], 0)
        traced = probe.LAYER_NOTICE+"\n"+clean.replace("warnings=0", "warnings=1")
        self.assertEqual(probe.diagnostics(traced, True)["observer_environment_notices"], 1)
        for errors, enabled in ((traced, False), (traced.replace("warnings=1", "warnings=2"), True),
                                (traced.replace(probe.LAYER_NOTICE, "[renderer] Vulkan validation: resource hazard"), True),
                                (clean.replace("errors=0", "errors=1"), False), ("", True)):
            with self.subTest(errors=errors), self.assertRaises(ValueError):
                probe.diagnostics(errors, enabled)

    def test_native_wsi_codes_remain_separate(self):
        log = world_trace()+"vkQueuePresentKHR(args) returns VkResult VK_ERROR_OUT_OF_DATE_KHR (-1000001004):\n"
        result = probe.native_trace(log, "world-window")
        self.assertEqual(result["native_out_of_date"], 1)
        self.assertEqual(result["native_suboptimal"], 0)


if __name__ == "__main__":
    unittest.main()
