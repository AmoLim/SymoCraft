#include "benchmark/runner.h"
#include "gui.h"
#include <Windows.h>
#include <shellapi.h>
#include <iostream>
#include <set>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
    int argc = 0;
    auto** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 64;
    struct FreeArgs { wchar_t** p; ~FreeArgs() { LocalFree(p); } } free_args{argv};
    if (argc == 1) return RunGui(instance, show);
    try {
        Benchmark::Config config;
        config.game = Benchmark::ExecutablePath().parent_path() / "SymoCraft.exe";
        bool run = false, resume = false, export_only = false;
        std::set<std::wstring> seen;
        for (int i = 1; i < argc; ++i) {
            const std::wstring option = argv[i];
            if (!seen.insert(option).second) throw std::runtime_error("Repeated option");
            if (option == L"--run") run = true;
            else if (option == L"--quick") config.quick = true;
            else if (option == L"--resume") resume = true;
            else if (option == L"--export") export_only = true;
            else {
                if (++i == argc) throw std::runtime_error("Missing option value");
                if (option == L"--game") config.game = Benchmark::fs::absolute(argv[i]);
                else if (option == L"--output") config.output = Benchmark::fs::absolute(argv[i]);
                else if (option == L"--focus-policy") config.focus_policy = Benchmark::Utf8(argv[i]);
                else if (option == L"--machine-label") config.machine_label = Benchmark::Utf8(argv[i]);
                else if (option == L"--power-mode") config.power_mode = Benchmark::Utf8(argv[i]);
                else if (option == L"--gpu-mode") config.gpu_mode = Benchmark::Utf8(argv[i]);
                else if (option == L"--clock-settings") config.clocks = Benchmark::Utf8(argv[i]);
                else throw std::runtime_error("Unknown option");
            }
        }
        if (config.output.empty() || (run == export_only) || (resume && !run))
            throw std::runtime_error("Use --run [--quick] [--resume] or --export, with --output DIRECTORY [--game EXE] [--focus-policy strict|allow-unfocused]");
        if (export_only) {
            Benchmark::Export(config); return 0;
        }
        return Benchmark::Run(config, {}, [](const Benchmark::Progress& progress) {
            if (progress.phase == "passed" || progress.phase == "failed" || progress.phase == "cancelled")
                std::cout << progress.message << std::endl;
        }, resume).passed ? 0 : 4;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 64; }
}
