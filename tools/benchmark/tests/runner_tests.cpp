#include "platform.h"
#include <fstream>
#include <iostream>
#include <thread>

namespace {
    void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
    template<class F> void Reject(F&& action) {
        try { action(); } catch (const std::exception&) { return; }
        throw std::runtime_error("Invalid operation accepted");
    }
    std::string Read(const Benchmark::fs::path& path) { std::ifstream input(path, std::ios::binary); return {(std::istreambuf_iterator<char>(input)), {}}; }
}
int wmain(int argc, wchar_t** argv)
{
    using namespace Benchmark;
    try {
        Require(argc == 4, "Missing test arguments");
        const std::wstring suite(argv[1]);
        const auto parent = fs::absolute(argv[3]) / (Timestamp() + "-" + Utf8(suite)); fs::create_directories(parent);
        if (suite == L"contracts") {
            const auto full = Plan(false), quick = Plan(true);
            Require(full.size() == 9 && quick.size() == 3, "Wrong plan size");
            unsigned duration = 0; for (const auto& round : full) duration += round.warmup + round.sample;
            Require(duration == 2160 && full[3].scene == "walk" && full[8].repeat == 3, "36-minute protocol changed");
            Require(Wide(Utf8(L"中文 空格 & path")) == L"中文 空格 & path", "Unicode conversion failed");
            const auto hashfile = parent / "hash.txt"; std::ofstream(hashfile, std::ios::binary) << "abc";
            Require(Sha256(hashfile) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA-256 mismatch");
            YAML::Node yaml; yaml["unknown"] = "missing-not-zero";
            WriteYaml(parent / "yaml.yaml", yaml); Require(ReadYaml(parent / "yaml.yaml")["unknown"].as<std::string>() == "missing-not-zero", "YAML round trip failed");
        } else if (suite == L"process") {
            const std::vector<std::wstring> values{L"", L"space path", L"quote\"x", L"trailing\\", L"中文 & no shell"};
            auto arguments = values; arguments.insert(arguments.begin(), L"--echo");
            Process process(fs::absolute(argv[2]), arguments, parent, parent / "stdout.log", parent / "stderr.log");
            while (process.Running()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
            Require(process.ExitCode() == 0, "Echo child failed");
            std::string expected; for (const auto& value : values) expected += Utf8(value) + "\r\n";
            Require(Read(parent / "stdout.log") == expected, "Command-line quoting failed");
            Process hung(fs::absolute(argv[2]), {L"--hang"}, parent, parent / "hung.log", parent / "hung.err");
            Require(hung.Stop() && !hung.Running() && hung.ExitCode() == 125, "Forced cancellation failed");
            Reject([&] { Process bad(parent / "absent.exe", {}, parent, parent / "bad.log", parent / "bad.err"); });
        } else if (suite == L"executor") {
            const auto package = parent / L"中文 模拟包 & spaces"; fs::create_directory(package);
            const auto executable = package / L"模拟游戏.exe"; fs::copy_file(argv[2], executable);
            for (const auto asset : {"configs/blockFormats.yaml", "shaders/vs_BlockShader.glsl", "shaders/fs_BlockShader.glsl", "shaders/vs_FrameShader.glsl", "shaders/fs_FrameShader.glsl", "textures/texture_atlas.png"}) {
                const auto path = package / "assets" / asset; fs::create_directories(path.parent_path()); std::ofstream(path) << "fixture";
            }
            Config config; config.game = executable; config.quick = true; config.output = parent / L"中文 成功结果";
            SetEnvironmentVariableW(L"SYMO_BENCH_FIXTURE", L"intel");
            Require(Run(config).passed, "Intel/Unicode quick fixture failed");
            Reject([&] { Run(config); });
            const auto archive = Export(config); Require(fs::file_size(archive) > 0, "ZIP export failed");
            const auto exported_log = archive.parent_path() / "results/static-1-attempt-1/stdout.log";
            Require(Read(exported_log).find(Utf8(package.wstring())) == std::string::npos, "Package path leaked in export");
            Require(!fs::exists(archive.parent_path() / "results/SymoCraft.exe"), "Executable leaked into results");
            config.output = parent / "retry";
            SetEnvironmentVariableW(L"SYMO_BENCH_FIXTURE", L"fail-walk");
            auto outcome = Run(config); Require(!outcome.passed && outcome.completed == 1, "Failed run did not stop remaining rounds");
            Require(!fs::exists(config.output / "edit-1-attempt-1"), "Ran after failure");
            const auto failed_hash = Sha256(config.output / "walk-1-attempt-1/result.yaml");
            SetEnvironmentVariableW(L"SYMO_BENCH_FIXTURE", L"amd");
            Require(Run(config, {}, {}, true).passed, "Retry did not finish");
            Require(Sha256(config.output / "walk-1-attempt-1/result.yaml") == failed_hash, "Retry overwrote failed evidence");
            Require(!ReadYaml(config.output / "session.yaml")["continuous_full_protocol"].as<bool>(), "Retry incorrectly qualified as continuous baseline");
            for (const auto mode : {L"instant-failure", L"minimized", L"missing-summary", L"old-protocol", L"bad-percentile", L"short"}) {
                config.output = parent / mode; SetEnvironmentVariableW(L"SYMO_BENCH_FIXTURE", mode);
                outcome = Run(config); Require(!outcome.passed && outcome.completed == 0, "Invalid fixture accepted");
            }
            SetEnvironmentVariableW(L"SYMO_BENCH_FIXTURE", L"hang"); config.output = parent / "cancel";
            std::stop_source source;
            std::jthread cancel([&] { std::this_thread::sleep_for(std::chrono::milliseconds(400)); source.request_stop(); });
            outcome = Run(config, source.get_token()); Require(outcome.cancelled && !outcome.passed, "Cancellation not recorded");
            SetEnvironmentVariableW(L"SYMO_BENCH_FIXTURE", nullptr);
            config.output = parent / "strict"; config.focus_policy = "strict"; Require(Run(config).passed, "Strict policy failed");
        } else throw std::runtime_error("Unknown suite");
        std::cout << "benchmark " << Utf8(suite) << " passed\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
