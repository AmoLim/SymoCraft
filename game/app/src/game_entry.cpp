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
    } catch (const std::exception& error) {
        std::cerr << "[runtime] asset preflight failed: " << error.what() << '\n'; return 3;
    } catch (...) {
        std::cerr << "[runtime] asset preflight failed: unexpected non-standard exception\n"; return 3;
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
    std::exception_ptr first_failure;
    std::unique_ptr<SymoCraft::Application::Application> application;
    try {
        if (options.world_summary) SymoCraft::Application::PrintWorldSummary(options);
        else {
            application = std::make_unique<SymoCraft::Application::Application>(options, entry);
            application->Initialize();
            application->Run();
        }
    } catch (const SymoCraft::Rendering::RenderError& error) {
        first_failure = std::current_exception();
        PrintRenderError(error); exit_code = 3;
    } catch (const std::exception& error) {
        first_failure = std::current_exception();
        std::cerr << "[runtime] fatal: " << error.what() << "; requested_backend=" << options.renderer << std::endl;
        exit_code = 3;
    } catch (...) {
        first_failure = std::current_exception();
        std::cerr << "[runtime] fatal: unexpected non-standard exception" << std::endl; exit_code = 3;
    }

    SymoCraft::Application::ShutdownReport shutdown;
    if (application) {
        try { shutdown = application->Shutdown(); }
        catch (const SymoCraft::Application::ShutdownError& error) {
            // The exception owns its report. Copy the values before its lifetime ends,
            // then join this path with successful Shutdown before sealing observations.
            shutdown = error.Report();
        } catch (...) {
            auto& item = shutdown.items[static_cast<std::size_t>(SymoCraft::Application::ShutdownStep::Runtime)];
            item.fact = SymoCraft::Application::ReleaseFact::Failed; item.error = std::current_exception();
        }
    }
    try {
    SymoCraft::Performance::FinalFacts facts;
    facts.runtime_succeeded = exit_code == 0;
    facts.shutdown_succeeded = shutdown.Succeeded();
    for (const auto& item : shutdown.items) {
        if (!item.error) continue;
        if (!first_failure) first_failure = item.error;
        try { std::rethrow_exception(item.error); }
        catch (const SymoCraft::Rendering::RenderError& error) { PrintRenderError(error); }
        catch (const std::exception& error) { std::cerr << "[runtime] shutdown failed: " << error.what() << '\n'; }
        catch (...) { std::cerr << "[runtime] shutdown failed: unexpected non-standard exception\n"; }
        if (item.error != first_failure) {
            try { std::rethrow_exception(item.error); }
            catch (const std::exception& error) { facts.secondary_errors.emplace_back(error.what()); }
            catch (...) { facts.secondary_errors.emplace_back("Unknown shutdown failure"); }
        }
    }
    if (!shutdown.Succeeded()) exit_code = 3;
    if (first_failure) {
        try { std::rethrow_exception(first_failure); }
        catch (const std::exception& error) { facts.first_error = error.what(); }
        catch (...) { facts.first_error = "Unknown primary failure"; }
    }
    if (auto* performance = application ? application->Observation() : nullptr) {
        try {
            facts.completed = performance->Completed();
            performance->Seal(facts);
            const auto result = performance->Export();
            if (result.state != SymoCraft::Performance::ExportState::Saved) {
                std::cerr << "[performance] export failed: " << result.stage << "; path=" << result.path.string()
                          << "; native=" << result.native_error.value() << "; " << result.diagnostic << '\n';
                exit_code = 3;
            } else {
                std::cout << "[performance] exported; valid_run=" << result.valid_run << '\n';
                if (!result.valid_run && exit_code == 0) exit_code = 4;
            }
        } catch (const std::exception& error) {
            std::cerr << "[performance] finalization failed: " << error.what() << '\n'; exit_code = 3;
        } catch (...) {
            std::cerr << "[performance] finalization failed: unexpected non-standard exception\n"; exit_code = 3;
        }
    }
    } catch (const std::exception& error) {
        std::cerr << "[runtime] final facts unavailable: " << error.what() << '\n'; exit_code = 3;
    } catch (...) {
        std::cerr << "[runtime] final facts unavailable: unexpected non-standard exception\n"; exit_code = 3;
    }
    application.reset();
    AmoBase::AmoMemory_MemoryLeaksDetected();
    if (!options.world_summary) std::cout << "[runtime] shutdown complete; exit_code=" << exit_code << std::endl;
    return exit_code;
}
