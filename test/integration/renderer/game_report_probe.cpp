#include <symocraft/telemetry/document_io.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace {
using SymoCraft::Data::Value;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint64_t Count(const Value& value) {
    if (const auto* number = std::get_if<std::uint64_t>(&value.storage())) return *number;
    if (const auto* number = std::get_if<std::int64_t>(&value.storage()); number && *number >= 0)
        return static_cast<std::uint64_t>(*number);
    throw std::runtime_error("Report counter is missing, negative or not integral");
}

double Duration(const Value& value) {
    Require(!std::holds_alternative<bool>(value.storage()), "Report duration is a boolean");
    const auto result = value.as<double>();
    Require(std::isfinite(result) && result >= 0, "Report duration is negative or nonfinite");
    return result;
}

std::uint64_t Sum(std::uint64_t first, std::uint64_t second) {
    Require(second <= std::numeric_limits<std::uint64_t>::max() - first, "Report counter overflow");
    return first + second;
}

std::string Digest(const Value& value) {
    const auto digest = value.as<std::string>();
    constexpr std::string_view prefix = "fnv1a64:";
    Require(digest.starts_with(prefix) && digest.size() == prefix.size() + 16,
            "World digest does not have the expected fnv1a64: plus 16-digit format");
    for (const auto character : std::string_view(digest).substr(prefix.size()))
        Require((character >= '0' && character <= '9') || (character >= 'a' && character <= 'f'),
                "World digest is not lowercase hexadecimal");
    return digest;
}

void Statistics(const Value& statistics, std::uint64_t count) {
    Require(Count(statistics["count"]) == count, "Report statistics have an unexpected sample count");
    if (count == 0) {
        Require(!statistics["min"] && !statistics["max"] && !statistics["mean"],
                "Empty report statistics fabricate a zero duration");
        return;
    }
    const auto minimum = Duration(statistics["min"]);
    const auto maximum = Duration(statistics["max"]);
    const auto mean = Duration(statistics["mean"]);
    const auto tolerance = std::max(1e-12, maximum * 1e-12);
    Require(minimum <= mean + tolerance && mean <= maximum + tolerance, "Report duration bounds are inconsistent");
    for (const auto* percentile : {"p50", "p95", "p99"}) {
        const auto value = Duration(statistics[percentile]);
        Require(minimum <= value && value <= maximum, "Report percentile is outside its bounds");
    }
}

void Screenshot(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::array<unsigned char, 24> header{};
    Require(static_cast<bool>(file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()))),
            "Final screenshot is missing or truncated");
    constexpr std::array<unsigned char, 8> signature{137, 80, 78, 71, 13, 10, 26, 10};
    Require(std::equal(signature.begin(), signature.end(), header.begin()), "Final screenshot is not a PNG");
    Require(header[12] == 'I' && header[13] == 'H' && header[14] == 'D' && header[15] == 'R',
            "Final screenshot does not begin with IHDR");
    const auto dimension = [&](std::size_t offset) {
        return (static_cast<std::uint32_t>(header[offset]) << 24) |
               (static_cast<std::uint32_t>(header[offset + 1]) << 16) |
               (static_cast<std::uint32_t>(header[offset + 2]) << 8) | header[offset + 3];
    };
    Require(dimension(16) == 1920 && dimension(20) == 1080, "Final screenshot extent is not 1920x1080");
}

void JsonString(std::ostream& output, const std::string& value) {
    constexpr char hex[] = "0123456789abcdef";
    output << '"';
    for (const unsigned char character : value) {
        if (character == '"' || character == '\\') output << '\\' << character;
        else if (character < 32) output << "\\u00" << hex[character >> 4] << hex[character & 15];
        else output << character;
    }
    output << '"';
}

