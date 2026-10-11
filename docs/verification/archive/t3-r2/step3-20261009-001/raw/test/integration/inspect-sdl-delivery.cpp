#include <yaml-cpp/yaml.h>
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
YAML::Node Read(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path) || std::filesystem::file_size(path) > 4 * 1024 * 1024)
        throw std::runtime_error("Required bounded YAML artifact is missing");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open protocol artifact");
    return YAML::Load(input);
}
bool Reason(const YAML::Node& summary, const char* reason) {
    if (!summary["invalid_reasons"].IsSequence()) throw std::runtime_error("Invalid reasons must be a sequence");
    for (const auto& item : summary["invalid_reasons"])
        if (item.as<std::string>() == reason) return true;
    return false;
}
YAML::Node ReadLive(const std::filesystem::path& path) {
    const auto handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open shared live protocol snapshot");
    struct Close { HANDLE value; ~Close() { CloseHandle(value); } } close{handle};
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(handle, &size) || size.QuadPart < 0 || size.QuadPart > 4 * 1024 * 1024)
        throw std::runtime_error("Invalid live protocol snapshot size");
    std::string contents(static_cast<std::size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    if (!ReadFile(handle, contents.data(), static_cast<DWORD>(contents.size()), &read, nullptr) || read != contents.size())
        throw std::runtime_error("Cannot read complete live protocol snapshot");
    return YAML::Load(contents);
}
}

int wmain(int argc, wchar_t** argv) {
    try {
        if (argc != 3) throw std::runtime_error("Expected --runner SESSION_DIRECTORY or --live-status STATUS_FILE");
        if (std::wstring(argv[1]) == L"--live-status") {
            const auto status = ReadLive(std::filesystem::path(argv[2]));
            const auto phase = status["phase"].as<std::string>();
            const bool warmup = phase == "warmup";
            const bool sampling = phase == "sampling";
            const auto elapsed = status["elapsed_seconds"].as<double>();
            const bool protocol = status["protocol_version"].as<int>() == 2 && elapsed >= 0;
            std::cout << std::boolalpha << "{\"protocol\":" << protocol << ",\"live_phase\":" << (warmup || sampling)
                << ",\"warmup\":" << warmup << ",\"sampling\":" << sampling << ",\"elapsed_seconds\":" << elapsed << "}\n";
            return protocol ? 0 : 1;
        }
        if (std::wstring(argv[1]) != L"--runner") throw std::runtime_error("Unknown inspection mode");
        const std::filesystem::path directory(argv[2]);
        const auto session = Read(directory / "session.yaml");
        const auto result = Read(directory / "static-1-attempt-1/result.yaml");
        const auto summary = Read(directory / "static-1-attempt-1/capture/summary.yaml");
        const auto status = Read(directory / "static-1-attempt-1/capture/status.yaml");
        const auto metadata = summary["metadata"];
        const bool session_protocol = session["runner_schema"].as<int>() == 1 && session["protocol_version"].as<int>() == 2 &&
            session["settings"]["profile"].as<std::string>() == "quick-60s" &&
            session["settings"]["focus_policy"].as<std::string>() == "allow-unfocused";
        const bool session_cancelled = session["cancelled"].as<bool>() && !session["completed"].as<bool>() &&
            session["completed_rounds"].as<unsigned>() == 0 && !session["continuous_full_protocol"].as<bool>();
        const auto rounds = session["rounds"];
        bool later_rounds_not_started = rounds.IsSequence() && rounds.size() == 3;
        if (later_rounds_not_started) {
            later_rounds_not_started = rounds[0]["scene"].as<std::string>() == "static" &&
                rounds[0]["attempts"].as<unsigned>() == 1 && rounds[0]["state"].as<std::string>() == "cancelled" &&
                !rounds[0]["passed"].as<bool>() && rounds[1]["attempts"].as<unsigned>() == 0 &&
                rounds[2]["attempts"].as<unsigned>() == 0;
        }
        const bool result_cancelled = result["runner_schema"].as<int>() == 1 && result["cancelled"].as<bool>() &&
            !result["passed"].as<bool>() && !result["timed_out"].as<bool>() && !result["forced_termination"].as<bool>() &&
            result["exit_code"].as<unsigned>() == 4;
        const bool game_protocol = metadata["schema_version"].as<int>() == 2 && metadata["protocol_version"].as<int>() == 2 &&
            metadata["workload_version"].as<int>() == 2 && metadata["benchmark"].as<std::string>() == "static" &&
            metadata["focus_policy"].as<std::string>() == "allow-unfocused" &&
            metadata["warmup_seconds"].as<unsigned>() == 10 && metadata["sample_seconds"].as<unsigned>() == 10 &&
            metadata["framebuffer_width"].as<int>() == 1920 && metadata["framebuffer_height"].as<int>() == 1080 &&
            metadata["msaa_samples"].as<int>() == 4 && !metadata["requested_vsync"].as<bool>() &&
            !metadata["gl_renderer"].as<std::string>().empty() && !metadata["gl_version"].as<std::string>().empty() &&
            status["protocol_version"].as<int>() == 2 && status["phase"].as<std::string>() == "finished";
        const bool incomplete = !summary["completed"].as<bool>() && !summary["valid_run"].as<bool>() &&
            Reason(summary, "duration-not-completed") && !Reason(summary, "runtime-error") &&
            summary["total_frames"].as<unsigned>() > 0;
        bool artifacts = true;
        for (const auto name : {"frames.csv", "memory.csv", "focus.csv"}) {
            const auto path = directory / "static-1-attempt-1/capture" / name;
            artifacts = artifacts && std::filesystem::is_regular_file(path) && std::filesystem::file_size(path) > 0;
        }
        const auto screenshot = directory / "static-1-attempt-1/capture/final-frame.png";
        const bool screenshot_present = std::filesystem::is_regular_file(screenshot) && std::filesystem::file_size(screenshot) > 0;
        const bool protocol = session_protocol && session_cancelled && later_rounds_not_started && result_cancelled &&
            game_protocol && incomplete && artifacts;
        std::cout << std::boolalpha << "{\"protocol\":" << protocol
            << ",\"session_protocol\":" << session_protocol << ",\"session_cancelled\":" << session_cancelled
            << ",\"later_rounds_not_started\":" << later_rounds_not_started << ",\"result_cancelled\":" << result_cancelled
            << ",\"game_protocol\":" << game_protocol << ",\"incomplete_with_cleanup\":" << incomplete
            << ",\"artifacts_retained\":" << artifacts << ",\"screenshot_present\":" << screenshot_present
            << ",\"game_exit_code\":" << result["exit_code"].as<unsigned>()
            << ",\"forced_termination\":" << result["forced_termination"].as<bool>()
            << ",\"total_frames\":" << summary["total_frames"].as<unsigned>() << "}\n";
        return protocol ? 0 : 1;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
