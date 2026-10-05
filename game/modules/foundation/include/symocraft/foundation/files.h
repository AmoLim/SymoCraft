#pragma once
#include <filesystem>

namespace SymoCraft::Files {
    // Atomically publish a closed temporary file on the same Windows volume.
    void Publish(const std::filesystem::path& temporary, const std::filesystem::path& destination);
}
