#include "application.h"
#include "application_probe.h"
#include "startup_options.h"
#include <symocraft/foundation/memory.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

namespace {
    using namespace SymoCraft;
    namespace App = SymoCraft::Application;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    struct Fault {
        App::Detail::AssemblyStage stage;
        unsigned hits = 0, shutdown_hits = 0;
    };
    App::ShutdownReport Close(App::Application& app, bool expect_failure = false) {
        try {
            auto result = app.Shutdown();
            Require(!expect_failure, "Expected shutdown exception not raised"); return result;
        } catch (const App::ShutdownError& error) {
            Require(expect_failure, "Unexpected shutdown exception");
            return error.Report(); // Copy ownership beyond the exception object.
        }
    }
}
int main(int argc, char** argv) {
    using namespace SymoCraft;
    try {
        Require(argc == 3, "Expected backend and new output parent");
        const auto parent = std::filesystem::absolute(argv[2]);
        Require(std::filesystem::create_directory(parent), "Probe output must be new with an existing parent");
        AmoBase::AmoMemory_Init(true, 1024);
        for (int point = 0; point < static_cast<int>(App::Detail::AssemblyStage::Count); ++point) {
            std::string renderer = argv[1], scenario = "static", output = (parent / ("assembly-" + std::to_string(point))).string();
            StartupOptions options;
            options.renderer = renderer; options.profile_scenario = scenario; options.scenario_seconds = 1;
            options.seed = 424242; options.regression_scene = true; options.frame_limit = 1;
            App::Application app(options, std::chrono::steady_clock::now());
            renderer = "invalid-mutated-caller-storage"; scenario = "invalid-mutated-caller-storage"; output = "invalid-mutated-caller-storage";
            Fault fault{static_cast<App::Detail::AssemblyStage>(point)};
            App::Detail::SetAssemblyProbe([](void* context, App::Detail::AssemblyStage stage) {
                auto& fault = *static_cast<Fault*>(context);
                if (stage == fault.stage) { ++fault.hits; throw std::runtime_error("Controlled post-acquisition failure"); }
            }, &fault);
            bool failed = false;
            try { app.Initialize(); } catch (const std::runtime_error& error) {
                std::cout << "assembly stage=" << point << "; hits=" << fault.hits << "; error=" << error.what() << '\n';
                failed = true;
            }
            App::Detail::SetAssemblyProbe(nullptr, nullptr);
            Require(failed && fault.hits == 1, "Assembly did not reach requested owned stage");
            const auto first = Close(app), repeated = Close(app);
            Require(first.Succeeded() && repeated.Succeeded(), "Partial assembly did not close applicable owners");
            for (std::size_t i = 0; i < first.items.size(); ++i)
                Require(first.items[i].fact == repeated.items[i].fact, "Repeated close changed cached facts");
            Require(app.DebugValues().state == Telemetry::DebugState::Paused, "Partial cleanup left rates active");
            Close(app);
        }
        const std::string output = (parent / "shutdown-failure").string();
        StartupOptions options;
        options.renderer = argv[1]; options.profile_scenario = "static"; options.scenario_seconds = 1;
        options.seed = 424242; options.regression_scene = true; options.frame_limit = 1;
        App::Application app(options, std::chrono::steady_clock::now());
        app.Initialize(); app.Run();
        Fault shutdown_fault{};
        App::Detail::SetShutdownProbe([](void* context, App::ShutdownStep step) {
            auto& fault = *static_cast<Fault*>(context); ++fault.shutdown_hits;
            if (step == App::ShutdownStep::Window)
                throw std::system_error(32, std::system_category(), "Controlled post-Destroy diagnostic failure");
        }, &shutdown_fault);
        const auto failed = Close(app, true);
        const auto hits = shutdown_fault.shutdown_hits;
        const auto repeated = Close(app, true);
        App::Detail::SetShutdownProbe(nullptr, nullptr);
        Require(hits == shutdown_fault.shutdown_hits && !failed.Succeeded() && !repeated.Succeeded(),
            "Repeated failed shutdown reran cleanup or lost failure");
        const auto index = static_cast<std::size_t>(App::ShutdownStep::Window);
        Require(failed.items[index].fact == App::ReleaseFact::Failed &&
            failed.items[static_cast<std::size_t>(App::ShutdownStep::Runtime)].fact == App::ReleaseFact::Released,
            "Window secondary diagnostic skipped Runtime cleanup");
        try { std::rethrow_exception(failed.items[index].error); }
        catch (const std::system_error& error) { Require(error.code().value() == 32, "Shutdown report lost original code"); }
        Close(app, true);
        std::cout << "Real Application: ten owned-stage failures, repeated cleanup, caller-string lifetime and owned shutdown error passed; injected errors are controlled, not hardware failures" << '\n';
        Require(std::filesystem::is_empty(parent), "Lifecycle cleanup created old performance files");
        return 0;
    } catch (const std::exception& error) {
        App::Detail::SetAssemblyProbe(nullptr, nullptr); App::Detail::SetShutdownProbe(nullptr, nullptr);
        std::cerr << error.what() << '\n'; return 1;
    }
}
