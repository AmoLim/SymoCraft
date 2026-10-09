#pragma once

#include <array>
#include <filesystem>
#include <iosfwd>
#include <string_view>

namespace SymoCraft::Assets
{
    std::filesystem::path ExecutableDirectory();
    std::filesystem::path Root();

    // Accept only paths inside the executable's assets directory.
    std::filesystem::path Resolve(const std::filesystem::path& relative_path);

    const std::array<std::string_view, 6>& RequiredFiles();
    bool CheckRequiredAssets(std::ostream& errors);
}
