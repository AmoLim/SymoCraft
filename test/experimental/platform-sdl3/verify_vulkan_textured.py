"""Bounded Vulkan E01-E03 experiments with independent standard-library checks."""

import argparse
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import zlib

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "scripts"))
from build import developer_environment
from scripts_common import hidden_process, sha256
from verification_common import file_version, identity, reject_links, require

WIDTH, HEIGHT = 640, 480
TEXELS = (
    (255, 32, 64, 255), (16, 240, 80, 255), (48, 96, 224, 255), (192, 128, 24, 255),
    (24, 160, 208, 255), (232, 48, 176, 255), (112, 224, 40, 255), (64, 80, 144, 255),
)
POINTS = ((146, 170), (226, 170), (146, 338), (226, 338),
          (466, 170), (546, 170), (466, 338), (546, 338))
GUARDS = ((100, 254), (271, 254), (186, 80), (186, 428),
          (420, 254), (591, 254), (506, 80), (506, 428), (346, 254))
CLEAR_RGBA = (8, 16, 24, 255)
MATRIX = (.25, 0, 0, 0, 0, .35, 0, 0, 0, 0, 1, 0, .08, -.06, .4, 1)
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def png_chunk(kind, payload):
    return (struct.pack(">I", len(payload)) + kind + payload +
            struct.pack(">I", zlib.crc32(kind + payload) & 0xffffffff))


def encode_png(width, height, pixels):
    require(len(pixels) == width * height * 4, "RGBA byte count does not match extent")
    scanlines = b"".join(b"\0" + pixels[y * width * 4:(y + 1) * width * 4]
                         for y in range(height))
    return (PNG_SIGNATURE + png_chunk(b"IHDR", struct.pack(">2I5B", width, height, 8, 6, 0, 0, 0)) +
            png_chunk(b"IDAT", zlib.compress(scanlines)) + png_chunk(b"IEND", b""))


def decode_png(data):
    """Decode noninterlaced RGBA8 screenshots without changing their pixels."""
    require(data.startswith(PNG_SIGNATURE), "Invalid PNG signature")
    offset, header, packed, ended = 8, None, bytearray(), False
    while offset < len(data):
        require(offset + 12 <= len(data), "Truncated PNG chunk")
        length, = struct.unpack_from(">I", data, offset)
        kind = data[offset + 4:offset + 8]
        end = offset + 12 + length
        require(end <= len(data), "Truncated PNG payload")
        payload = data[offset + 8:offset + 8 + length]
        checksum, = struct.unpack_from(">I", data, offset + 8 + length)
        require(checksum == zlib.crc32(kind + payload) & 0xffffffff, "PNG chunk CRC mismatch")
        if kind == b"IHDR":
            require(header is None and offset == 8 and length == 13, "Invalid PNG header order")
            header = struct.unpack(">2I5B", payload)
            require(0 < header[0] <= 4096 and 0 < header[1] <= 4096 and
                    header[2:] == (8, 6, 0, 0, 0), "Expected bounded noninterlaced RGBA8 PNG")
        elif kind == b"IDAT":
            require(header is not None and not ended, "Invalid PNG data order")
            packed.extend(payload)
        elif kind == b"IEND":
            require(length == 0 and end == len(data), "Invalid PNG end")
            ended = True
        elif not kind[0] & 32:
            raise ValueError("Unsupported critical PNG chunk")
        offset = end
    require(header is not None and ended, "PNG is incomplete")
    width, height = header[:2]
    stride = width * 4
    raw = zlib.decompress(packed)
    require(len(raw) == height * (stride + 1), "Unexpected decompressed PNG byte count")
    pixels, previous = bytearray(), bytearray(stride)
    for y in range(height):
        start = y * (stride + 1)
        mode, row = raw[start], bytearray(raw[start + 1:start + 1 + stride])
        require(mode <= 4, "Invalid PNG row filter")
        for x in range(stride):
            left = row[x - 4] if x >= 4 else 0
            up = previous[x]
            corner = previous[x - 4] if x >= 4 else 0
            if mode == 1:
                predictor = left
            elif mode == 2:
                predictor = up
            elif mode == 3:
                predictor = (left + up) // 2
            elif mode == 4:
                estimate = left + up - corner
                distances = (abs(estimate - left), abs(estimate - up), abs(estimate - corner))
                predictor = (left, up, corner)[distances.index(min(distances))]
            else:
                predictor = 0
            row[x] = (row[x] + predictor) & 255
        pixels.extend(row)
        previous = row
    return width, height, bytes(pixels)


def srgb_decode_byte(value):
    normalized = value / 255.0
    linear = normalized / 12.92 if normalized <= .04045 else ((normalized + .055) / 1.055) ** 2.4
    return int(math.floor(linear * 255 + .5))


def pixel_at(pixels, width, x, y):
    offset = (y * width + x) * 4
    return tuple(pixels[offset:offset + 4])


