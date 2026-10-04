#include <MemoryAllocator/AmoBase.h>
#include "core/application.h"
#include "core/asset_paths.h"
#include "core/startup_options.h"

#include <iostream>
#include <exception>
#include <string_view>
#include <vector>
#include <memory>

int main(int argc, char* argv[])
{
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
    try
    {
        if (options.world_summary) SymoCraft::Application::PrintWorldSummary(options);
        else {
            SymoCraft::Application::Init(options);
            SymoCraft::Application::Run(options);
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
    AmoBase::AmoMemory_MemoryLeaksDetected();
    if (!options.world_summary)
        std::cout << "[runtime] shutdown complete; exit_code=" << exit_code << std::endl;
    return exit_code;
}
