#include "platform.h"
#include <bcrypt.h>
#include <powrprof.h>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace Benchmark {
    void Check(bool success, const char* operation)
    {
        if (!success) throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation);
    }
    std::string Utf8(const std::wstring& value)
    {
        if (value.empty()) return {};
        const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        Check(size != 0, "UTF-8 conversion");
        std::string result(size, '\0');
        Check(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr) != 0, "UTF-8 conversion");
        return result;
    }
    std::wstring Wide(const std::string& value)
    {
        if (value.empty()) return {};
        const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
        Check(size != 0, "UTF-16 conversion");
        std::wstring result(size, L'\0');
        Check(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), size) != 0, "UTF-16 conversion");
        return result;
    }
    fs::path ExecutablePath()
    {
        std::wstring buffer(32768, L'\0');
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        Check(length != 0 && length < buffer.size(), "Executable path");
        buffer.resize(length); return buffer;
    }
    fs::path DefaultOutputParent()
    {
        std::wstring buffer(32768, L'\0');
        const auto length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer.data(), static_cast<DWORD>(buffer.size()));
        Check(length != 0 && length < buffer.size(), "LOCALAPPDATA");
        buffer.resize(length); return fs::path(buffer) / "SymoCraftBenchmark" / "results";
    }
    std::string Timestamp()
    {
        SYSTEMTIME time{}; GetSystemTime(&time);
        char value[40]{};
        sprintf_s(value, "%04u%02u%02u-%02u%02u%02u-%03uZ", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
        return value;
    }
    std::wstring Quote(const std::wstring& argument)
    {
        std::wstring out = L"\""; unsigned slashes = 0;
        for (const auto c : argument) {
            if (c == L'\\') { ++slashes; continue; }
            out.append(c == L'"' ? 2 * slashes + 1 : slashes, L'\\');
            slashes = 0; out += c;
        }
        out.append(2 * slashes, L'\\'); out += L'"'; return out;
    }
    Process::Process(const fs::path& executable, const std::vector<std::wstring>& arguments,
                     const fs::path& directory, const fs::path& stdout_path, const fs::path& stderr_path)
    {
        SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
        Handle output(CreateFileW(stdout_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        Handle error(CreateFileW(stderr_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        Handle input(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr));
        Check(output.value != INVALID_HANDLE_VALUE && error.value != INVALID_HANDLE_VALUE && input.value != INVALID_HANDLE_VALUE, "Create process logs");
        job_.value = CreateJobObjectW(nullptr, nullptr); Check(job_.value != nullptr, "Create job");
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        Check(SetInformationJobObject(job_.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) != 0, "Configure job");

        SIZE_T size = 0; InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        std::vector<unsigned char> storage(size);
        auto* attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
        Check(InitializeProcThreadAttributeList(attributes, 1, 0, &size) != 0, "Initialize process attributes");
        struct Cleanup { LPPROC_THREAD_ATTRIBUTE_LIST list; ~Cleanup() { DeleteProcThreadAttributeList(list); } } cleanup{attributes};
        HANDLE inherited[]{input.value, output.value, error.value};
        Check(UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited, sizeof(inherited), nullptr, nullptr) != 0, "Set inherited handles");
        STARTUPINFOEXW startup{}; startup.StartupInfo.cb = sizeof(startup);
        startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        startup.StartupInfo.hStdInput = input.value; startup.StartupInfo.hStdOutput = output.value; startup.StartupInfo.hStdError = error.value;
        startup.lpAttributeList = attributes;
        std::wstring command = Quote(executable.wstring());
        for (const auto& argument : arguments) command += L" " + Quote(argument);
        PROCESS_INFORMATION info{};
        Check(CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
            CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT, nullptr, directory.c_str(), &startup.StartupInfo, &info) != 0, "Start child process");
        process_.value = info.hProcess; Handle thread(info.hThread); pid_ = info.dwProcessId;
        if (!AssignProcessToJobObject(job_.value, process_.value)) {
            const auto code = GetLastError(); TerminateProcess(process_.value, 125); WaitForSingleObject(process_.value, 5000);
            SetLastError(code); Check(false, "Assign child to job");
        }
        Check(ResumeThread(thread.value) != static_cast<DWORD>(-1), "Resume child");
    }
    bool Process::Running() const
    {
        const auto result = WaitForSingleObject(process_.value, 0);
        Check(result != WAIT_FAILED, "Poll child"); return result == WAIT_TIMEOUT;
    }
    DWORD Process::ExitCode() const
    {
        DWORD code = 0; Check(GetExitCodeProcess(process_.value, &code) != 0, "Read child exit code"); return code;
    }
    bool Process::Stop()
    {
        if (!Running()) return false;
        EnumWindows([](HWND window, LPARAM argument) -> BOOL {
            DWORD owner = 0; GetWindowThreadProcessId(window, &owner);
            if (owner == static_cast<DWORD>(argument)) PostMessageW(window, WM_CLOSE, 0, 0);
            return TRUE;
        }, static_cast<LPARAM>(pid_));
        if (WaitForSingleObject(process_.value, 3000) == WAIT_OBJECT_0) return false;
        Check(TerminateJobObject(job_.value, 125) != 0, "Terminate child job");
        Check(WaitForSingleObject(process_.value, 5000) == WAIT_OBJECT_0, "Wait for terminated child"); return true;
    }
    void RequireRegular(const fs::path& path)
    {
        if (!fs::is_regular_file(path)) throw std::runtime_error("Required file is missing");
        for (auto current = fs::absolute(path); !current.empty(); current = current.parent_path()) {
            const auto attributes = GetFileAttributesW(current.c_str());
            Check(attributes != INVALID_FILE_ATTRIBUTES, "Inspect file attributes");
            if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) throw std::runtime_error("Linked/reparse paths are not supported for benchmark artifacts");
            if (current == current.parent_path()) break;
        }
    }
    std::string Sha256(const fs::path& path)
    {
        RequireRegular(path);
        BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE hash = nullptr;
        struct Cleanup { BCRYPT_ALG_HANDLE& a; BCRYPT_HASH_HANDLE& h; ~Cleanup() { if (h) BCryptDestroyHash(h); if (a) BCryptCloseAlgorithmProvider(a, 0); } } cleanup{algorithm, hash};
        auto check = [](NTSTATUS result) { if (result < 0) throw std::runtime_error("SHA-256 failed"); };
        check(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
        check(BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0));
        std::ifstream input(path, std::ios::binary); if (!input) throw std::runtime_error("Cannot read hash input");
        std::array<unsigned char, 65536> buffer{};
        while (input) {
            input.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            if (input.gcount()) check(BCryptHashData(hash, buffer.data(), static_cast<ULONG>(input.gcount()), 0));
        }
        if (!input.eof()) throw std::runtime_error("Hash input read failed");
        std::array<unsigned char, 32> digest{}; check(BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0));
        std::ostringstream text; text << std::hex << std::setfill('0');
        for (auto byte : digest) text << std::setw(2) << static_cast<unsigned>(byte);
        return text.str();
    }
    YAML::Node ReadYaml(const fs::path& path)
    {
        RequireRegular(path);
        // Permit atomic replacement while polling; read one open file snapshot, not a partially written document.
        Handle input(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        Check(input.value != INVALID_HANDLE_VALUE, "Open protocol snapshot");
        LARGE_INTEGER size{}; Check(GetFileSizeEx(input.value, &size) != 0, "Size protocol snapshot");
        if (size.QuadPart < 0 || size.QuadPart > 4 * 1024 * 1024) throw std::runtime_error("Oversized protocol file");
        std::string text(static_cast<std::size_t>(size.QuadPart), '\0'); DWORD read = 0;
        Check(ReadFile(input.value, text.data(), static_cast<DWORD>(text.size()), &read, nullptr) != 0 && read == text.size(), "Read protocol snapshot");
        return YAML::Load(text);
    }
    void WriteYaml(const fs::path& path, const YAML::Node& node)
    {
        const auto temporary = fs::path(path.wstring() + L".tmp");
        std::ofstream file(temporary, std::ios::binary); file.exceptions(std::ios::badbit | std::ios::failbit);
        file << YAML::Dump(node) << '\n'; file.close();
        Check(MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0, "Publish YAML");
    }
    YAML::Node Machine()
    {
        YAML::Node machine;
        wchar_t value[512]{}; DWORD size = sizeof(value);
        if (RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString", RRF_RT_REG_SZ, nullptr, value, &size) == ERROR_SUCCESS)
            machine["cpu"] = Utf8(value);
        else machine["cpu"] = "unknown";
        size = sizeof(value);
        if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuildNumber", RRF_RT_REG_SZ, nullptr, value, &size) == ERROR_SUCCESS)
            machine["windows_build"] = Utf8(value);
        MEMORYSTATUSEX memory{sizeof(memory)};
        if (GlobalMemoryStatusEx(&memory)) machine["physical_memory_bytes"] = memory.ullTotalPhys;
        SYSTEM_POWER_STATUS power{};
        if (GetSystemPowerStatus(&power)) machine["ac_line_status"] = static_cast<unsigned>(power.ACLineStatus);
        GUID* scheme = nullptr;
        if (PowerGetActiveScheme(nullptr, &scheme) == ERROR_SUCCESS) {
            wchar_t text[64]{};
            swprintf_s(text, L"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", scheme->Data1, scheme->Data2, scheme->Data3,
                scheme->Data4[0], scheme->Data4[1], scheme->Data4[2], scheme->Data4[3], scheme->Data4[4], scheme->Data4[5], scheme->Data4[6], scheme->Data4[7]);
            machine["power_scheme_guid"] = Utf8(text); LocalFree(scheme);
        }
        machine["cpu_temperature"] = "not-collected";
        machine["gpu_temperature"] = "not-collected";
        machine["actual_gpu_and_driver"] = "capture/summary.yaml metadata.gl_renderer and gl_version";
        return machine;
    }
}
