#include <MemoryAllocator/AmoBase.h>
#include "core/application.h"
#include "core/asset_paths.h"
#include "core/startup_options.h"
#include "core/performance.h"

#include <iostream>
#include <exception>
#include <string_view>
#include <vector>
#include <memory>

int main(int argc, char* argv[])
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
    if (!SymoCraft::Assets::CheckRequiredAssets(std::cerr))
        return 2;
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
                performance = std::make_unique<SymoCraft::Performance::Session>(options, entry);
                performance->startup["argument_preflight_and_collector_setup"] = SymoCraft::Performance::Milliseconds(entry);
            }
            SymoCraft::Application::Init(options, performance.get());
            SymoCraft::Application::Run(options, performance.get());
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "[runtime] fatal: " << error.what() << std::endl;
        exit_code = 3;
    }
    catch (...)
    {
        std::cerr << "[runtime] fatal: unexpected non-standard exception" << std::endl;
        exit_code = 3;
    }

    // Run's local GL objects have unwound; release renderer objects before the context.
    SymoCraft::Application::Free();
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
