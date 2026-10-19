"""Check real renderer/app/shader dependency ownership without changing contents."""
import argparse
import csv
from pathlib import Path
import re
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import ROOT, cli, file_path, logged, new_directory, option, probe_header, read_log, require, sha256, utc_now, workspace_path, write_json

PATTERNS = {
    "coreRenderer": r"game/modules/renderer/.*CMakeFiles/symocraft_renderer\.dir/",
    "legacyRenderer": r"game/modules/renderer/.*CMakeFiles/symocraft_renderer_legacy_opengl\.dir/",
    "nativeD3D12": r"game/modules/renderer/.*CMakeFiles/symocraft_renderer_d3d12\.dir/",
    "nativeVulkan": r"game/modules/renderer/.*CMakeFiles/symocraft_renderer_vulkan\.dir/",
    "world": "game/modules/world/", "simulation": "game/modules/simulation/", "app": "game/app/",
    "appRenderData": r"game/app/.*CMakeFiles/symocraft_app_render_data\.dir/",
    "appRuntime": r"game/app/.*CMakeFiles/(?:SymoCraft|symocraft_app_runtime|symocraft_application)\.dir/",
    "appCacheTests": r"CMakeFiles/app_render_cache_tests\.dir/",
    "appConversionTests": r"CMakeFiles/app_render_conversion_tests\.dir/", "benchmark": "tools/benchmark/",
    "publicConsumer": r"CMakeFiles/renderer_public_api_consumer\.dir/", "facadeTests": r"CMakeFiles/renderer_facade_tests\.dir/",
}


def compiled_owners(log):
    objects = re.findall(r"Building CXX object ([^\r\n]+)", log.replace("\\", "/"), re.I)
    return {name: sum(bool(re.search(pattern, obj)) for obj in objects) for name, pattern in PATTERNS.items()}


def app_sources(log):
    return [owner + ":" + source for owner, source in re.findall(r"Building CXX object [^\r\n]*game/app/CMakeFiles/(symocraft_app_render_data|symocraft_app_runtime|symocraft_application|SymoCraft)\.dir/src/([^/]+)\.obj", log.replace("\\", "/"))]


def exact(actual, expected):
    return len(actual) == len(expected) and set(actual) == set(expected)


def production_target(targets, sources, name, kind, expected):
    types = [x for x in targets if x["target"] == name and x["property"] == "TYPE"]
    production = [x for x in sources if x["target"] == name]
    if not types and not production:
        return False
    require(len(types) == 1 and types[0]["type"] == kind and types[0]["raw_value"] == kind, f"Invalid production declaration: {name}")
    actual = [x["file"].replace("\\", "/") for x in production if x["file"].endswith(".cpp")]
    require(exact(actual, expected) and all(x["type"] == kind for x in production), f"Production source ownership changed: {name}")
    declared = [x for x in targets if x["target"] == name and x["property"] == "SOURCES"]
    require(len(declared) == 1, f"Missing target sources: {name}")
    require(exact([Path(x).name for x in declared[0]["raw_value"].replace("\\", "/").split(";") if x.endswith(".cpp")],
                  [Path(x).name for x in expected]), f"Target sources disagree with ownership: {name}")
    return True


def assert_app(counts, actual, expected):
    require(counts["app"] == len(expected) and counts["appRenderData"] == sum(x.startswith("symocraft_app_render_data:") for x in expected)
            and counts["appRuntime"] == sum(x.startswith(("SymoCraft:", "symocraft_app_runtime:", "symocraft_application:")) for x in expected) and exact(actual, expected),
            f"Header rebuilt the wrong app sources: {actual}; owners={counts}")


def shader_counts(log):
    normalized = log.replace("\\", "/")
    return {"dxil": len(re.findall(r"Compiling D3D12 (?:blocks|selection)\.(?:vs|ps) Shader Model 6\.0", normalized)),
            "spirv": len(re.findall(r"Generating [^\r\n]*shaders/vulkan/(?:blocks|selection)\.(?:vs|ps)\.spv", normalized))}


