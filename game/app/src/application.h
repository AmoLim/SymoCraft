#pragma once
#include <iosfwd>
#include <memory>
#include <symocraft/telemetry/performance.h>
#include "shutdown_report.h"

namespace SymoCraft {
    struct StartupOptions;
    namespace Performance { class Session; }
    namespace Application {
        class Application final {
        public:
            Application(const StartupOptions& options, Performance::Clock::time_point entry);
            ~Application() noexcept;
            Application(const Application&) = delete;
            Application& operator=(const Application&) = delete;
            void Initialize();
            void Run();
            ShutdownReport Shutdown();
            Performance::Session* Observation() noexcept;
        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;
        };
        void PrintWorldSummary(const StartupOptions& options);
        bool CheckAssets(const StartupOptions& options, std::ostream& errors);
    }
}