def check_pixels(pixels, width, height, space, swap_palette=False):
    require((width, height) == (WIDTH, HEIGHT), "E01 fixture requires actual 640x480 drawable pixels")
    require(len(pixels) == width * height * 4, "Raw top-left RGBA8 file has wrong byte count")
    require(space in ("Linear", "SRGB"), "Unrecognized texture color space")
    expected = TEXELS if space == "Linear" else tuple(tuple(srgb_decode_byte(c) for c in t[:3]) + (255,)
                                                    for t in TEXELS)
    if swap_palette:
        expected = expected[4:] + expected[:4]
    samples = []
    for index, ((x, y), wanted) in enumerate(zip(POINTS, expected)):
        observed = pixel_at(pixels, width, x, y)
        checks = [pixel_at(pixels, width, x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1)]
        passed = all(all(abs(pixel[channel] - wanted[channel]) <= 2 for channel in range(3)) and
                     pixel[3] == wanted[3] for pixel in checks)
        samples.append({"layer": index // 4, "texel": [index % 2, (index % 4) // 2],
                        "xy": [x, y], "observed": observed, "expected": wanted,
                        "rgb_tolerance": 2, "alpha_exact": True, "patch_3x3_passed": passed})
    guards = []
    for x, y in GUARDS:
        patch = [pixel_at(pixels, width, x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1)]
        passed = all(all(abs(pixel[channel] - CLEAR_RGBA[channel]) <= 2 for channel in range(3)) and
                     pixel[3] == CLEAR_RGBA[3] for pixel in patch)
        guards.append({"xy": [x, y], "observed": pixel_at(pixels, width, x, y), "expected": CLEAR_RGBA,
                       "patch_3x3_passed": passed, "distance_from_expected_edge_at_least_4px": True})
    return {"space": space, "swap_palette": swap_palette, "samples": samples, "geometry_guards": guards,
            "passed": all(s["patch_3x3_passed"] for s in samples + guards),
            "oracle": "independent asymmetric 2x2x2 fixture; SRGB decode to UNORM attachment",
            "limits": "Safe interior samples, not full-frame pixel equivalence, alpha-depth, or mixed-layer proof"}


def self_test():
    small = bytes(range(32))
    require(decode_png(encode_png(4, 2, small)) == (4, 2, small), "PNG roundtrip failed")
    damaged = bytearray(encode_png(4, 2, small))
    damaged[-1] ^= 1
    try:
        decode_png(damaged)
    except ValueError:
        pass
    else:
        raise ValueError("PNG CRC corruption was accepted")
    require(tuple(srgb_decode_byte(c) for c in (192, 128, 24)) == (134, 55, 2), "sRGB oracle failed")
    for space in ("Linear", "SRGB"):
        pixels = bytearray(bytes(CLEAR_RGBA) * (WIDTH * HEIGHT))
        for point, texel in zip(POINTS, TEXELS):
            color = texel if space == "Linear" else tuple(srgb_decode_byte(c) for c in texel[:3]) + (255,)
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    offset = ((point[1] + dy) * WIDTH + point[0] + dx) * 4
                    pixels[offset:offset + 4] = bytes(color)
        require(check_pixels(pixels, WIDTH, HEIGHT, space)["passed"], "Positive pixel oracle failed")
        offset = (POINTS[0][1] * WIDTH + POINTS[0][0]) * 4
        pixels[offset:offset + 4] = bytes(TEXELS[4])
        require(not check_pixels(pixels, WIDTH, HEIGHT, space)["passed"], "Swapped layer was accepted")
        pixels[offset:offset + 4] = bytes(TEXELS[0] if space == "Linear" else
                                          tuple(srgb_decode_byte(c) for c in TEXELS[0][:3]) + (255,))
        require(check_pixels(pixels, WIDTH, HEIGHT, space)["passed"], "Restored texture oracle failed")
        offset = (GUARDS[0][1] * WIDTH + GUARDS[0][0]) * 4
        pixels[offset:offset + 4] = bytes(TEXELS[0])
        require(not check_pixels(pixels, WIDTH, HEIGHT, space)["passed"], "Wrong geometry extent was accepted")
    uniform = {"type": "matrix", "set": 0, "binding": 0, "block_size": 64}
    types = {"matrix": {"members": [{"offset": 0, "matrix_stride": 16, "row_major": True}]}}
    image = [{"set": 0, "binding": 1, "type": "texture2DArray"}]
    sampler = [{"set": 0, "binding": 2}]
    vertex = {"entryPoints": [{"name": "VSMain", "mode": "vert"}], "types": types, "ubos": [uniform],
              "inputs": [{"location": 0, "type": "ivec3", "name": "in.var.POSITION"},
                         {"location": 1, "type": "vec3", "name": "in.var.TEXCOORD0"}]}
    fragment = {"entryPoints": [{"name": "PSMain", "mode": "frag"}],
                "separate_images": image, "separate_samplers": sampler}
    for configuration in ("Release", "Debug"):
        vs, ps = json.loads(json.dumps(vertex)), json.loads(json.dumps(fragment))
        if configuration == "Debug":
            vs["inputs"].append({"location": 2, "type": "float", "name": "in.var.DIRECTION"})
            vs["separate_images"], vs["separate_samplers"] = image, sampler
            ps["types"], ps["ubos"] = types, [uniform]
        require(verify_reflection(vs, ps, configuration)["passed"], "Positive reflection contract failed")
        bad = json.loads(json.dumps(vs))
        bad["inputs"][-1]["type"] = "vec2"
        try:
            verify_reflection(bad, ps, configuration)
        except ValueError:
            pass
        else:
            raise ValueError("Wrong reflected attribute type was accepted")
    bad_fragment = json.loads(json.dumps(fragment))
    bad_fragment["separate_images"][0]["binding"] = 0
    try:
        verify_reflection(vertex, bad_fragment, "Release")
    except ValueError:
        pass
    else:
        raise ValueError("Conflicting descriptor binding was accepted")
    lifetime_self_test()
    resize_self_test()
    print("PNG/CRC, color/geometry/reflection, E02 lifetime/empty, and E03 resize/pause oracles passed")