void Json(std::ostream& output, const Value& value) {
    std::visit([&](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) output << "null";
        else if constexpr (std::is_same_v<T, bool>) output << (item ? "true" : "false");
        else if constexpr (std::is_same_v<T, std::string>) JsonString(output, item);
        else if constexpr (std::is_same_v<T, Value::Mapping>) {
            output << '{';
            bool first = true;
            for (const auto& [key, child] : item) {
                if (!first) output << ',';
                first = false;
                JsonString(output, key);
                output << ':';
                Json(output, child);
            }
            output << '}';
        } else if constexpr (std::is_same_v<T, Value::Sequence>) {
            output << '[';
            bool first = true;
            for (const auto& child : item) {
                if (!first) output << ',';
                first = false;
                Json(output, child);
            }
            output << ']';
        } else {
            if constexpr (std::is_floating_point_v<T>) Require(std::isfinite(item), "Nonfinite JSON output");
            output << item;
        }
    }, value.storage());
}
}

int main(int argc, char** argv) {
    try {
        Require(argc == 4, "Usage: game_report_probe REPORT_DIRECTORY D3D12|OpenGL static|walk|edit");
        const auto directory = std::filesystem::absolute(argv[1]);
        const std::string backend = argv[2], workload = argv[3];
        Require(backend == "D3D12" || backend == "OpenGL", "Unsupported expected backend");
        Require(workload == "static" || workload == "walk" || workload == "edit", "Unsupported expected workload");
        const auto report = SymoCraft::Data::LoadYamlFile(directory / "summary.yaml");
        const auto& metadata = report["metadata"];
        Require(report["completed"].as<bool>() && report["valid_run"].as<bool>(), "Game run is incomplete or invalid");
        Require(report["invalid_reasons"].IsSequence() && report["invalid_reasons"].size() == 0,
                "Valid game report retains invalidation reasons");
        Require(Count(metadata["schema_version"]) == 2 && Count(metadata["protocol_version"]) == 2,
                "Benchmark protocol/schema changed unexpectedly");
        Require(metadata["benchmark"].as<std::string>() == workload, "Report workload differs from requested workload");
        Require(!metadata["requested_vsync"].as<bool>(), "Verification benchmark requested VSync");
        Require(Count(metadata["requested_framebuffer_width"]) == 1920 && Count(metadata["requested_framebuffer_height"]) == 1080,
                "Verification benchmark request is not 1920x1080");
        Require(Count(metadata["framebuffer_width"]) == 1920 && Count(metadata["framebuffer_height"]) == 1080,
                "Game framebuffer differs from the requested extent");
        Require(std::filesystem::is_regular_file(directory / "frames.csv") && std::filesystem::file_size(directory / "frames.csv") > 0,
                "Raw game frame data is missing");
        Screenshot(directory / "final-frame.png");
        Require(metadata["screenshot"].as<std::string>().find("after measurement") != std::string::npos,
                "Screenshot measurement exclusion is not recorded");

        const auto total = Count(report["total_frames"]), measured = Count(report["frame_ms"]["count"]);
        const auto gpu = Count(report["gpu_draw_ms"]["count"]), missing_gpu = Count(report["gpu_missing_sample_frames"]);
        Require(measured > 0 && total >= measured, "No measured game frames");
        Require(gpu > 0 && Sum(gpu, missing_gpu) == measured, "GPU valid/missing sample counts are inconsistent");
        Statistics(report["frame_ms"], measured);
        Statistics(report["gpu_draw_ms"], gpu);
        Require(Duration(report["gpu_draw_ms"]["max"]) > 0, "Game has no positive measured GPU draw sample");
        Statistics(report["present_wait_cpu_ms"], measured);

        const auto& world = metadata["world"];
        const auto& generation = world["generation"];
        Require(generation["digest_schema"].as<std::string>() == "fnv1a64-v1/le-coord-i32x2/block-u16x4/chunk-x,z/block-y,x,z",
                "World digest schema differs from the established generation contract");
        const auto radius = Count(generation["radius"]), chunks = Count(world["chunks"]);
        Require(radius == 10 && chunks == 441, "Verification did not use the full default generated world");
        Require(generation["chunk_dimensions"].IsSequence() && generation["chunk_dimensions"].size() == 3
                && Count(generation["chunk_dimensions"][0]) == 16 && Count(generation["chunk_dimensions"][1]) == 256
                && Count(generation["chunk_dimensions"][2]) == 16, "World chunk dimensions changed");
        const auto initial_digest = Digest(world["final_digest"]), final_digest = Digest(metadata["final_world_digest"]);
        const auto edits = Count(report["sample_edits"]), scheduled = Count(metadata["total_scheduled_edits_applied"]);
        if (workload == "edit") {
            Require(edits > 0 && scheduled >= edits, "Edit workload applied no measured edits");
            const auto plan = metadata["edit_plan"].as<std::string>();
            Require(plan.find("forward") != std::string::npos && plan.find("reverse") != std::string::npos,
                    "Edit digest changes/reversals lack a recorded workload plan");
        } else {
            Require(edits == 0 && scheduled == 0 && initial_digest == final_digest,
                    "Static/walk benchmark changed world content");
        }

        Value result;
        result["schema"] = "symocraft.t3.r2-step3-game-probe-v1";
        result["backend"] = backend;
        result["workload"] = workload;
        result["total_frames"] = total;
        result["measured_frames"] = measured;
        result["gpu_valid_samples"] = gpu;
        result["gpu_missing_samples"] = missing_gpu;
        result["world_radius"] = radius;
        result["world_chunks"] = chunks;
        result["world_seed"] = Count(generation["seed"]);
        result["digest_schema"] = generation["digest_schema"];
        result["initial_mesh_chunks"] = Count(metadata["first_mesh_chunks"]);
        result["terrain_digest"] = Digest(world["terrain_digest"]);
        result["scene_digest"] = Digest(world["scene_digest"]);
        result["initial_digest"] = initial_digest;
        result["final_digest"] = final_digest;
        result["digest_changed"] = initial_digest != final_digest;
        result["sample_edits"] = edits;
        result["scheduled_edits"] = scheduled;

        if (backend == "D3D12") {
            const auto& renderer = metadata["renderer"];
            Require(metadata["renderer_backend"].as<std::string>() == "D3D12"
                    && renderer["backend"].as<std::string>() == "D3D12", "Actual backend differs from D3D12");
            Require(!metadata["gl_vendor"] && !metadata["gl_renderer"] && !metadata["gl_version"],
                    "Native report contains legacy GL identity");
            Require(renderer["device_name"].as<std::string>() == "NVIDIA GeForce RTX 5070 Ti", "Unexpected native GPU");
            Require(!renderer["device_identity"].as<std::string>().empty(), "Native device identity is missing");
            Require(renderer["driver_version"].as<std::string>() == "32.0.16.1664", "Native driver differs from verification matrix");
            Require(Count(renderer["actual_samples"]) == 4 && Count(metadata["requested_msaa_samples"]) == 4,
                    "Native requested/actual MSAA differs from 4x");
            Require(Count(renderer["framebuffer_width"]) == 1920 && Count(renderer["framebuffer_height"]) == 1080
                    && renderer["actual_present_mode"].as<std::string>() == "Immediate"
                    && renderer["validation_enabled"].as<bool>(), "Native framebuffer/present/validation configuration differs");
            Require(renderer["framebuffer_format"].as<std::string>() == "R8G8B8A8_UNORM RTV / swapchain storage (no output sRGB encode)",
                    "Native framebuffer format/output encoding changed");
            const auto api = renderer["api_version"].as<std::string>();
            Require(api.find("FL12_0") != std::string::npos && api.find("SM6_0") != std::string::npos,
                    "Native API/capability identity is missing");
            for (const auto* capability : {"texture_arrays", "selection_boxes", "screenshots", "gpu_timestamps"})
                Require(renderer["capabilities"][capability].as<bool>(), "Required native renderer capability is absent");
            const auto camera = metadata["camera_contract"].as<std::string>();
            const auto atlas = metadata["atlas_contract"].as<std::string>();
            Require(camera.find("far 2000") != std::string::npos && camera.find("world-space identity") != std::string::npos,
                    "Actual game camera/world-space contract not recorded");
            Require(atlas.find("512x512 RGBA8") != std::string::npos && atlas.find("64 bottom-row-first") != std::string::npos
                    && atlas.find("V=1-V exactly once") != std::string::npos && atlas.find("Linear") != std::string::npos,
                    "Actual atlas layer/UV/color contract not recorded");
            Require(Count(metadata["startup_upload_bytes"]) > 0 && Count(metadata["startup_upload_calls"]) > 0
                    && Duration(report["startup_ms"]["resource_upload_cpu"]) > 0, "Native startup resource uploads are missing");
            Statistics(report["cpu_prepare_ms"], measured);
            Statistics(report["cpu_submit_ms"], measured);
            Statistics(report["cpu_submit_excluding_upload_ms"], 0);
            std::uint64_t explained_missing = 0;
            if (const auto& reasons = report["gpu_missing_sample_reasons"]; reasons.IsMap())
                for (const auto& [reason, count] : std::get<Value::Mapping>(reasons.storage())) {
                    Require(!reason.empty(), "Missing GPU reason is empty");
                    explained_missing = Sum(explained_missing, Count(count));
                }
            Require(explained_missing == missing_gpu, "Missing native GPU samples lack matching explanations");
            const auto resource_bytes = Count(report["sample_resource_upload_bytes"]);
            const auto render_bytes = Count(report["sample_render_upload_bytes"]);
            const auto calls = Count(report["sample_upload_calls"]);
            Require(Count(report["sample_upload_bytes"]) == Sum(resource_bytes, render_bytes), "Native upload attribution does not sum");
            if (workload == "edit") Require(resource_bytes > 0 && calls > 0, "Edit workload did not update persistent GPU resources");
            else Require(resource_bytes == 0 && calls == 0, "Unchanged world geometry was uploaded again");
            result["device_name"] = renderer["device_name"];
            result["device_identity"] = renderer["device_identity"];
            result["driver_version"] = renderer["driver_version"];
            result["actual_samples"] = Count(renderer["actual_samples"]);
            result["actual_present_mode"] = renderer["actual_present_mode"];
            result["validation_enabled"] = renderer["validation_enabled"];
            result["startup_upload_bytes"] = Count(metadata["startup_upload_bytes"]);
            result["startup_upload_calls"] = Count(metadata["startup_upload_calls"]);
            result["sample_resource_upload_bytes"] = resource_bytes;
            result["sample_render_upload_bytes"] = render_bytes;
            result["sample_upload_calls"] = calls;
        } else {
            const auto name = metadata["gl_renderer"].as<std::string>();
            Require(name.find("NVIDIA GeForce RTX 5070 Ti") != std::string::npos, "Unexpected legacy GPU");
            Require(!metadata["gl_vendor"].as<std::string>().empty() && !metadata["gl_version"].as<std::string>().empty(),
                    "Legacy GL device identity is missing");
            Require(!metadata["renderer"] && !metadata["renderer_backend"], "Modern renderer identity leaked into legacy report");
            Require(Count(metadata["msaa_samples"]) == 4, "Legacy actual MSAA differs from 4x");
            Require(metadata["gpu_time_scope"].as<std::string>().find("GL_TIME_ELAPSED") != std::string::npos,
                    "Legacy GPU timing scope was replaced");
            Statistics(report["cpu_submit_excluding_upload_ms"], measured);
            Require(!report["cpu_prepare_ms"] && !report["cpu_submit_ms"] && !report["sample_resource_upload_bytes"],
                    "Modern telemetry summary shape leaked into legacy report");
            result["device_name"] = name;
            result["actual_samples"] = Count(metadata["msaa_samples"]);
        }
        Json(std::cout, result);
        std::cout << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Game report verification failed: " << error.what() << '\n';
        return 1;
    }
}