def check_probe(name, log, counts, actual_app, public_app, cache_app, has_cache_tests, expected_shaders=None):
    all_count = len(re.findall(r"Building (?:C|CXX) object", log))
    shaders = shader_counts(log)
    def zero(*names):
        return all(counts[x] == 0 for x in names)
    shader_probe = name in ("private-d3d12-shader", "shared-shader-constants", "shared-blocks-shader", "shared-selection-shader")
    if shader_probe:
        require(shaders == (expected_shaders or {"dxil": 4, "spirv": 0}) and all_count == 0,
                f"Wrong shader regeneration or unrelated C++ compilation: {shaders}")
    elif name == "private-app-world-cache":
        assert_app(counts, actual_app, cache_app)
        expected_tests = int(has_cache_tests)
        require(counts["appCacheTests"] == expected_tests and zero("appConversionTests", "coreRenderer", "legacyRenderer", "nativeD3D12", "nativeVulkan", "publicConsumer", "facadeTests", "world", "simulation", "benchmark")
                and not any(shaders.values()) and all_count == len(cache_app) + expected_tests, f"Private app cache rebuilt wrong owners: {counts}")
    elif name == "private-d3d12-pipeline":
        require(counts["nativeD3D12"] == 2 and all_count == 2 and zero("coreRenderer", "nativeVulkan", "legacyRenderer", "publicConsumer", "facadeTests", "world", "simulation", "app", "benchmark", "appCacheTests", "appConversionTests"),
                f"Private D3D12 header rebuilt wrong owners: {counts}")
    elif name == "private-vulkan-header":
        # The private factory and the one Vulkan implementation own this declaration.
        require(counts["nativeVulkan"] == 1 and counts["coreRenderer"] == 1 and all_count == 2
                and zero("nativeD3D12", "legacyRenderer", "publicConsumer", "facadeTests", "world", "simulation", "app", "benchmark", "appCacheTests", "appConversionTests"),
                f"Private Vulkan header rebuilt wrong owners: {counts}")
    elif name == "private-backend-contract":
        require(counts["coreRenderer"] > 0 and zero("legacyRenderer", "publicConsumer", "world", "simulation", "app", "benchmark", "appCacheTests", "appConversionTests"),
                f"Private backend header rebuilt wrong owners: {counts}")
    else:
        assert_app(counts, actual_app, public_app)
        require(counts["coreRenderer"] > 0 and counts["publicConsumer"] > 0 and zero("legacyRenderer", "world", "simulation", "benchmark"), f"Public renderer types rebuilt wrong owners: {counts}")
    relinked = bool(re.search(r"Linking CXX executable [^\r\n]*renderer_public_api_consumer(?:\.exe)?(?:\s|$)", log))
    require(shader_probe or name == "private-app-world-cache" or relinked, "Renderer implementation did not relink its public API consumer.")
    return shaders, relinked


