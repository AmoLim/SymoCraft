#include "symocraft/foundation/files.h"
#include <system_error>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace SymoCraft::Files {
    void Publish(const std::filesystem::path& temporary, const std::filesystem::path& destination) {
        if (!MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING))
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                    "Cannot publish benchmark status");
    }
}
