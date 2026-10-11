#pragma once
#include <iosfwd>

namespace SymoCraft {
    struct StartupOptions;
    namespace Performance { class Session; }
    namespace Application {
        void Init(const StartupOptions& options, Performance::Session* performance = nullptr);
        void Run(const StartupOptions& options, Performance::Session* performance = nullptr);
        void PrintWorldSummary(const StartupOptions& options);
        bool CheckAssets(const StartupOptions& options, std::ostream& errors);
        void Free();
    }
}