def verify_reflection(vertex, fragment, configuration):
    require(configuration in ("Debug", "Release"), "Unknown shader build configuration")
    require(vertex["entryPoints"] == [{"name": "VSMain", "mode": "vert"}] and
            fragment["entryPoints"] == [{"name": "PSMain", "mode": "frag"}], "Wrong shader entry point")
    expected_inputs = {(0, "ivec3"), (1, "vec3")}
    if configuration == "Debug":
        expected_inputs.add((2, "float"))
        require(any(i["location"] == 2 and i["name"] == "in.var.DIRECTION" for i in vertex["inputs"]),
                "Unoptimized location2 must remain the same-source float DIRECTION")
    require(len(vertex["inputs"]) == len(expected_inputs) and
            {(item["location"], item["type"]) for item in vertex["inputs"]} == expected_inputs,
            "Unexpected configuration-specific vertex ABI locations/types")

    def check_uniform(stage, expected_count):
        uniforms = stage.get("ubos", [])
        require(len(uniforms) == expected_count, "Unexpected configuration-specific matrix UBO declaration count")
        for uniform in uniforms:
            require((uniform["set"], uniform["binding"], uniform["block_size"]) == (0, 0, 64),
                    "Wrong matrix UBO binding/size")
            members = stage["types"][uniform["type"]]["members"]
            require(len(members) == 1 and members[0]["offset"] == 0 and members[0]["matrix_stride"] == 16 and
                    members[0].get("row_major") is True, "Unexpected DXC matrix layout decorations")

    check_uniform(vertex, 1)
    check_uniform(fragment, 1 if configuration == "Debug" else 0)
    for stage, retained in ((fragment, True), (vertex, configuration == "Debug")):
        images = [(i["set"], i["binding"], i["type"]) for i in stage.get("separate_images", [])]
        samplers = [(i["set"], i["binding"]) for i in stage.get("separate_samplers", [])]
        require(images == ([(0, 1, "texture2DArray")] if retained else []), "Wrong texture array descriptor")
        require(samplers == ([(0, 2)] if retained else []), "Wrong sampler descriptor")
    return {"passed": True, "configuration": configuration,
            "constant_bytes": 64, "matrix_stride": 16, "spirv_row_major": True,
            "position_location_type": "0/ivec3", "texture_location_type": "1/vec3",
            "direction_shader_input": configuration == "Debug", "direction_location_type": "2/float" if configuration == "Debug" else None,
            "unoptimized_same_source_declarations_retained": configuration == "Debug",
            "set0_bindings": {"constant": 0, "texture_array": 1, "sampler": 2},
            "runtime_layout_correctness_not_implied": True}


def verify_native(native, capture, configuration):
    require(native["extent"] == {"width": WIDTH, "height": HEIGHT}, "Wrong native drawable extent")
    device = native["device"]
    vendor = int(device["vendor_id"], 0) if isinstance(device["vendor_id"], str) else device["vendor_id"]
    require(vendor == 0x10de and "5070 Ti" in device["device_name"], "E01 did not use the required native RTX5070Ti")
    require(device["device_id"] is not None and device["driver_version"] is not None and
            isinstance(device["queue_family"], int) and device["queue_family"] >= 0, "Missing native device/queue identity")
    api = device["api_version"]
    version = tuple(int(part) for part in api.split(".")) if isinstance(api, str) else (api >> 22, (api >> 12) & 1023)
    require(version >= (1, 2), "Actual Vulkan device API is below the approved 1.2 baseline")
    require(native["samples"] == 1 and native["present_mode"] == "Fifo", "Unexpected E01 samples/present mode")
    require(native["swapchain_format"] in ("B8G8R8A8_UNORM", "R8G8B8A8_UNORM") and
            native["swapchain_color_space"] == "SRGB_NONLINEAR", "Fixture oracle requires a UNORM render attachment")
    require(tuple(native["clear_rgba"]) == CLEAR_RGBA, "Unexpected clear-color fixture")
    layout = native["layout"]
    for field, expected in {"vertex_stride": 28, "position_offset": 0, "uv_layer_offset": 12,
                            "direction_offset": 24, "descriptor_set": 0, "constant_binding": 0,
                            "texture_binding": 1, "sampler_binding": 2, "viewport": "negative-height-only"}.items():
        require(layout[field] == expected, "Native layout does not match the independently reflected fixture: " + field)
    require(layout["direction_shader_input"] is (configuration == "Debug"),
            "Native location2 declaration does not match the reflected Debug/Release shader configuration")
    require(len(layout["matrix"]) == 16 and all(math.isfinite(actual) and abs(actual - expected) < 1e-6
                                              for actual, expected in zip(layout["matrix"], MATRIX)), "Wrong fixture matrix upload")
    images = native["images"]
    require(len(images) == 2 and {image["space"] for image in images} == {"Linear", "SRGB"}, "Need both color-space captures")
    checks = []
    for image in images:
        space = image["space"]
        require(image["file"] == space.lower() + ".rgba" and image["format"] == "RGBA8" and
                image["origin"] == "top-left" and (image["width"], image["height"]) == (WIDTH, HEIGHT),
                "Invalid raw readback file identity/extent/origin")
        path = capture / image["file"]
        reject_links(path)
        raw = path.read_bytes()
        pixels = check_pixels(raw, WIDTH, HEIGHT, space)
        png = capture / (space.lower() + ".png")
        with png.open("xb") as destination:
            destination.write(encode_png(WIDTH, HEIGHT, raw))
        require(decode_png(png.read_bytes()) == (WIDTH, HEIGHT, raw), "PNG conversion changed native RGBA pixels")
        pixels["raw"] = identity(path)
        pixels["png"] = identity(png)
        pixels["readback_and_present_completed"] = all(image[key] is True for key in
                                                       ("submitted", "retired", "present_fence_completed"))
        pixels["passed"] = pixels["passed"] and pixels["readback_and_present_completed"]
        checks.append(pixels)
    validation = native["validation"]
    validation_passed = (validation["enabled"] is True and validation["errors"] == 0 and
                         validation["warnings"] == 0 and native["cleanup_completed"] is True)
    return {"passed": all(image["passed"] for image in checks) and validation_passed,
            "images": checks, "validation_and_cleanup_passed": validation_passed,
            "independent_pixel_oracle": True, "native_sample_values_not_used_as_oracle": True,
            "limits": ["Not production Vulkan integration", "Not E02/E03 or full A06/A09 acceptance",
                       "No alpha-depth, mixed-layer or complete cull/winding matrix verification"]}


