"""Analyze the frozen nine-round exploration without granting acceptance."""
import argparse
from datetime import date
import math
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import cli, load_json, option, require, sha256, write_json


def number(value):
    value = float(value)
    require(math.isfinite(value) and value >= 0, "Invalid metric.")
    return value


def value_range(values):
    values = sorted(values)
    require(len(values) == 3, "Each scene must retain three valid rounds.")
    return {"min": values[0], "median": values[1], "max": values[2], "spread": values[2] - values[0]}


def up(value, step):
    return round(math.ceil(value / step) * step, 6)


def analyze(directory, analysis_date, metrics_only=False):
    date.fromisoformat(analysis_date)
    directory = Path(directory)
    summary_path = directory / "summary.json"
    summary = load_json(summary_path)
    require(summary["passed"] and summary["actual_game_processes"] == 9 and summary["passed_count"] == 9,
            "A complete nine-round valid exploration is required for this budget proposal.")
    rounds, scenes = [], []
    for scene in ("static", "walk", "edit"):
        runs = [x for x in summary["rounds"] if x["scene"] == scene]
        require(len(runs) == 3, f"Missing scene rounds: {scene}")
        for run in runs:
            require(run["passed"] and run["memory"]["passed"], "Invalid round or memory coverage.")
            m, memory = run["metrics"], run["memory"]
            frame_count = number(m["frame_count"])
            require(frame_count > 0, "Frame count must be positive.")
            missing = number(m["gpu_missing_sample_frames"])
            ws, private = memory["working_set_bytes"], memory["private_bytes"]
            rounds.append({"scene": scene, "repeat": run["repeat"], "frames": frame_count,
                "p95_ms": number(m["frame_p95_ms"]), "p99_ms": number(m["frame_p99_ms"]),
                "first_frame_ms": number(m["first_frame_swap_return_from_main_entry_ms"]),
                "ws_observed_max_mib": number(ws["max_observed_bytes"]) / 1048576,
                "private_observed_max_mib": number(private["max_observed_bytes"]) / 1048576,
                "ws_growth_mib": (number(ws["last_bytes"]) - number(ws["first_bytes"])) / 1048576,
                "private_growth_mib": (number(private["last_bytes"]) - number(private["first_bytes"])) / 1048576,
                "memory_points": memory["samples"], "gpu_count": number(m["gpu_count"]),
                "gpu_missing": missing, "gpu_missing_ratio": missing / frame_count})
        scene_rounds = [x for x in rounds if x["scene"] == scene]
        scenes.append({"scene": scene, **{key: value_range([x[key] for x in scene_rounds]) for key in
            ("p95_ms", "p99_ms", "first_frame_ms", "ws_observed_max_mib", "private_observed_max_mib")}})
    policies = [dict(id=1, name="tight", p95=1.05, p99=1.10, memory=1.10, startup=1.10),
                dict(id=2, name="balanced", p95=1.10, p99=1.20, memory=1.15, startup=1.20),
                dict(id=3, name="wide", p95=1.20, p99=1.30, memory=1.25, startup=1.30)]
    options = [] if metrics_only else [{"id": policy["id"], "name": policy["name"], "recommended": policy["id"] == 2,
        "nominal_policy_multipliers": policy, "frame_limits": [{"scene": x["scene"], "frame_p95_ms": up(x["p95_ms"]["max"] * policy["p95"], .1),
            "frame_p99_ms": up(x["p99_ms"]["max"] * policy["p99"], .1)} for x in scenes],
        "ws_observed_max_limit_mib": up(max(x["ws_observed_max_mib"] for x in rounds) * policy["memory"], 64),
        "private_observed_max_limit_mib": up(max(x["private_observed_max_mib"] for x in rounds) * policy["memory"], 64),
        "first_frame_limit_ms": up(max(x["first_frame_ms"] for x in rounds) * policy["startup"], 50)} for policy in policies]
    return {"date": analysis_date, "role": "Frozen GLFW repeat exploratory P95/P99 measurement; not matched Q06 acceptance" if metrics_only else "Frozen GLFW exploratory Q07 budget proposal; not matched Q06 acceptance",
        "source_summary": str(summary_path), "source_summary_sha256": sha256(summary_path), "source_manifest_sha256": sha256(directory / "manifest.json"),
        "planned": 9, "valid": 9, "rounds": rounds, "scenes": scenes, "options": options,
        "budget_approved": False, "sdl_acceptance_sampled": False, "fix1_resumed": False,
        "interpretation": ["Nearest-rank per round. A median of three P95 values is not a pooled percentile.",
            "Candidate headroom is a policy choice, not a confidence interval or a proven noise bound.",
            "Only three independent 30-second rounds per scene; keep all slow and unfocused frames.",
            "Memory limits apply to periodic observations in [10,40)s, not loading/export peaks, VRAM, or a leak guarantee.",
            "These old-source measurements must not be combined with current SDL as a matched migration comparison.",
            "No acceptance outcome before human budget confirmation and required same-source comparison."]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("InputDirectory", "OutputFile", "AnalysisDate"):
        option(parser, name, required=True)
    option(parser, "MetricsOnly", action="store_true")
    args = parser.parse_args()
    require(not Path(args.OutputFile).exists(), "Analysis output must be new.")
    result = analyze(args.InputDirectory, args.AnalysisDate, args.MetricsOnly)
    write_json(args.OutputFile, result)
    print({"scenes": result["scenes"], "options": result["options"]})


if __name__ == "__main__":
    cli(main)
