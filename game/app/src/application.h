#pragma once

namespace SymoCraft {
    struct StartupOptions;
    namespace Performance { class Session; }
    namespace Application {
        void Init(const StartupOptions& options, Performance::Session* performance = nullptr);
        void Run(const StartupOptions& options, Performance::Session* performance = nullptr);
        void PrintWorldSummary(const StartupOptions& options);
        void Free();
    }
}
