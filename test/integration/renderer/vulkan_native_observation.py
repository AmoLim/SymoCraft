"""Private native-call snapshots, never inferred from logical phase visits."""
import csv
import math
from collections import Counter
from pathlib import Path

CALLS = (
    "buffer_create", "buffer_allocate", "buffer_bind", "buffer_destroy", "buffer_free",
    "image_create", "image_allocate", "image_bind", "image_destroy", "image_free",
    "map_write", "unmap_write", "flush_write", "pool_create", "sets_allocate", "descriptor_update", "pool_destroy",
    "query_read", "fence_poll", "wait_submit", "wait_upload", "wait_reacquired_present", "wait_drain_present",
    "acquire", "queue_submit", "queue_present")
ROLES = ("mesh", "mesh_uniform", "selection_uniform", "selection_vertex", "staging", "readback")
WRITES = ("ubo_descriptor_writes", "image_descriptor_writes", "sampler_descriptor_writes")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def nonnegative(value, integer=False):
    parsed = int(value) if integer else float(value)
    require(math.isfinite(parsed) and parsed >= 0, "Invalid native counter/timing")
    return parsed


def deltas(raw):
    if not any("native_model" in row for row in raw):
        return None
    require(all(row.get("native_model") == "native_calls_v1" for row in raw), "Missing/mixed native model")
    previous, previous_id, result = None, 0, {}
    for row in raw:
        frame_id = nonnegative(row["renderer_frame_id"], True)
        require(frame_id > previous_id, "Native frame IDs must be strictly increasing")
        require(int(row["observed_frame_id"]) == frame_id, "Stale present observation frame ID")
        require(0 <= int(row["frame_slot"]) < 3 and int(row["present_image"]) >= 0, "Invalid actual slot/image")
        require(int(row["submit_before_present"]) in (0, 1) and int(row["submit_after_present"]) in (0, 1),
                "Native fence observation failed; never substitute readiness")
        current = {}
        for name in CALLS:
            values = tuple(nonnegative(row[name + suffix], suffix != "_native_wall_ms")
                           for suffix in ("_calls", "_successes", "_native_wall_ms"))
            require(values[1] <= values[0], "Native successes exceed calls")
            current[name] = values
        for name in (*WRITES, *(role + "_allocations" for role in ROLES)):
            current[name] = nonnegative(row[name], True)
        change = None
        if previous is not None:
            change = {}
            for name in CALLS:
                values = tuple(after - before for before, after in zip(previous[name], current[name]))
                require(values[0] >= 0 and 0 <= values[1] <= values[0] and values[2] >= -1e-6,
                        "Native cumulative counter moved backwards")
                change[name] = (values[0], values[1], max(0., values[2]))
            for name in (*WRITES, *(role + "_allocations" for role in ROLES)):
                change[name] = current[name] - previous[name]
                require(change[name] >= 0, "Native cumulative allocation/write count moved backwards")
        result[str(frame_id)] = (row, change)
        previous, previous_id = current, frame_id
    return result


def allocations(types, records):
    by_type = {nonnegative(row["memory_type"], True): row for row in types}
    require(len(by_type) == len(types) and bool(types), "Missing/duplicate native memory types")
    seen, result = set(), []
    for row in records:
        role, index = row["role"], nonnegative(row["memory_type"], True)
        require(role in ROLES and index in by_type and (role, index) not in seen, "Unknown/duplicate buffer allocation identity")
        seen.add((role, index))
        native = by_type[index]
        values = {key: nonnegative(row[key], True) for key in (
            "allocations", "binds", "releases", "requested_bytes", "allocated_bytes", "live_bytes", "requirements_bits_or", "requirements_bits_and")}
        require(values["allocations"] > 0 and values["allocations"] == values["binds"] == values["releases"] and values["live_bytes"] == 0,
                "Normal buffer allocation/bind/release counts unbalanced")
        require(0 < values["requested_bytes"] <= values["allocated_bytes"], "Invalid actual buffer byte counts")
        mask = values["requirements_bits_and"]
        require(mask & (1 << index) and (values["requirements_bits_or"] | mask) == values["requirements_bits_or"],
                "Chosen memory type incompatible with requirements")
        flags = nonnegative(native["property_flags"], True)
        require(flags & 2, "Observed host-written buffer is not HOST_VISIBLE")
        result.append({"role": role, "memory_type": index, "property_flags": flags,
            "heap_index": int(native["heap_index"]), "heap_flags": int(native["heap_flags"]),
            "device_local": bool(flags & 1), **values,
            "device_local_types_compatible_with_all_allocations": [i for i, t in by_type.items()
                if int(t["property_flags"]) & 1 and mask & (1 << i)]})
    require(any(row["role"] == "mesh" for row in result), "Missing actual mesh allocation observations")
    return result


def read(path):
    with Path(path).open(encoding="utf-8-sig", newline="") as file:
        return list(csv.DictReader(file))


