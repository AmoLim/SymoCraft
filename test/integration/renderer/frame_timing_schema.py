"""Private layer-work semantics; legacy missing measurements never become zero."""
import math


def layer_work(row, frame, legacy_model):
    try:
        return _layer_work(row, frame, legacy_model)
    except (KeyError, TypeError, OverflowError) as error:
        raise ValueError("Incomplete layer-work measurement") from error


def _layer_work(row, frame, legacy_model):
    model = row.get("layer_validation_model", legacy_model)
    if model not in (legacy_model, "cached_max_v1"):
        raise ValueError("Unknown layer validation model")
    if "layer_validation_model" not in row and "layer_summary_check_work_ms" in row:
        raise ValueError("Missing layer validation model on extended timing row")
    if model == "unmeasured_v1":
        if any(key.startswith(("layer_scan_", "layer_summary_")) or key == "layer_vertices" for key in row):
            raise ValueError("Unversioned layer fields cannot be treated as unmeasured")
        return {"model": model, "wall_ms": None, "visits": None, "scanned_vertices": None}
    selection = int(row["selection_allocate_visits"]) > 0
    scan = float(row["layer_scan_work_ms"])
    scan_visits = int(row["layer_scan_work_visits"])
    vertices = int(row["layer_vertices"])
    if model == "per_vertex_v1":
        if not math.isfinite(scan) or scan < 0 or scan_visits <= 0 or vertices != int(frame["vertices"]) - (24 if selection else 0):
            raise ValueError("Missing/invalid legacy scan or vertex mapping")
        return {"model": model, "wall_ms": scan, "visits": scan_visits, "scanned_vertices": vertices}
    wall = float(row["layer_summary_check_work_ms"])
    visits = int(row["layer_summary_check_work_visits"])
    if scan != 0 or scan_visits != 0 or vertices != 0:
        raise ValueError("Cached model still reports legacy vertex scanning")
    if not math.isfinite(wall) or wall < 0 or visits != int(frame["draw_calls"]) - int(selection) or visits <= 0:
        raise ValueError("Missing/invalid summary check or draw mapping")
    return {"model": model, "wall_ms": wall, "visits": visits, "scanned_vertices": 0}
