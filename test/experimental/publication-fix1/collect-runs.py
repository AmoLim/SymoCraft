"""Collect independent native publication diagnostics without retries."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import uuid
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
import re
from verification_common import ROOT, assert_hash, capture, cli, file_path, files, identity, new_directory, option, read_log, reject_links, require, sha256, snapshot_identity, utc_now, write_json
from scripts_common import integer


def numeric_code(value):
    return type(value) in (int, float) and 0 <= value <= 0xffffffff and value == int(value)


def source_identity():
    paths = ["game/modules/foundation/src/files.cpp", "game/modules/foundation/include/symocraft/foundation/files.h",
             "game/modules/telemetry/src/performance.cpp", "game/modules/telemetry/include/symocraft/telemetry/performance.h",
             "test/unit/foundation/files_tests.cpp", "test/unit/foundation/CMakeLists.txt", "test/unit/telemetry/performance_tests.cpp",
             "test/unit/telemetry/CMakeLists.txt", "test/support/publication_diagnostics.h",
             "test/experimental/publication-fix1/collect-runs.py", "scripts/verification_common.py", "scripts/scripts_common.py"]
    records = snapshot_identity(paths)
    canonical = "\n".join(x["path"] + ":" + x["sha256"] for x in records)
    return {"method": "sorted-relative-path:sha256, UTF-8, LF, no-final-LF", "sha256": hashlib.sha256(canonical.encode("utf-8")).hexdigest(), "files": records}


def scene(root):
    result = []
    pending = [root]
    while pending:
        for path in sorted(pending.pop().iterdir()):
            record = {"path": path.relative_to(root).as_posix(), "bytes": None, "sha256": None, "query_error": None}
            try:
                stat = path.lstat()
                link = path.is_symlink() or bool(getattr(stat, "st_file_attributes", 0) & 0x400)
                record.update(is_directory=path.is_dir(), attributes=getattr(stat, "st_file_attributes", stat.st_mode), reparse=link)
                if not link:
                    if path.is_dir():
                        pending.append(path)
                    else:
                        record.update(bytes=stat.st_size, sha256=sha256(path))
            except OSError as error:
                record["query_error"] = str(error)
            result.append(record)
    return result


def read_diagnostics(stdout, stderr, suite, run_id, pid, enabled, source_sha, exe_sha, configuration):
    tests, apis, other, problems = [], [], [], []
    for stream, text in (("stdout", stdout), ("stderr", stderr)):
        current = {}
        for number, line in enumerate(text.splitlines(), 1):
            if not line.strip().startswith("{"):
                continue
            try:
                value = json.loads(line)
                require(isinstance(value, dict), "Diagnostic must be an object.")
            except ValueError:
                problems.append(f"Invalid JSON at {stream}:{number}")
                continue
            envelope = {"stream": stream, "line": number, "record": value}
            kind = value.get("kind")
            key = (value.get("pid"), value.get("tid"))
            if kind == "publication_test":
                for required in ("suite", "run_id", "pid", "tid", "event", "phase", "expected", "outcome", "utc_filetime"):
                    if value.get(required) is None:
                        problems.append(f"Missing publication_test.{required} at {stream}:{number}")
                for field, expected in {"suite": suite, "run_id": run_id, "pid": pid, "config": configuration, "source_sha": source_sha, "exe_sha": exe_sha}.items():
                    if value.get(field) != expected:
                        problems.append(f"Publication {field} differs from invocation.")
                if value.get("failure_class") == "win32_system_error" or value.get("error_category") == "system" or value.get("event") == "expected_rejection":
                    if not numeric_code(value.get("numeric_win32")) or not value.get("error_category"):
                        problems.append(f"Invalid raw Win32 code/category at {stream}:{number}")
                if value.get("event") == "begin":
                    current[key] = value
                tests.append(envelope)
                if value.get("event") in {"success", "expected_rejection", "expected_standard_rejection", "unexpected_failure"}:
                    current.pop(key, None)
            elif kind == "publish_api":
                for required in ("api", "flags", "win32", "success", "pid", "tid", "utc_filetime", "sequence"):
                    if value.get(required) is None:
                        problems.append(f"Missing publish_api.{required} at {stream}:{number}")
                if not numeric_code(value.get("win32")) or type(value.get("success")) is not bool or value["success"] != (value.get("win32") == 0):
                    problems.append(f"API success/code disagreement at {stream}:{number}")
                if value.get("pid") != pid or value.get("api") not in {"ReplaceFileW", "MoveFileExW"}:
                    problems.append("Unexpected API identity.")
                envelope.update(context=current.get(key), context_association="same-stream preceding begin, same PID/TID" if key in current else "unassociated")
                if key not in current:
                    problems.append(f"API has no active same-stream context at {stream}:{number}")
                apis.append(envelope)
            else:
                if kind == "diagnostic_failure":
                    problems.append("Test reported diagnostic capture failure.")
                other.append(envelope)
    events = lambda name: [x for x in tests if x["record"].get("event") == name]
    completions = events("suite_success")
    failures = events("unexpected_failure") + events("outer_failure")
    if not tests:
        problems.append("No structured test diagnostics; empty stderr is not an error code.")
    if not failures and len(completions) != 1:
        problems.append("Successful process must emit exactly one suite_success.")
    if enabled and not apis:
        problems.append("API diagnostics enabled but no actual API records.")
    if not enabled and apis:
        problems.append("API records in explicitly untraced run.")
    counters = {"operation_events": len(events("begin")), "expected_rejections": len(events("expected_rejection")), "expected_standard_rejections": len(events("expected_standard_rejection"))}
    if len(completions) == 1:
        for name, count in counters.items():
            if completions[0]["record"].get(name) != count:
                problems.append("Suite completion counter disagreement: " + name)
    return {"status": "complete" if not problems else "incomplete", "problems": problems, "test_events": tests, "api_events": apis,
            "other_json": other, "unexpected_events": len(failures), "completed_suites": len(completions),
            "api_calls": len(apis) if enabled else None, "api_calls_observed": len(apis), "api_successes": sum(x["record"].get("success") is True for x in apis),
            "test_operation_successes": len(events("success")), **counters}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("BuildDir", "Config", "Output"):
        option(parser, name, required=True, **({"choices": ["Debug", "Release"]} if name == "Config" else {}))
    option(parser, "Test", choices=["foundation.files", "performance.export", "both"], default="both")
    option(parser, "FilesExe")
    option(parser, "PerfExe")
    option(parser, "RunCount", type=integer(1, 20), default=20)
    option(parser, "TimeoutSeconds", type=integer(1, 3600), default=60)
    for name in ("ApiDiagnostics", "NoApiDiagnostics", "StopOnUnexpectedFailure"):
        option(parser, name, action="store_true")
    args = parser.parse_args()
    require(not (args.ApiDiagnostics and args.NoApiDiagnostics), "Conflicting API diagnostics flags.")
    enabled = not args.NoApiDiagnostics
    build = Path(args.BuildDir).resolve(strict=True)
    reject_links(args.Output)
    selected = []
    for suite, value in (("foundation.files", args.FilesExe), ("performance.export", args.PerfExe)):
        if args.Test not in ("both", suite):
            continue
        require(value, "Provide the exact executable for " + suite)
        selected.append({"suite": suite, **identity(file_path(value))})
    source = source_identity()
    libraries = [identity(p) for p in files(build) if p.suffix == ".lib" and re.search("foundation|telemetry", p.stem)]
    output = new_directory(args.Output)
    manifest = {"schema_version": 1, "purpose": "Fix1 bounded independent diagnostic batch, not V05 final acceptance", "created_utc": utc_now(),
                "repository": str(ROOT), "output": str(output), "build_directory": str(build), "configuration": args.Config, "suites": selected,
                "planned_processes_per_suite": args.RunCount, "planned_processes": args.RunCount * len(selected), "source_identity": source,
                "library_inputs": libraries, "cmake_cache_sha256": sha256(build / "CMakeCache.txt") if (build / "CMakeCache.txt").exists() else None,
                "collector": {"path": str(Path(__file__)), "sha256": sha256(__file__), "python": sys.version, "process_model": "one child per case parent; raw stdout/stderr files"},
                "api_diagnostics": enabled, "timeout_seconds": args.TimeoutSeconds, "stop_on_unexpected_failure": args.StopOnUnexpectedFailure,
                "external_trace": "not started by collector", "parent_environment_mutated": False, "retries": False, "sleep": False}
    write_json(output / "manifest.json", manifest)
    results, errors = [], []
    batch = uuid.uuid4().hex
    for selection in selected:
        suite, exe, digest = selection["suite"], selection["path"], selection["sha256"]
        for index in range(1, args.RunCount + 1):
            name = suite.replace(".", "-") + f"-{index:03}"
            directory = new_directory(output / name)
            parent = new_directory(directory / "case-parent")
            run_id = batch + "-" + name
            overrides = {"SYMOCRAFT_FIX1_RUN_ID": run_id, "SYMOCRAFT_FIX1_CONFIG": args.Config, "SYMOCRAFT_FIX1_SOURCE_SHA": source["sha256"],
                         "SYMOCRAFT_FIX1_EXE_SHA": digest, "SYMOCRAFT_PUBLISH_DIAGNOSTICS": "1" if enabled else "0"}
            env = dict(os.environ)
            env.update(overrides)
            record = {"run_id": run_id, "suite": suite, "configuration": args.Config, "started_utc": utc_now(), "launched": False,
                      "executable": exe, "executable_sha256": digest, "source_sha256": source["sha256"], "working_directory": str(build), "arguments": [str(parent)],
                      "child_environment_overrides": overrides, "timeout_seconds": args.TimeoutSeconds, "retry_count": 0, "sleeps": 0, "procmon_enabled_by_collector": False,
                      "collector_errors": [], "passed": False, "exit_code": None, "timeout": False}
            try:
                actual = capture(exe, [parent], build, directory / "stdout.log", directory / "stderr.log", args.TimeoutSeconds, env)
                record.update(actual, launched=True, timeout=actual["timed_out"], elapsed_ms=actual["elapsed_seconds"] * 1000)
                diagnostics = read_diagnostics(read_log(directory / "stdout.log"), read_log(directory / "stderr.log"), suite, run_id, actual["pid"], enabled, source["sha256"], digest, args.Config)
                record.update(diagnostics=diagnostics, diagnostic_status=diagnostics["status"])
                assert_hash(exe, digest)
                record["product_status"] = "not_completed" if actual["timed_out"] else "fail" if actual["exit_code"] or diagnostics["unexpected_events"] else "pass"
            except (OSError, ValueError) as error:
                record["collector_errors"].append(str(error))
                record.setdefault("product_status", "not_completed")
                record.setdefault("diagnostic_status", "incomplete")
            record.update(collector_status="fail" if record["collector_errors"] else "pass", ended_utc=utc_now(), original_case_parent=str(parent), scene=scene(parent))
            for stream in ("stdout", "stderr"):
                record[stream + "_sha256"] = sha256(directory / (stream + ".log")) if (directory / (stream + ".log")).exists() else None
            record["passed"] = record["product_status"] == "pass" and record["diagnostic_status"] == "complete" and not record["collector_errors"]
            write_json(directory / "result.json", record)
            with (output / "runs.jsonl").open("a", encoding="utf-8") as stream:
                stream.write(json.dumps(record, ensure_ascii=False, allow_nan=False) + "\n")
            results.append(record)
            if record["collector_errors"] or (args.StopOnUnexpectedFailure and not record["passed"]):
                break
    unchanged = False
    try:
        unchanged = source_identity()["sha256"] == source["sha256"]
        require(unchanged, "Source identity changed during batch.")
        for entry in libraries:
            assert_hash(entry["path"], entry["sha256"])
    except (OSError, ValueError) as error:
        errors.append(str(error))
    suites = []
    for selection in selected:
        runs = [r for r in results if r["suite"] == selection["suite"]]
        row = {"suite": selection["suite"], "planned": args.RunCount, "actual": len(runs)}
        row.update({"product_" + state: sum(r["product_status"] == state for r in runs) for state in ("pass", "fail")})
        row.update(not_completed=sum(r["product_status"] == "not_completed" for r in runs), diagnostic_incomplete=sum(r["diagnostic_status"] != "complete" for r in runs), collector_failure=sum(r["collector_status"] != "pass" for r in runs))
        for key in ("api_calls", "api_calls_observed", "expected_rejections", "expected_standard_rejections", "operation_events"):
            row[key] = None if key == "api_calls" and not enabled else sum((r.get("diagnostics", {}).get(key) or 0) for r in runs)
        suites.append(row)
    summary = {"schema_version": 1, "ended_utc": utc_now(), "manifest": "manifest.json", "run_records": "runs.jsonl", "planned_processes": manifest["planned_processes"],
               "actual_processes": sum(r["launched"] for r in results), "recorded_attempts": len(results), "suites": suites, "collector_errors": errors,
               "source_identity_unchanged": unchanged, "early_stop": len(results) != manifest["planned_processes"],
               "passed": not errors and len(results) == manifest["planned_processes"] and all(r["passed"] for r in results),
               "root_cause_confirmed": False, "fix_confirmed": False, "v05_acceptance": False, "issue_closed": False,
               "interpretation": "Expected API rejections are not product failures; no localized message is parsed for an error code; raw cases retained."}
    write_json(output / "summary.json", summary)
    require(summary["passed"], f"Publication collection did not fully pass; see {output}")


if __name__ == "__main__":
    cli(main)
