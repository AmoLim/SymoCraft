"""Validators for reports emitted by the native SDL capability fixtures."""

import argparse
from pathlib import Path
import re
from verification_common import ROOT, assert_hash, capture, clean_environment, evidence_sources, file_path, identity, json_probe, new_directory, option, platform_imports, read_log, require, sha256, utc_now, workspace_path, write_json
from scripts_common import integer


def capability_cases(vulkan, fixture=None):
    cases = [("gl", "gl", 0, "", {}, [], False), ("native", "native", 0, "", {}, ["--wait-close"] if fixture else [], False),
             ("benchmark-window", "gl", 0, "", {}, ["--benchmark", "--allow-unfocused", "--wait-close"] if fixture else
              ["--benchmark", "--allow-unfocused", "--width", "1920", "--height", "1080", "--test-wait-close"], False),
             ("initialization-failure", "native", 1, "SDL_InitSubSystem" if fixture else "SDL_Init(SDL_INIT_VIDEO)",
              {"SDL_VIDEO_DRIVER": "symocraft-invalid-driver"}, [], False)]
    if fixture:
        cases += [("lifecycle", "lifecycle", 0, "", {}, [], False),
                  ("opengl-library-failure", "gl", 1, "SDL_CreateWindow", {"SDL_OPENGL_LIBRARY": "symocraft-missing-opengl-library.dll"}, [], False)]
    cases.append(("vulkan" if vulkan else "vulkan-disabled", "vulkan", 0 if vulkan else 1,
                  "" if vulkan else "Vulkan mode was not compiled", {}, ["--wait-close"] if fixture and vulkan else [], False))
    if fixture:
        if vulkan:
            cases.append(("vulkan-library-failure", "vulkan", 1, "SDL_CreateWindow", {"SDL_VULKAN_LIBRARY": "symocraft-missing-vulkan-library.dll"}, [], False))
        for fault in ["create-window", "create-context", "make-current", "set-vsync", "present", "query-extension", "get-procedure", "destroy-context-after-release"] + (["surface"] if vulkan else []):
            cases.append(("restricted-" + fault, "vulkan" if fault == "surface" else "gl", 1, fault,
                          {"SDL3_DYNAMIC_API": str(fixture)}, ["--fault", fault], True))
    return cases


def capability_valid(record, errors, case, production=False):
    name, mode, expected, diagnostic, env, extra, fault = case
    report = record.get("report") or {}
    if record["timed_out"] or record["exit_code"] != expected or not report:
        return False
    if expected:
        valid = report.get("status") == "fail" and diagnostic in errors
        if production:
            valid = valid and report.get("video_active_after_failure") is False
        if fault:
            valid = valid and report.get("restricted_fixture_loaded") is True and report.get("fixture_failures_injected") == 1
            valid = valid and all(report.get("fixture_" + kind + "s_created") is not None and
                                  report.get("fixture_" + kind + "s_created") == report.get("fixture_" + kind + "s_destroyed") for kind in ("window", "context"))
            valid = valid and report.get("fixture_video_quit_calls", 0) > 0
            if name == "restricted-surface":
                valid = valid and report.get("vulkan_instance_created") is True and report.get("vulkan_instance_destroyed") is True and report.get("fixture_surface_create_calls") == 1 and report.get("fixture_surfaces_destroyed") == 0
        return bool(valid)
    valid = report.get("status") == "pass" and all(report.get(k) is True for k in ("window_destroyed", "sdl_shutdown_completed")) and "cleanup:" not in errors
    if not production:
        return valid and report.get("native_hwnd_valid") is True
    valid = valid and report.get("destroy_repeated") is True and report.get("free_repeated") is True
    if mode == "gl":
        valid = valid and report.get("gl_samples") == 4 and report.get("gl_sample_buffers") == 1 and report.get("vsync_on_actual") == 1 and report.get("vsync_off_actual") == 0 and report.get("gl_presented_frames", 0) > 0
    if mode == "vulkan":
        valid = valid and all(report.get(k) is True for k in ("vulkan_instance_created", "vulkan_surface_created", "vulkan_surface_destroyed", "vulkan_instance_destroyed"))
    if "--wait-close" in extra:
        valid = valid and report.get("wait_close_received") is True
    if mode == "lifecycle":
        valid = valid and report.get("external_sdl_owner_preserved") is True and report.get("init_repeated") is True
    return bool(valid)


