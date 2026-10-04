#pragma once
#include <filesystem>
#include <functional>
#include <stop_token>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace Benchmark {
    namespace fs = std::filesystem;
    struct Config {
        fs::path game, output;
        bool quick = false;
        std::string focus_policy = "allow-unfocused";
        std::string machine_label = "unknown", power_mode = "unknown", gpu_mode = "unknown", clocks = "unknown";
    };
    struct Round { std::string scene; unsigned repeat, warmup, sample; };
    struct Progress {
        unsigned round = 0, total = 0, passed = 0;
        std::string phase = "idle", message;
        double elapsed = 0, duration = 0;
    };
    struct Outcome { bool passed = false, cancelled = false; unsigned completed = 0; fs::path directory; };
    using Observer = std::function<void(const Progress&)>;

    std::vector<Round> Plan(bool quick);
    Outcome Run(const Config& config, std::stop_token stop = {}, const Observer& observer = {}, bool resume = false);
    YAML::Node ValidateCapture(const fs::path& capture, const Round& round, const std::string& focus_policy, unsigned long exit_code);
    fs::path Export(const Config& config, std::stop_token stop = {});
    fs::path ExecutablePath();
    fs::path DefaultOutputParent();
    std::wstring Wide(const std::string& value);
    std::string Utf8(const std::wstring& value);
    std::string Timestamp();
    std::string Sha256(const fs::path& path);
    YAML::Node ReadYaml(const fs::path& path);
    void WriteYaml(const fs::path& path, const YAML::Node& node);
}
