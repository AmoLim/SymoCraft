#include "symocraft/foundation/files.h"
#include <atomic>
#include <cstdio>
#include <string>
#include <system_error>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace SymoCraft::Files {
    namespace {
        bool DiagnosticsEnabled() {
            static const bool enabled = [] {
                wchar_t value[2] = {};
                return GetEnvironmentVariableW(L"SYMOCRAFT_PUBLISH_DIAGNOSTICS", value, 2) == 1
                    && value[0] == L'1';
            }();
            return enabled;
        }

        void RecordApi(const char* api, bool success, DWORD error, bool enabled) noexcept {
            if (!enabled) return;
            static std::atomic<unsigned long long> sequence = 0;
            FILETIME time;
            GetSystemTimeAsFileTime(&time);
            const auto utc = (static_cast<unsigned long long>(time.dwHighDateTime) << 32)
                | time.dwLowDateTime;
            char line[384];
            const int length = std::snprintf(line, sizeof(line),
                "{\"kind\":\"publish_api\",\"api\":\"%s\",\"flags\":0,\"win32\":%lu,"
                "\"success\":%s,\"pid\":%lu,\"tid\":%lu,\"utc_filetime\":%llu,\"sequence\":%llu}\n",
                api, error, success ? "true" : "false",
                GetCurrentProcessId(), GetCurrentThreadId(), utc, ++sequence);
            if (length > 0 && static_cast<std::size_t>(length) < sizeof(line)) {
                DWORD written = 0;
                WriteFile(GetStdHandle(STD_ERROR_HANDLE), line, static_cast<DWORD>(length), &written, nullptr);
            }
        }
    }

    void Publish(const std::filesystem::path& temporary, const std::filesystem::path& destination) {
        const bool diagnostics = DiagnosticsEnabled();
        // ReplaceFile also works while readers keep the old file open with FILE_SHARE_DELETE.
        const bool replaced = ReplaceFileW(destination.c_str(), temporary.c_str(), nullptr, 0, nullptr, nullptr) != 0;
        const DWORD replace_error = replaced ? ERROR_SUCCESS : GetLastError();
        RecordApi("ReplaceFileW", replaced, replace_error, diagnostics);
        if (replaced) return;
        if (replace_error == ERROR_FILE_NOT_FOUND) {
            // First publication has no destination; do not overwrite a concurrently created file.
            const bool moved = MoveFileExW(temporary.c_str(), destination.c_str(), 0) != 0;
            const DWORD move_error = moved ? ERROR_SUCCESS : GetLastError();
            RecordApi("MoveFileExW", moved, move_error, diagnostics);
            if (moved) return;
            throw std::system_error(static_cast<int>(move_error), std::system_category(),
                "Cannot publish benchmark status; api=MoveFileExW flags=0 replace_error="
                + std::to_string(replace_error));
        }
        throw std::system_error(static_cast<int>(replace_error), std::system_category(),
                                "Cannot publish benchmark status; api=ReplaceFileW flags=0");
    }
}