def preserve_failure_images(capture):
    """Keep usable raw readbacks visible without turning a native failure into acceptance."""
    diagnostics = {"acceptance_passed": False, "diagnostic_only": True,
                   "expected_protocol_extent": [WIDTH, HEIGHT], "images": []}
    for raw_path in sorted(capture.glob("*.rgba")):
        space = "SRGB" if raw_path.stem == "srgb" else "Linear"
        item = {"case": raw_path.stem, "space": space, "acceptance_passed": False}
        diagnostics["images"].append(item)
        try:
            reject_links(raw_path)
            item["raw"] = identity(raw_path)
            raw = raw_path.read_bytes()
            width, height = WIDTH, HEIGHT
            metadata_path = raw_path.with_suffix(".json")
            if metadata_path.exists():
                reject_links(metadata_path)
                metadata = json.loads(metadata_path.read_text(encoding="utf-8-sig"))
                require(metadata["schema"] == "symocraft.vulkan.capture.v1" and metadata["format"] == "RGBA8" and
                        metadata["origin"] == "top-left", "Wrong failed capture metadata")
                width, height = metadata["width"], metadata["height"]
                require(0 < width <= 4096 and 0 < height <= 4096, "Invalid failed capture extent")
                item["metadata"] = identity(metadata_path)
            require(len(raw) == width * height * 4, "Failed raw capture byte count does not match actual extent")
            png = raw_path.with_suffix(".png")
            reject_links(png)
            if not png.exists():
                with png.open("xb") as destination:
                    destination.write(encode_png(width, height, raw))
            require(decode_png(png.read_bytes()) == (width, height, raw), "Failure PNG differs from raw readback")
            item["png"] = identity(png)
            if raw_path.stem in ("initial", "larger", "wide", "narrow", "restored", "final"):
                pixels = check_resize_pixels(raw, width, height)
            else:
                pixels = check_empty_pixels(raw) if raw_path.stem == "cleared-before-restore" else check_pixels(
                    raw, width, height, space, raw_path.stem in ("updated-before-clear", "recreated-before-destroy"))
            item["pixel_oracle_passed"] = pixels.pop("passed")
            item["pixel_diagnostics"] = pixels
        except Exception as error:
            item["diagnostic_failure"] = str(error)
    return diagnostics


def check_empty_pixels(raw):
    require(len(raw) == WIDTH * HEIGHT * 4, "Empty-frame readback has wrong extent/byte count")
    return {"passed": raw == bytes(CLEAR_RGBA) * (WIDTH * HEIGHT),
            "all_pixels_checked": WIDTH * HEIGHT, "expected_rgba": CLEAR_RGBA}


def check_lifetime_frame(frame, case, index):
    name, vertices, _, mesh_released, texture_released, rejected = case
    require(frame["name"] == name and frame["file"] == name + ".rgba" and frame["vertices"] == vertices,
            "Wrong E02 frame order/input/readback identity")
    require(frame["fence_before"] == 1 and frame["fence_after_mutation"] == 1 and frame["fence_completed"] == 0 and
            frame["gate_before"] == 0 and frame["gate_after_mutation"] == 0,
            "E02 mutation was not bracketed by actual pending native fence/timeline observations")
    require(frame["mesh_released"] == mesh_released and frame["texture_released"] == texture_released and
            frame["stale_rejections"] == rejected, "E02 logical/native retirement order mismatch")
    require(frame["retained_mesh"] is True and frame["retained_texture"] is True and
            frame["retired_mesh"] is True and frame["retired_texture"] is (index >= 3) and
            frame["distinct_update_buffer"] is (index == 0) and frame["present_fence_completed"] is True,
            "E02 frame did not retain/retire the expected immutable native versions")


def lifetime_self_test():
    empty = bytes(CLEAR_RGBA) * (WIDTH * HEIGHT)
    require(check_empty_pixels(empty)["passed"], "Empty-frame positive oracle failed")
    require(not check_empty_pixels(empty[:-4] + bytes(TEXELS[0]))["passed"], "Single residual pixel accepted")
    swapped = bytearray(empty)
    for (x, y), color in zip(POINTS, TEXELS[4:] + TEXELS[:4]):
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                offset = ((y + dy) * WIDTH + x + dx) * 4
                swapped[offset:offset + 4] = bytes(color)
    require(check_pixels(swapped, WIDTH, HEIGHT, "Linear", True)["passed"] and
            not check_pixels(swapped, WIDTH, HEIGHT, "Linear")["passed"], "Updated layer oracle not independent")
    case = ("created-before-update", 12, False, [0, 0, 1], [0, 0, 0], [0, 0])
    frame = {"name": case[0], "file": case[0] + ".rgba", "vertices": 12, "fence_before": 1,
             "fence_after_mutation": 1, "fence_completed": 0, "gate_before": 0, "gate_after_mutation": 0,
             "mesh_released": [0, 0, 1], "texture_released": [0, 0, 0], "stale_rejections": [0, 0],
             "retained_mesh": True, "retained_texture": True, "retired_mesh": True, "retired_texture": False,
             "distinct_update_buffer": True, "present_fence_completed": True}
    check_lifetime_frame(frame, case, 0)
    for key, value in (("fence_before", 0), ("fence_after_mutation", 0), ("fence_completed", 1),
                       ("gate_after_mutation", 1), ("mesh_released", [0, 1, 1]),
                       ("texture_released", [0, 1, 1]), ("retained_mesh", False),
                       ("retired_mesh", False), ("distinct_update_buffer", False), ("present_fence_completed", False)):
        wrong = {**frame, key: value}
        try:
            check_lifetime_frame(wrong, case, 0)
        except ValueError:
            pass
        else:
            raise ValueError("Incorrect E02 lifetime proof accepted: " + key)