def distribution(values):
    require(bool(values), "No native values")
    ordered = sorted(values)
    return {"mean": sum(values) / len(values), "p50": ordered[math.ceil(len(values) * .5) - 1],
            "p95": ordered[math.ceil(len(values) * .95) - 1], "p99": ordered[math.ceil(len(values) * .99) - 1], "max": ordered[-1]}


def analyze(raw, frames, timings):
    observed = deltas(raw)
    if observed is None:
        return {"model": "unmeasured_v1", "calls": None, "allocations": None}
    sample = [row for row in frames if 10 <= float(row["elapsed_s"]) < 60]
    joined = []
    for frame in sample:
        require(frame["renderer_frame_id"] in observed, "Missing same-frame native snapshot")
        row, change = observed[frame["renderer_frame_id"]]
        require(change is not None, "Startup cumulative snapshot cannot be used as a frame delta")
        require(row["screenshot"] == "0" and row["completed"] == "1", "Incomplete/screenshot native sample")
        for call, phase in (("queue_present", "queue_present"), ("acquire", "acquire_image"), ("wait_reacquired_present", "present_fence_wait")):
            require(change[call][2] <= float(row[phase + "_wall_ms"]) + .002, "Nested native interval exceeds owning phase")
        joined.append((frame, row, change))
    require(bool(joined), "No joined native sample")
    prefix = str(timings)
    memory = allocations(read(prefix + ".memory-types.csv"), read(prefix + ".buffer-allocations.csv"))
    devices = read(prefix + ".device.csv")
    require(len(devices) == 1, "Missing native device snapshot")
    device = devices[0]
    image_count = int(device["swapchain_images"])
    require(image_count > 0 and all(int(row["present_image"]) < image_count for _, row, _ in joined), "Stale swapchain image index")
    final = read(prefix + ".native-final.csv")
    require({r["call"] for r in final} == set(CALLS) and len(final) == len(CALLS), "Missing native final counters")
    totals = {r["call"]: int(r["calls"]) for r in final}
    require(totals["buffer_create"] == totals["buffer_allocate"] == totals["buffer_bind"] == totals["buffer_destroy"] == totals["buffer_free"],
            "Native Buffer calls unbalanced at normal shutdown")
    require(totals["image_create"] == totals["image_allocate"] == totals["image_bind"] == totals["image_destroy"] == totals["image_free"],
            "Native Image calls unbalanced at normal shutdown")
    require(totals["pool_create"] == totals["pool_destroy"], "Native pool calls unbalanced at normal shutdown")
    bindings = read(prefix + ".binding-slots.csv")
    require(len(bindings) == 6 and {(int(r["slot"]),r["kind"]) for r in bindings} == {(i,k) for i in range(3) for k in ("mesh","selection")},
            "Missing per-slot native binding totals")
    require(all(int(r["pools_created"]) == int(r["pools_released"]) for r in bindings), "Binding pool release mismatch")
    top = sorted(joined, key=lambda item: float(item[0]["frame_ms"]), reverse=True)[:math.ceil(len(joined) * .01)]

    def group(selected):
        statuses = Counter((int(row["submit_before_present"]), int(row["submit_after_present"])) for _,row,_ in selected)
        return {"frames": len(selected), "frame_ms": distribution([float(f["frame_ms"]) for f,_,_ in selected]),
            "native_present_ms": distribution([d["queue_present"][2] for _,_,d in selected]),
            "gpu_draw_ms": distribution([float(f["gpu_draw_ms"]) for f,_,_ in selected]),
            "fence_before_after_counts": {f"{a}->{b}":count for (a,b),count in sorted(statuses.items())},
            "tracked_submit_pending": sorted({int(r["tracked_submit_pending"]) for _,r,_ in selected}),
            "tracked_present_pending": sorted({int(r["tracked_present_pending"]) for _,r,_ in selected}),
            "present_host_time_share": sum(d["queue_present"][2] for _,_,d in selected) / sum(float(f["frame_ms"]) for f,_,_ in selected)}

    return {"model": "native_calls_v1", "device": device, "allocations": memory,
        "calls": {name:{"calls":sum(d[name][0] for _,_,d in joined),"successes":sum(d[name][1] for _,_,d in joined),
            "host_wall_ms_per_frame":distribution([d[name][2] for _,_,d in joined])} for name in CALLS},
        "role_allocation_deltas": {role:sum(d[role + "_allocations"] for _,_,d in joined) for role in ROLES},
        "descriptor_write_deltas": {name:sum(d[name] for _,_,d in joined) for name in WRITES},
        "mesh_observations": {key:sorted({int(row[key]) for _,row,_ in joined}) for key in
            ("mesh_bound_buffers", "mesh_bound_bytes", "mesh_device_local_buffers", "mesh_device_local_bytes")},
        "all_frames": group(joined), "slowest_one_percent": group(top), "binding_slots_final": bindings,
        "limitations": "Host snapshots and nonblocking submit-fence status are not hardware queue occupancy, presentation completion, PCIe bandwidth, or GPU residency tracing. Native deltas include calls since the previous Render snapshot, including pre-Render polling; native timings are nested, not additive to owning stages."}