def input_valid(record, errors):
    report = record.get("report") or {}
    valid = not record["timed_out"] and all(report.get(k) is True for k in ("window_destroyed", "sdl_shutdown_completed")) and "cleanup:" not in errors
    return (valid and record["exit_code"] == 0 and report.get("status") == "pass" and report.get("checks", 0) > 0,
            valid and record["exit_code"] == 2 and report.get("status") == "partial")


def allocation_valid(record, errors, case):
    r = record.get("report") or {}
    valid = not record["timed_out"] and record["exit_code"] == 0 and r.get("status") == "pass" and r.get("failures_injected") == 1 and all(r.get(k) is True for k in
            ("window_destroyed", "gl_context_released", "external_video_owner_preserved", "sdl_shutdown_completed")) and not re.search(r"cleanup:|Window secondary cleanup failure:", errors)
    if case == "window-object":
        return valid and r.get("windows_shown") == 0 and r.get("windows_destroyed") == 0 and "Window::Create: Window/Impl allocation failed" in r.get("expected_diagnostic", "")
    valid = valid and r.get("windows_shown") == 1 and r.get("windows_destroyed") == 1 and r.get("actual_context_observed") is True and r.get("context_gone_before_window_destroy_event") is True
    if case == "input-reserve":
        return valid and "Window::Create: input buffers could not be allocated" in r.get("expected_diagnostic", "")
    return valid and r.get("calibrated_growth_capacity", 0) > 64 and all(r.get(k) is True for k in
           ("previous_snapshot_preserved", "capture_rethrows_saved_failure", "previous_nonempty_snapshot_observed")) and all("Window input event adaptation failed" in r.get(k, "") for k in ("expected_diagnostic", "capture_rethrow_diagnostic"))