def verify_lifetime(native, capture, configuration):
    require(native["schema"] == "symocraft.vulkan-r1-e02.native.v1", "Wrong E02 native schema")
    require(native["extent"] == {"width": WIDTH, "height": HEIGHT} and native["samples"] == 1 and
            native["present_mode"] == "Fifo", "Wrong E02 native extent/sampling/present mode")
    device = native["device"]
    require(device["vendor_id"] == 0x10de and device["device_id"] == 0x2c05 and
            "RTX 5070 Ti" in device["device_name"] and device["api_version"] >= (1 << 22 | 2 << 12) and
            device["driver_version"] and isinstance(device["queue_family"], int), "Wrong E02 actual adapter/API identity")
    require(native["swapchain_format"] in ("B8G8R8A8_UNORM", "R8G8B8A8_UNORM") and
            native["swapchain_color_space"] == "SRGB_NONLINEAR", "Wrong E02 attachment color space")
    require(native["direction_shader_input"] is (configuration == "Debug"), "Wrong E02 shader ABI configuration")
    validation = native["validation"]
    require(validation == {"enabled": True, "synchronization_enabled": True, "warnings": 0, "errors": 0},
            "E02 requires enabled synchronization validation with no warnings/errors")
    require(native["controlled_native_timeline_gate"] is True and
            native["caller_mesh_and_texture_poisoned_and_freed"] is True and native["cleanup_completed"] is True and
            native["present_retirement"] == "EXT_swapchain_maintenance1_present_fence", "Missing E02 lifecycle proof")
    require(native["counts"] == {"mesh_versions_created": 5, "mesh_versions_released": 5, "mesh_versions_live": 0, "mesh_peak": 2,
            "mesh_buffers_created": 4, "mesh_buffers_released": 4,
            "texture_created": 2, "texture_released": 2, "texture_live": 0, "texture_peak": 1,
            "stale_rejections": 9, "submissions": 5}, "E02 native resource/handle accounting mismatch")
    cases = (("created-before-update", 12, False, [0, 0, 1], [0, 0, 0], [0, 0]),
             ("updated-before-clear", 12, True, [1, 1, 2], [0, 0, 0], [0, 0]),
             ("cleared-before-restore", 0, False, [2, 2, 3], [0, 0, 0], [0, 0]),
             ("restored-before-destroy", 12, False, [3, 3, 4], [0, 0, 1], [0, 5]),
             ("recreated-before-destroy", 12, True, [4, 4, 5], [1, 1, 2], [7, 9]))
    require(len(native["frames"]) == len(cases), "E02 needs all five ordered frame/mutation cases")
    images = []
    for index, (frame, case) in enumerate(zip(native["frames"], cases)):
        name, vertices, swapped, mesh_released, texture_released, rejected = case
        check_lifetime_frame(frame, case, index)
        path = capture / frame["file"]
        reject_links(path)
        raw = path.read_bytes()
        pixels = check_pixels(raw, WIDTH, HEIGHT, "Linear", swapped) if vertices else check_empty_pixels(raw)
        require(pixels["passed"], "E02 independent updated/empty/deleted frame pixels mismatch: " + name)
        png = path.with_suffix(".png")
        with png.open("xb") as destination:
            destination.write(encode_png(WIDTH, HEIGHT, raw))
        require(decode_png(png.read_bytes()) == (WIDTH, HEIGHT, raw), "E02 PNG changed native pixels")
        images.append({"case": name, "pixel_checks": pixels, "raw": identity(path), "png": identity(png)})
    return {"passed": True, "images": images, "independent_pixel_oracle": True,
            "controlled_native_pending_not_spontaneous_stall": True,
            "limits": ["Experiment-only registry, not public Renderer integration",
                       "No full capacity/failure groups, E03, alpha-depth or MSAA"]}


def resize_points(width, height):
    require(200 <= width <= 4096 and 200 <= height <= 4096, "Resize fixture drawable extent is out of bounds")
    sx = height / (3 * width)
    point = lambda x, y: (int(math.floor((sx * x + 1.08) * width / 2 + .5)),
                          int(math.floor((1.06 - .35 * y) * height / 2 + .5)))
    points = [point(x, y) for xbase in (-2, 2) for y in (1, -1) for x in (xbase - .5, xbase + .5)]
    center_y = round(.53 * height)
    guards = []
    for xbase in (-2, 2):
        left, _ = point(xbase - 1, 0)
        right, _ = point(xbase + 1, 0)
        center, _ = point(xbase, 0)
        guards.extend(((left - 5, center_y), (right + 5, center_y),
                       (center, round(.18 * height) - 5), (center, round(.88 * height) + 5)))
    guards.append((round(.54 * width), center_y))
    require(all(1 <= x < width - 1 and 1 <= y < height - 1 for x, y in points + guards), "Resize oracle points clipped outside frame")
    return points, guards


def check_resize_pixels(raw, width, height):
    require(len(raw) == width * height * 4, "Resized raw byte count does not match actual drawable")
    points, guards = resize_points(width, height)
    samples = []
    for (x, y), color in [*zip(points, TEXELS), *((xy, CLEAR_RGBA) for xy in guards)]:
        patch = [pixel_at(raw, width, x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1)]
        passed = all(all(abs(pixel[i] - color[i]) <= 2 for i in range(3)) and pixel[3] == color[3] for pixel in patch)
        samples.append({"xy": [x, y], "observed": pixel_at(raw, width, x, y), "expected": color, "patch_3x3_passed": passed})
    return {"passed": all(sample["patch_3x3_passed"] for sample in samples), "samples": samples,
            "oracle": "independent height/(3*width) aspect-corrected matrix; 8 texture patches and 9 guards",
            "limits": "Not full-frame equivalence or full-game camera validation"}


