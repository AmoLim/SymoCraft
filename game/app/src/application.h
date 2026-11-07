#pragma once
#include <iosfwd>
#include <memory>
#include <chrono>
#include <symocraft/telemetry/debug_snapshot.h>
#include "shutdown_report.h"

namespace SymoCraft {
    struct StartupOptions;
    namespace Application {
        class Application final {
        public:
            Application(const StartupOptions& options, std::chrono::steady_clock::time_point entry);
            ~Application() noexcept;
            Application(const Application&) = delete;
            Application& operator=(const Application&) = delete;
            void Initialize();
            void Run();
            ShutdownReport Shutdown();
            Telemetry::DebugSnapshot DebugValues() const;
        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;
        };
        void PrintWorldSummary(const StartupOptions& options);
        bool CheckAssets(const StartupOptions& options, std::ostream& errors);
    }
}
