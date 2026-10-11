#pragma once
#include "benchmark/runner.h"
#include <Windows.h>
#include <chrono>

namespace Benchmark {
    using Clock = std::chrono::steady_clock;
    struct Handle {
        HANDLE value = nullptr;
        Handle() = default;
        explicit Handle(HANDLE h) : value(h) {}
        ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
    };
    std::wstring Quote(const std::wstring& argument);
    void Check(bool success, const char* operation);
    class Process {
    public:
        Process(const fs::path& executable, const std::vector<std::wstring>& arguments,
                const fs::path& directory, const fs::path& stdout_path, const fs::path& stderr_path);
        bool Running() const;
        DWORD ExitCode() const;
        bool Stop(); // WM_CLOSE, then bounded forced termination of this process's job only.
        DWORD Id() const { return pid_; }
        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;
    private:
        Handle job_, process_;
        DWORD pid_ = 0;
    };
    YAML::Node Machine();
    void RequireRegular(const fs::path& path);
}