def check_resize_frame(frame, index):
    names = ("initial", "larger", "wide", "narrow", "restored", "final")
    require(frame["name"] == names[index] and frame["file"] == names[index] + ".rgba" and frame["generation"] == index + 1,
            "Wrong resized frame order/native swapchain generation")
    extent = frame["extent"]
    require(len(extent) == 2 and all(isinstance(value, int) and 200 <= value <= 4096 for value in extent) and
            all(frame[key] == extent for key in ("sdl_pixels", "client_pixels", "surface_pixels")),
            "Swapchain extent differs from actual SDL/Win32/surface pixels")
    require(math.isclose(frame["projection_x"], extent[1] / (3 * extent[0]), rel_tol=0, abs_tol=1e-7), "Stale projection aspect ratio")
    require(frame["draw_and_present_completed"] is True and frame["old_size_views_live"] == 0 and frame["framebuffers_live"] == 0,
            "Old dimension-dependent references are not retired")


def check_resize_pause(pause):
    require(pause["minimized"] is True and pause["iconic"] is True and pause["effective_render_extent"] == [0, 0] and
            10 <= pause["duration_seconds"] <= 15 and 50 <= pause["steps"] <= 150,
            "Missing finite real minimized/zero-extent pause")
    require(pause["calls_before"] == pause["calls_after"] == [4, 4, 4, 4], "Acquire/submit/present/rebuild happened during zero-size pause")
    require(all(len(pause[key]) == 2 and all(isinstance(value, int) and value >= 0 for value in pause[key]) for key in
                ("sdl_pixels", "client_pixels", "surface_pixels")), "Missing separate raw minimized dimensions")


def resize_self_test():
    frame = {"name": "wide", "file": "wide.rgba", "generation": 3, "extent": [960, 540],
             "sdl_pixels": [960, 540], "client_pixels": [960, 540], "surface_pixels": [960, 540],
             "projection_x": .1875, "draw_and_present_completed": True, "old_size_views_live": 0, "framebuffers_live": 0}
    check_resize_frame(frame, 2)
    pause = {"minimized": True, "iconic": True, "effective_render_extent": [0, 0], "duration_seconds": 10.01,
             "steps": 100, "calls_before": [4]*4, "calls_after": [4]*4, "sdl_pixels": [720, 600],
             "client_pixels": [0, 0], "surface_pixels": [0, 0]}
    check_resize_pause(pause)
    for source, validate, changes in ((frame, lambda row: check_resize_frame(row, 2),
                                     (("generation", 2), ("surface_pixels", [640, 480]), ("projection_x", .25),
                                      ("draw_and_present_completed", False), ("old_size_views_live", 1))),
                                    (pause, check_resize_pause, (("calls_after", [5]*4), ("steps", 10000),
                                                               ("duration_seconds", .1), ("minimized", False)))):
        for key, value in changes:
            try:
                validate({**source, key: value})
            except ValueError:
                pass
            else:
                raise ValueError("Incorrect E03 proof accepted: " + key)
    for width, height in ((640, 480), (800, 600), (960, 540), (720, 600)):
        raw = bytearray(bytes(CLEAR_RGBA) * width * height)
        points, _ = resize_points(width, height)
        for (x, y), color in zip(points, TEXELS):
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    offset = ((y + dy) * width + x + dx) * 4
                    raw[offset:offset + 4] = bytes(color)
        require(check_resize_pixels(raw, width, height)["passed"], "Resized positive pixel oracle failed")
        x, y = points[0]; offset = (y * width + x) * 4
        raw[offset:offset + 4] = bytes(TEXELS[4])
        require(not check_resize_pixels(raw, width, height)["passed"], "Wrong restored texture layer accepted")


def verify_resize(native, capture, configuration):
    require(native["schema"] == "symocraft.vulkan-r1-e03.native.v1", "Wrong E03 native schema")
    device = native["device"]
    require(device["vendor_id"] == 0x10de and device["device_id"] == 0x2c05 and "RTX 5070 Ti" in device["device_name"] and
            device["api_version"] >= (1 << 22 | 2 << 12) and device["driver_version"] and device["queue_family"] >= 0, "Wrong E03 actual adapter/API")
    require(native["samples"] == 1 and native["present_mode"] == "Fifo" and native["swapchain_format"] in ("B8G8R8A8_UNORM", "R8G8B8A8_UNORM") and
            native["swapchain_color_space"] == "SRGB_NONLINEAR" and native["swapchain_clipped"] is False, "Wrong E03 presentation/readback mode")
    require(native["validation"] == {"enabled": True, "synchronization_enabled": True, "warnings": 0, "errors": 0} and
            native["direction_shader_input"] is (configuration == "Debug") and native["window_surface_device_unchanged"] is True and
            native["cleanup_completed"] is True and native["present_retirement"] == "EXT_swapchain_maintenance1_present_fence", "E03 diagnostic/owner/retirement mismatch")
    require(all(native[key] == value for key, value in {"geometry_uploads": 1, "texture_uploads": 1, "acquire_calls": 6,
            "submit_calls": 6, "present_calls": 6, "swapchain_create_calls": 6, "swapchain_destroy_calls": 6,
            "swap_views_live": 0, "framebuffers_live": 0}.items()), "E03 actual resource/presentation accounting mismatch")
    check_resize_pause(native["pause"])
    require(len(native["frames"]) == 6 and len({tuple(frame["extent"]) for frame in native["frames"]}) >= 4, "Missing independent real pixel-size changes")
    require(native["frames"][3]["extent"] == native["frames"][4]["extent"] and
            native["frames"][0]["extent"] == native["frames"][5]["extent"], "Restored/final drawable differs from original requested shape")
    images = []
    for index, frame in enumerate(native["frames"]):
        check_resize_frame(frame, index)
        path = capture / frame["file"]
        reject_links(path)
        raw = path.read_bytes(); width, height = frame["extent"]
        metadata = json.loads(path.with_suffix(".json").read_text(encoding="utf-8-sig"))
        require(metadata == {"schema": "symocraft.vulkan.capture.v1", "width": width, "height": height,
                             "format": "RGBA8", "origin": "top-left"}, "E03 raw capture metadata differs from actual framebuffer")
        pixels = check_resize_pixels(raw, width, height)
        require(pixels["passed"], "E03 independent resized/restored pixels failed: " + frame["name"])
        png = path.with_suffix(".png")
        with png.open("xb") as destination:
            destination.write(encode_png(width, height, raw))
        require(decode_png(png.read_bytes()) == (width, height, raw), "E03 PNG changed actual readback")
        images.append({"case": frame["name"], "pixel_checks": pixels, "raw": identity(path), "png": identity(png)})
    return {"passed": True, "images": images, "raw_minimized_sizes_not_rewritten_to_zero": True,
            "limits": ["Experiment only, not public Renderer/full game A11/A15", "No depth/MSAA rebuild or injected out-of-date/suboptimal"]}