def verify(args):
    build = workspace_path(args.BuildDirectory, ROOT / "out")
    require((build / "CMakeCache.txt").is_file(), "BuildDirectory must contain a configured, settled CMake build.")
    cmake = file_path(args.CMakePath)
    output = new_directory(workspace_path(args.OutputDirectory, ROOT / "out"))
    result = {"schema": "symocraft.renderer-incremental-build", "version": 4, "started_utc": utc_now(), "finished_utc": None,
              "build_directory": str(build), "output_directory": str(output), "status": "running", "baseline_compilations": None,
              "production_callers": None, "probes": [], "error": None}
    def run(name):
        return logged(cmake, ["--build", build, "--parallel", "8"], output / (name + ".log"))
    def tsv(name):
        with (build / name).open(encoding="utf-8-sig", newline="") as source:
            return list(csv.DictReader(source, delimiter="\t"))
    try:
        baseline = run("baseline")
        result["baseline_compilations"] = len(re.findall(r"Building (?:C|CXX) object", baseline, re.I))
        require(result["baseline_compilations"] == 0, "The initial build was not settled. Rerun after production changes finish.")
        targets, sources = tsv("renderer-targets.tsv"), tsv("production-sources.tsv")
        data_sources = ["game/app/src/world_render_cache.cpp", "game/app/src/render_conversion.cpp"]
        if any(x["target"] == "symocraft_app_render_data" and x["file"] == "game/app/src/benchmark_scenario.cpp" for x in sources):
            data_sources.append("game/app/src/benchmark_scenario.cpp")
        data = production_target(targets, sources, "symocraft_app_render_data", "STATIC_LIBRARY", data_sources)
        runtime_name = "symocraft_application" if any(x["target"] == "symocraft_application" for x in targets) else "symocraft_app_runtime"
        runtime = production_target(targets, sources, runtime_name, "STATIC_LIBRARY", ["game/app/src/application.cpp", "game/app/src/game_renderer.cpp", "game/app/src/game_entry.cpp"])
        entry = production_target(targets, sources, "SymoCraft", "EXECUTABLE", ["game/app/src/main.cpp"])
        require(entry == runtime, "Executable and production runtime ownership disagree.")
        require(not runtime or data, "Game runtime requires its real app render data target.")
        cache_tests = sum(x["target"] == "app_render_cache_tests" and x["property"] == "TYPE" and x["type"] == "EXECUTABLE" and x["raw_value"] == "EXECUTABLE" for x in targets) == 1
        result["production_callers"] = {"app_render_data": data, "game_runtime": runtime, "app_cache_tests": cache_tests,
            "targets_sha256": sha256(build / "renderer-targets.tsv").upper(), "sources_sha256": sha256(build / "production-sources.tsv").upper()}
        public_app = (["symocraft_app_render_data:world_render_cache.cpp", "symocraft_app_render_data:render_conversion.cpp"] if data else []) + ([runtime_name + ":application.cpp", runtime_name + ":game_renderer.cpp"] if runtime else [])
        cache_app = (["symocraft_app_render_data:world_render_cache.cpp"] if data else []) + ([runtime_name + ":game_renderer.cpp"] if runtime else [])
        probes = [("private-backend-contract", "game/modules/renderer/src/backend/render_backend.h"), ("public-renderer-types", "game/modules/renderer/include/symocraft/renderer/types.h")]
        if data:
            probes.append(("private-app-world-cache", "game/app/src/world_render_cache.h"))
        graph = read_log(build / "build.ninja")
        direct, vulkan = "symocraft_renderer_d3d12.dir" in graph, "symocraft_renderer_vulkan.dir" in graph
        if direct:
            probes.append(("private-d3d12-pipeline", "game/modules/renderer/src/backend/d3d12/pipeline.h"))
        if vulkan:
            probes.append(("private-vulkan-header", "game/modules/renderer/src/backend/vulkan/vulkan_backend.h"))
        if direct or vulkan:
            probes += [("shared-shader-constants", "assets/shaders/hlsl/draw_constants.hlsli"),
                       ("shared-blocks-shader", "assets/shaders/hlsl/blocks.hlsl"),
                       ("shared-selection-shader", "assets/shaders/hlsl/selection.hlsl")]
        for name, relative in probes:
            record = {"probe": name, "header": relative, "status": "running", "compiled_owners": None, "compiled_app_sources": [], "compiled_shaders": 0, "relinked_public_consumer": False}
            result["probes"].append(record)
            try:
                with probe_header(ROOT / relative) as timestamp:
                    record["timestamp"] = timestamp
                    log = run(name)
                    counts, actual = compiled_owners(log), app_sources(log)
                    record["compiled_owners"], record["compiled_app_sources"] = counts, actual
                    expected = {"dxil": (4 if name == "shared-shader-constants" else 2) if direct else 0,
                                "spirv": (4 if name == "shared-shader-constants" else 2) if vulkan else 0}
                    record["compiled_shaders"], record["relinked_public_consumer"] = check_probe(name, log, counts, actual, public_app, cache_app, cache_tests, expected)
                    record["status"] = "passed"
            except BaseException:
                record["status"] = "failed"
                raise
            finally:
                record.update(record.pop("timestamp", {}))
        result["status"] = "passed"
    except BaseException as error:
        result["status"], result["error"] = "failed", str(error)
        raise
    finally:
        result["finished_utc"] = utc_now()
        write_json(output / "result.json", result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("BuildDirectory", "OutputDirectory", "CMakePath"):
        option(parser, name, required=True)
    print(verify(parser.parse_args()))


if __name__ == "__main__":
    cli(main)
