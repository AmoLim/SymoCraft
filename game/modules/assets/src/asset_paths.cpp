#include "symocraft/assets/asset_paths.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace SymoCraft::Assets
{
    namespace
    {
        constexpr std::array<std::string_view, 6> required_files{
            "configs/blockFormats.yaml",
            "shaders/vs_BlockShader.glsl",
            "shaders/fs_BlockShader.glsl",
            "shaders/vs_FrameShader.glsl",
            "shaders/fs_FrameShader.glsl",
            "textures/texture_atlas.png"
        };

        bool IsInside(const std::filesystem::path& root, const std::filesystem::path& candidate)
        {
            auto candidate_part = candidate.begin();
            for (const auto& root_part : root)
            {
                if (candidate_part == candidate.end() || root_part != *candidate_part)
                    return false;
                ++candidate_part;
            }
            return candidate_part != candidate.end();
        }

        std::string ToUtf8(const std::filesystem::path& path)
        {
            const auto text = path.u8string();
            return std::string(text.begin(), text.end());
        }
    }

    std::filesystem::path ExecutableDirectory()
    {
        std::vector<wchar_t> buffer(256);
        for (;;)
        {
            const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (length == 0)
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                        "GetModuleFileNameW failed");
            if (length < buffer.size())
                return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
            if (buffer.size() > std::numeric_limits<DWORD>::max() / 2)
                throw std::length_error("Executable path exceeds the Windows API buffer limit");
            buffer.resize(buffer.size() * 2);
        }
    }

    std::filesystem::path Root()
    {
        return ExecutableDirectory() / "assets";
    }

    std::filesystem::path Resolve(const std::filesystem::path& relative_path)
    {
        const auto& native = relative_path.native();
        if (relative_path.empty() || relative_path.has_root_path() ||
            native.find(L'\0') != std::wstring::npos || native.find(L':') != std::wstring::npos)
            throw std::invalid_argument("Asset path must be a nonempty relative path without roots or streams");
        for (const auto& part : relative_path)
        {
            if (part == "..")
                throw std::invalid_argument("Parent traversal is not allowed in asset paths");
        }

        // Resolve existing links too, so a link below assets cannot redirect outside its root.
        const auto root = std::filesystem::weakly_canonical(Root());
        const auto result = std::filesystem::weakly_canonical(root / relative_path);
        if (!IsInside(root, result))
            throw std::invalid_argument("Asset path resolves outside the assets directory");
        return result;
    }

    const std::array<std::string_view, 6>& RequiredFiles()
    {
        return required_files;
    }

    bool CheckRequiredAssets(std::ostream& errors)
    {
        bool valid = true;
        for (const auto relative : required_files)
        {
            try
            {
                const auto path = Resolve(relative);
                std::error_code error;
                if (!std::filesystem::is_regular_file(path, error))
                {
                    errors << "Missing or inaccessible required asset: " << relative
                           << " (checked " << ToUtf8(path) << ")";
                    if (error)
                        errors << ": " << error.message();
                    errors << '\n';
                    valid = false;
                }
            }
            catch (const std::exception& error)
            {
                errors << "Cannot resolve required asset " << relative << ": " << error.what() << '\n';
                valid = false;
            }
        }
        return valid;
    }
}