def parser():
    result = argparse.ArgumentParser(description=__doc__)
    result.add_argument("--configuration", choices=("Debug", "Release"), default="Release")
    result.add_argument("--attempt", type=int, default=1)
    result.add_argument("--experiment", choices=("E01", "E02", "E03"), default="E01")
    result.add_argument("--output-root")
    result.add_argument("--production-build")
    result.add_argument("--sdk", default="C:/VulkanSDK/1.4.363.0")
    result.add_argument("--cmake", default="C:/Program Files/CMake/bin/cmake.exe")
    result.add_argument("--ninja", default="E:/Applications/JetBrains/CLion/bin/ninja/win/x64/ninja.exe")
    result.add_argument("--visual-studio", default="E:/Applications/Microsoft VS/2022")
    result.add_argument("--toolset", default="14.38")
    result.add_argument("--self-test", action="store_true")
    return result


def collect(args):
    require(1 <= args.attempt <= 999, "Attempt must be between 1 and 999; there is no automatic retry")
    base = (ROOT / (args.output_root or "out/m3-t3/r4-" + args.experiment.lower() + "-001")).resolve()
    require(base.is_relative_to(ROOT / "out/m3-t3"), "Experiment outputs must stay below out/m3-t3")
    output = base / (args.configuration.lower() + f"-{args.attempt:03}")
    reject_links(output)
    output.mkdir(parents=True, exist_ok=False)
    manifest = {"schema": "symocraft.vulkan-" + args.experiment.lower() + ".v1", "experiment": args.experiment, "configuration": args.configuration,
                "started_utc": datetime.now(timezone.utc).isoformat(), "commands": [], "passed": False,
                "automatic_retry": False, "performance_sampling": False,
                "game_takeover_or_complete_A06_A09": False, "complete_A08_A10_A15": False,
                "runner": identity(Path(__file__))}
    sources = [ROOT / "assets/shaders/hlsl" / name for name in ("blocks.hlsl", "draw_constants.hlsli")]
    source_root = ROOT / "test/experimental/platform-sdl3"
    sources += [source_root / "CMakeLists.txt", *sorted(source_root.glob("vulkan*.cpp")),
                *sorted(source_root.glob("vulkan*.h"))]

    def execute(command, label, timeout, env):
        item = {"label": label, "command": list(map(str, command)), "timeout_seconds": timeout}
        manifest["commands"].append(item)
        try:
            result = subprocess.run(item["command"], cwd=output, env=env, capture_output=True,
                                    timeout=timeout, **hidden_process())
            item["exit_code"] = result.returncode
            (output / (label + ".stdout.log")).write_bytes(result.stdout)
            (output / (label + ".stderr.log")).write_bytes(result.stderr)
            print(f"{label}: exit={result.returncode}", flush=True)
            require(result.returncode == 0, label + " failed; original stdout/stderr retained")
            return result.stdout
        except subprocess.TimeoutExpired as error:
            item["timed_out"] = True
            (output / (label + ".stdout.log")).write_bytes(error.stdout or b"")
            (output / (label + ".stderr.log")).write_bytes(error.stderr or b"")
            raise

    try:
        env = developer_environment(Path(args.visual_studio), args.toolset)
        env.pop("SDL_DYNAMIC_API", None)
        sdk = Path(args.sdk).resolve(strict=True)
        cmake, ninja = Path(args.cmake).resolve(strict=True), Path(args.ninja).resolve(strict=True)
        compiler = Path(env["VCTOOLSINSTALLDIR"]) / "bin/Hostx64/x64/cl.exe"
        tools = {"dxc": sdk / "Bin/dxc.exe", "spirv_val": sdk / "Bin/spirv-val.exe",
                 "spirv_cross": sdk / "Bin/spirv-cross.exe", "cmake": cmake, "ninja": ninja,
                 "compiler": compiler}
        manifest["tools"] = {key: {**identity(path), "file_version": file_version(path)} for key, path in tools.items()}
        for name, flags in (("dxc", ["--version"]), ("spirv_val", ["--version"]),
                            ("spirv_cross", ["--help"]), ("cmake", ["--version"]), ("ninja", ["--version"])):
            execute([tools[name], *flags], name + "-version", 15, env)
        manifest["toolchain_environment"] = {key: env.get(key) for key in
                ("VCTOOLSVERSION", "WINDOWSSDKDIR", "WINDOWSSDKVERSION", "UCRTVERSION", "VULKAN_SDK",
                 "VK_ICD_FILENAMES", "VK_DRIVER_FILES", "VK_LAYER_PATH", "VK_INSTANCE_LAYERS")}
        manifest["sources"] = [identity(path) for path in sources]
        shader_directory = output / "shaders"
        shader_directory.mkdir()
        flags = ["-nologo", "-Ges", "-WX", "-Zpc", "-Od" if args.configuration == "Debug" else "-O3",
                 "-spirv", "-fspv-target-env=vulkan1.2", "-fvk-use-dx-layout",
                 "-fvk-b-shift", "0", "0", "-fvk-t-shift", "1", "0", "-fvk-s-shift", "2", "0"]
        manifest["shader_configuration"] = {"target_environment": "vulkan1.2", "layout": "dx-layout",
                "profiles": ["vs_6_0", "ps_6_0"], "compiler_invert_y": False, "flags": flags}
        reflections = {}
        for stage, entry in (("vs", "VSMain"), ("ps", "PSMain")):
            shader = shader_directory / ("blocks." + stage + ".spv")
            execute([tools["dxc"], *flags, "-E", entry, "-T", stage + "_6_0", "-I", sources[0].parent,
                     "-Fo", shader, sources[0]], "compile-" + stage, 30, env)
            execute([tools["spirv_val"], "--target-env", "vulkan1.2", shader], "validate-" + stage, 15, env)
            reflections[stage] = json.loads(execute([tools["spirv_cross"], shader, "--reflect"],
                                                    "reflect-" + stage, 15, env))
        manifest["reflections"] = reflections
        manifest["reflection_check"] = verify_reflection(reflections["vs"], reflections["ps"], args.configuration)
        production = (ROOT / (args.production_build or "out/build/windows-" + args.configuration.lower())).resolve(strict=True)
        manifest["production_build"] = str(production)
        execute([cmake, "-S", source_root, "-B", output / "build", "-G", "Ninja",
                 "-DCMAKE_BUILD_TYPE=" + args.configuration, "-DCMAKE_MAKE_PROGRAM=" + str(ninja),
                 "-DCMAKE_C_COMPILER=" + str(compiler), "-DCMAKE_CXX_COMPILER=" + str(compiler),
                 "-DSYMOCRAFT_PLATFORM_PRODUCTION_BUILD=" + str(production),
                 "-DSYMOCRAFT_PLATFORM_PROBE_VULKAN=ON", "-DSYMOCRAFT_PLATFORM_PROBE_TEXTURED=ON",
                 "-DSYMOCRAFT_PLATFORM_PROBE_LIFETIME=" + ("ON" if args.experiment == "E02" else "OFF"),
                 "-DSYMOCRAFT_PLATFORM_PROBE_RESIZE=" + ("ON" if args.experiment == "E03" else "OFF"),
                 "-DSYMOCRAFT_VULKAN_E01_DXC=" + str(tools["dxc"]),
                 "-DSYMOCRAFT_VULKAN_HEADERS_ROOT=" + str(sdk / "Include")], "configure", 60, env)
        target = {"E01": "SymoCraftVulkanTexturedProbe", "E02": "SymoCraftVulkanLifetimeProbe", "E03": "SymoCraftVulkanResizeProbe"}[args.experiment]
        execute([cmake, "--build", output / "build", "--target", target, "--parallel", "8"],
                "build", 120, env)
        imports_file = output / "build/production-imports.json"
        manifest["production_imports"] = json.loads(imports_file.read_text(encoding="utf-8-sig"))
        manifest["imports_manifest"] = identity(imports_file)
        executable = output / "build" / (target + ".exe")
        manifest["executable"] = identity(executable)
        manifest["shaders"] = [identity(shader_directory / name) for name in ("blocks.vs.spv", "blocks.ps.spv")]
        manifest["cmake_dependency_shader_outputs_not_used_for_draw"] = [
            identity(output / "build/shaders" / name) for name in ("blocks.vs.spv", "blocks.ps.spv")]
        dependencies = execute([compiler.with_name("dumpbin.exe"), "/DEPENDENTS", executable], "imports", 15, env)
        require("vulkan-1.dll" not in dependencies.decode("utf-8", errors="replace").lower(),
                "Experimental probe must use the production SDL Vulkan loader rather than an import library")
        manifest["no_vulkan_import_library"] = True
        capture = output / "capture"
        execute([executable, "--vertex", shader_directory / "blocks.vs.spv", "--fragment",
                 shader_directory / "blocks.ps.spv", "--output", capture], "native", 60, env)
        native = json.loads((capture / "native.json").read_text(encoding="utf-8-sig"))
        manifest["native"] = native
        verify = {"E01": verify_native, "E02": verify_lifetime, "E03": verify_resize}[args.experiment]
        manifest["pixel_checks"] = verify(native, capture, args.configuration)
        require(manifest["pixel_checks"]["passed"], "Independent experiment pixel checks failed; raw captures retained")
        for item in (*manifest["sources"], *manifest["shaders"], manifest["executable"], manifest["runner"]):
            require(sha256(Path(item["path"])) == item["sha256"], "Source/shader/executable/runner changed during experiment")
        for item in manifest["production_imports"]["imports"]:
            require(sha256(Path(item["archive"])) == item["sha256"], "Imported production archive changed during experiment")
        for item in manifest["tools"].values():
            require(sha256(Path(item["path"])) == item["sha256"], "Compiler/validator tool changed during experiment")
        manifest["passed"] = True
    except Exception as error:
        manifest["failure"] = str(error)
        manifest["failure_capture_diagnostics"] = preserve_failure_images(output / "capture")
        raise
    finally:
        manifest["finished_utc"] = datetime.now(timezone.utc).isoformat()
        manifest["artifacts"] = [identity(path, output) for path in sorted(output.rglob("*"))
                                 if path.is_file() and "build" not in path.relative_to(output).parts]
        (output / "result.json").write_text(json.dumps(manifest, ensure_ascii=True, indent=2), encoding="utf-8")
    print(f"Vulkan {args.experiment} checks passed: {output}")
    return manifest


def main():
    args = parser().parse_args()
    if args.self_test:
        self_test()
        return
    collect(args)


if __name__ == "__main__":
    main()
