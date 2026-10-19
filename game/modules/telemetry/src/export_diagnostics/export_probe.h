#pragma once
#include <filesystem>

namespace SymoCraft::Performance::Detail {
    // Private cold-path seam; the production owner still performs every actual Publish.
    using ExportProbe = void (*)(void*, const std::filesystem::path&, const char*);
    void SetExportProbe(ExportProbe probe, void* context) noexcept;
}