def run(kind, source):
    parser = argparse.ArgumentParser(description="Run real native SDL probes; synthetic input is not human hardware acceptance.")
    option(parser, "Executable", required=True)
    option(parser, "OutputDirectory", required=True)
    if kind in {"sdl3", "platform"}:
        option(parser, "Vulkan", action="store_true")
    if kind == "input":
        option(parser, "Repeat", type=integer(1, 10), default=1)
    if kind == "allocation":
        option(parser, "TimeoutSeconds", type=integer(1, 120), default=30)
    args = parser.parse_args()
    executable = file_path(args.Executable)
    inputs = [identity(executable)]
    imports = None
    fixture = None
    if kind in {"platform", "input", "allocation"}:
        probe_sources = ("input_tests.cpp", "input_test_events.h") if kind == "input" else ()
        imports = platform_imports(executable, source, probe_sources)
        inputs.extend(identity(ROOT / "game/modules/platform/src" / name) for name in ("window.cpp", "input_state.h"))
    if kind == "platform":
        fixture = file_path(executable.parent / "symocraft_platform_failure_fixture.dll")
    output = new_directory(workspace_path(args.OutputDirectory, ROOT / "out"))
    results = []
    summary = {"executable": str(executable), "executable_sha256": inputs[0]["sha256"], "cases": results, "passed": False,
               "verification_sources": evidence_sources([], [Path(source) / ("verify-probes.py" if kind in {"platform", "sdl3"} else "verify-" + kind + ".py"), Path(__file__)])}
    if imports:
        summary["production_imports"] = imports
    try:
        if kind in {"sdl3", "platform"}:
            if fixture:
                summary["fixture_sha256"] = sha256(fixture)
            for case in capability_cases(args.Vulkan, fixture):
                name, mode, expected, diagnostic, env, extra, fault = case
                environment = clean_environment()
                environment.update(env)
                record, errors = json_probe(executable, ["--mode", mode, "--frames", "8", *extra], output, name, env=environment)
                record.update(case=name, expected_exit_code=expected, passed=capability_valid(record, errors, case, fixture is not None))
                if fixture:
                    record["restricted_fault_injection"] = fault
                results.append(record)
            summary["scope"] = "Real SDL window/API probes; restricted fixtures are labelled. Not physical-input, gameplay, or backend acceptance."
        elif kind == "input":
            for iteration in range(1, args.Repeat + 1):
                for case in ("queue", "wait-input", "native-keyboard", "focus", "cursor-resize", "benchmark", "wait-close"):
                    record, errors = json_probe(executable, ["--case", case], output, f"{case}-{iteration:03}", env=clean_environment())
                    passed, partial = input_valid(record, errors)
                    record.update(case=case, iteration=iteration, passed=passed, partial=partial)
                    results.append(record)
            summary["partial_cases"] = sum(r["partial"] for r in results)
            summary["failed_cases"] = sum(not r["partial"] and not r["passed"] for r in results)
            summary["status"] = "fail" if summary["failed_cases"] else "partial" if summary["partial_cases"] else "pass"
            summary["scope"] = "Actual imported production Window; queued SDL and owned Win32 messages are synthetic, not physical hardware acceptance."
            summary["unverified"] = "Held physical buttons/motion, Raw Input, alternate layouts, acceleration/feel, DPI, other machines."
        elif kind == "allocation":
            for case in ("window-object", "input-reserve", "event-growth-poll", "event-growth-wait"):
                record, errors = json_probe(executable, ["--case", case], output, case, args.TimeoutSeconds, clean_environment())
                record.update(case=case, expected_exit_code=0, passed=allocation_valid(record, errors, case))
                results.append(record)
            summary.update(probe_source_sha256=sha256(Path(source) / "allocation_tests.cpp"),
                           verification_script_sha256=sha256(Path(source) / "verify-allocation.py"),
                           production_imports_sha256=sha256(executable.parent / "production-imports.json"),
                           nonempty_snapshot_preservation_proof_complete=all(r["report"] and r["report"].get("previous_nonempty_snapshot_observed") is True for r in results if r["case"].startswith("event-growth")),
                           timeout_seconds=args.TimeoutSeconds,
                           scope="Actual production archives; probe-only C++ allocation failures and queued reducer faults; passing faults exit 0.",
                           unverified="OS/SDL exhaustion, physical input, actual driver failures.")
        else:
            record = capture(executable, [], ROOT, output / "stdout.log", output / "stderr.log", env=clean_environment(), timeout=30)
            text = read_log(output / "stdout.log")
            match = re.search(r"fixture: (\d+) checks passed", text)
            summary.update(created_utc=utc_now(), exit_code=record["exit_code"], timed_out=record["timed_out"], checks=int(match[1]) if match else 0,
                           synthetic_own_window_messages=True, hardware_validation=False)
            summary["passed"] = record["exit_code"] == 0 and not record["timed_out"] and summary["checks"] > 0
        for entry in inputs:
            assert_hash(entry["path"], entry["sha256"])
        summary["source_and_executable_identity_unchanged"] = True
        if kind != "runtime-input":
            summary["passed"] = bool(results) and all(r["passed"] for r in results)
    except (OSError, ValueError, RuntimeError) as error:
        summary["failure"] = str(error)
        raise
    finally:
        write_json(output / ("result.json" if kind == "runtime-input" else "summary.json"), summary)
    require(summary["passed"], f"Native SDL verification did not fully pass; see {output}")
    print(f"Native SDL verification passed: {output}")
