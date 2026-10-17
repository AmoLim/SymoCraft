#include <symocraft/foundation/memory.h>
#include "application.h"
#include "game_entry.h"
#include <symocraft/assets/asset_paths.h>
#include "startup_options.h"
#include <symocraft/telemetry/performance.h>
#include <symocraft/renderer/error.h>

#include <iostream>
#include <exception>
#include <string_view>
#include <vector>
#include <memory>

namespace {

const char* ErrorCategoryName(SymoCraft::Rendering::RenderErrorCategory category) noexcept {
    using Category = SymoCraft::Rendering::RenderErrorCategory;
    switch (category) {
    case Category::InvalidInput: return "InvalidInput";
    case Category::InvalidHandle: return "InvalidHandle";
    case Category::AllocationFailure: return "AllocationFailure";
    case Category::Unsupported: return "Unsupported";
    case Category::AssetUnavailable: return "AssetUnavailable";
    case Category::InitializationFailure: return "InitializationFailure";
    case Category::DeviceLost: return "DeviceLost";
    case Category::SubmissionFailure: return "SubmissionFailure";
    case Category::Timeout: return "Timeout";
    case Category::InvalidState: return "InvalidState";
    case Category::WrongThread: return "WrongThread";
    case Category::ReentrantCall: return "ReentrantCall";
    }
    return "Unknown";
}

void PrintRenderError(const SymoCraft::Rendering::RenderError& error) {
    std::cerr << "[renderer] " << error.BackendName() << " " << error.Operation()
              << ": " << error.what() << "; category=" << ErrorCategoryName(error.Category())
              << "; native=" << error.NativeCode()
              << "; diagnostic=" << error.Diagnostic() << std::endl;
}

} // namespace

int SymoCraft::Application::RunGame(int argc, char* argv[])
{
    const auto entry = SymoCraft::Performance::Clock::now();
    SymoCraft::StartupOptions options;
    try {
        std::vector<std::string_view> arguments;
        for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
        options = SymoCraft::ParseStartupOptions(arguments);
    } catch (const std::exception& error) {
        std::cerr << SymoCraft::StartupUsage << '\n' << error.what() << '\n';
        return 64;
    }
    try {
        if (!SymoCraft::Application::CheckAssets(options, std::cerr)) return 2;
    } catch (const SymoCraft::Rendering::RenderError& error) {
        PrintRenderError(error);
        return 3;
    }
    if (options.check_assets)
    {
        std::cout << "All required assets are present.\n";
        return 0;
    }

#ifdef _DEBUG
    AmoBase::AmoMemory_Init(true, 1024);
#endif
    int exit_code = 0;
    std::unique_ptr<SymoCraft::Performance::Session> performance;
    try
    {
        if (options.world_summary) SymoCraft::Application::PrintWorldSummary(options);
        else {
            if (!options.benchmark.empty()) {
                SymoCraft::Performance::SessionConfig config;
                config.output_directory = options.output_directory;
                config.focus_policy = options.focus_policy;
                config.benchmark = options.benchmark;
                config.width = options.width; config.height = options.height;
                config.warmup_seconds = options.warmup_seconds; config.sample_seconds = options.sample_seconds;
                config.vsync = options.vsync;
                performance = std::make_unique<SymoCraft::Performance::Session>(config, entry);
                performance->startup["argument_preflight_and_collector_setup"] = SymoCraft::Performance::Milliseconds(entry);
            }
            SymoCraft::Application::Init(options, performance.get());
            SymoCraft::Application::Run(options, performance.get());
        }
    }
    catch (const SymoCraft::Rendering::RenderError& error)
    {
        PrintRenderError(error);
        exit_code = 3;
    }
    catch (const std::exception& error)
    {
        std::cerr << "[runtime] fatal: " << error.what()
                  << "; requested_backend=" << options.renderer << std::endl;
        exit_code = 3;
    }
    catch (...)
    {
        std::cerr << "[runtime] fatal: unexpected non-standard exception" << std::endl;
        exit_code = 3;
    }

    // Release all graphics ownership while the borrowed Window is still alive.
    try {
        SymoCraft::Application::Free();
    }
    catch (const SymoCraft::Rendering::RenderError& error) {
        std::cerr << "[runtime] shutdown failed; requested_backend=" << options.renderer << '\n';
        PrintRenderError(error);
        if (exit_code == 0) exit_code = 3;
    }
    catch (const std::exception& error) {
        std::cerr << "[runtime] shutdown failed: " << error.what() << std::endl;
        if (exit_code == 0) exit_code = 3;
    }
    catch (...) {
        std::cerr << "[runtime] shutdown failed: unexpected non-standard exception" << std::endl;
        if (exit_code == 0) exit_code = 3;
    }
    if (performance) {
        try {
            const bool valid = performance->Export(exit_code == 0);
            std::cout << "[performance] exported; valid_run=" << valid << '\n';
            if (!valid && exit_code == 0) exit_code = 4;
        }
        catch (const std::exception& error) { std::cerr << "[performance] export failed: " << error.what() << '\n'; exit_code = 3; }
    }
    AmoBase::AmoMemory_MemoryLeaksDetected();
    if (!options.world_summary)
        std::cout << "[runtime] shutdown complete; exit_code=" << exit_code << std::endl;
    return exit_code;
}
