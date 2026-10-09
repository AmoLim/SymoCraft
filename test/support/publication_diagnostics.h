#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <typeinfo>
#include <utility>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace SymoCraft::Test {
    class ContractFailure : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    inline std::string JsonText(const std::string& text) {
        std::ostringstream out;
        out << '"';
        for (const unsigned char c : text) {
            switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                                  << static_cast<unsigned>(c) << std::dec;
                else out << c;
            }
        }
        out << '"';
        return out.str();
    }

    inline std::string PathText(const std::filesystem::path& path) {
        const auto utf8 = path.u8string();
        return {utf8.begin(), utf8.end()};
    }

    inline std::string EnvironmentJson(const char* name) {
        const auto value = std::getenv(name);
        return value && *value ? JsonText(value) : "null";
    }

    inline std::string Timestamp(std::uint64_t& ticks) {
        FILETIME filetime{};
        GetSystemTimeAsFileTime(&filetime);
        ticks = (static_cast<std::uint64_t>(filetime.dwHighDateTime) << 32) | filetime.dwLowDateTime;
        SYSTEMTIME utc{};
        if (!FileTimeToSystemTime(&filetime, &utc)) return "null";
        char text[40];
        std::snprintf(text, sizeof(text), "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
                      utc.wYear, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute,
                      utc.wSecond, utc.wMilliseconds);
        return JsonText(text);
    }

    // Observation handles share deletion and close before the tested publication call.
    inline std::string ObservePath(const std::filesystem::path& path) {
        const DWORD attributes = GetFileAttributesW(path.c_str());
        const DWORD attribute_error = attributes == INVALID_FILE_ATTRIBUTES ? GetLastError() : ERROR_SUCCESS;
        std::ostringstream out;
        out << "{\"exists\":";
        if (attributes != INVALID_FILE_ATTRIBUTES) out << "true";
        else if (attribute_error == ERROR_FILE_NOT_FOUND || attribute_error == ERROR_PATH_NOT_FOUND) out << "false";
        else out << "null";
        out << ",\"attributes\":";
        if (attributes == INVALID_FILE_ATTRIBUTES) out << "null";
        else out << attributes;
        out << ",\"query_error\":" << attribute_error;
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            out << ",\"content_bytes\":null,\"content_fnv1a64\":null,\"content_error\":null"
                   ",\"content_state\":" << JsonText(attributes == INVALID_FILE_ATTRIBUTES ? "unavailable" : "directory") << '}';
            return out.str();
        }
        const HANDLE file = CreateFileW(path.c_str(), GENERIC_READ,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                       nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        const DWORD open_error = file == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
        std::uint64_t bytes = 0;
        std::uint64_t digest = 14695981039346656037ull;
        DWORD content_error = open_error;
        bool truncated = false;
        if (file != INVALID_HANDLE_VALUE) {
            char buffer[4096];
            while (true) {
                DWORD read = 0;
                if (!ReadFile(file, buffer, sizeof(buffer), &read, nullptr)) {
                    content_error = GetLastError();
                    break;
                }
                if (read == 0) break;
                for (DWORD i = 0; i < read; ++i) {
                    digest ^= static_cast<unsigned char>(buffer[i]);
                    digest *= 1099511628211ull;
                }
                bytes += read;
                if (bytes >= 1024 * 1024) { truncated = true; break; }
            }
            CloseHandle(file);
        }
        out << ",\"content_bytes\":";
        if (open_error) out << "null";
        else out << bytes;
        out << ",\"content_fnv1a64\":";
        if (content_error) out << "null";
        else {
            std::ostringstream hex;
            hex << std::hex << std::setw(16) << std::setfill('0') << digest;
            out << JsonText(hex.str());
        }
        out << ",\"content_error\":" << content_error << ",\"content_truncated\":"
            << (truncated ? "true" : "false") << ",\"content_state\":"
            << JsonText(content_error ? "query_failed" : "observed") << '}';
        return out.str();
    }

    class PublicationDiagnostics {
    public:
        explicit PublicationDiagnostics(std::string suite) : suite_(std::move(suite)) {
            if (const auto value = std::getenv("SYMOCRAFT_FIX1_RUN_ID"); value && *value) run_id_ = value;
            else {
                std::uint64_t ticks = 0;
                Timestamp(ticks);
                run_id_ = std::to_string(GetCurrentProcessId()) + '-' + std::to_string(ticks);
            }
        }

        void Begin(const char* phase, const char* operation, const std::filesystem::path& source,
                   const std::filesystem::path& destination, int iteration = -1, DWORD reader_share = 0,
                   const char* held_reader = "none", int expected_error = -1) noexcept {
            try {
                Context(phase, operation, source, destination, iteration, reader_share, held_reader, expected_error);
                before_ = ObservePair();
                ++operation_events_;
                Emit("begin", nullptr);
            } catch (...) { DiagnosticFailure(); }
        }

        void Context(const char* phase, const char* operation, const std::filesystem::path& source,
                     const std::filesystem::path& destination, int iteration = -1, DWORD reader_share = 0,
                     const char* held_reader = "none", int expected_error = -1) noexcept {
            try {
                phase_ = phase;
                operation_ = operation;
                source_ = source;
                destination_ = destination;
                iteration_ = iteration;
                reader_share_ = reader_share;
                held_reader_ = held_reader;
                expected_error_ = expected_error;
                before_ = "null";
            } catch (...) { DiagnosticFailure(); }
        }

        void Success() noexcept { Emit("success", nullptr); }
        void SuiteSuccess() noexcept { Emit("suite_success", nullptr); }
        void Failure(const char* event, const std::exception& error, const char* failure_class = nullptr) noexcept {
            Emit(event, &error, false, failure_class);
        }
        void UnknownFailure(const char* event) noexcept { Emit(event, nullptr, true); }

    private:
        std::string ObservePair() const {
            if (source_.empty() && destination_.empty()) return "null";
            return "{\"source\":" + ObservePath(source_) + ",\"target\":" + ObservePath(destination_) + '}';
        }

        void DiagnosticFailure() noexcept {
            std::fprintf(stderr, "{\"kind\":\"diagnostic_failure\",\"numeric_win32\":null,\"pid\":%lu,\"tid\":%lu}\n",
                         static_cast<unsigned long>(GetCurrentProcessId()), static_cast<unsigned long>(GetCurrentThreadId()));
            std::fflush(stderr);
        }

        void Emit(const char* event, const std::exception* error, bool unknown = false,
                  const char* failure_class = nullptr) noexcept {
            // Capture exception codes before observations; query errors occupy separate fields.
            const auto system = dynamic_cast<const std::system_error*>(error);
            const bool is_win32 = system && system->code().category() == std::system_category();
            const int code = system ? system->code().value() : 0;
            try {
                std::uint64_t ticks = 0;
                const auto utc = Timestamp(ticks);
                if (std::string(event) == "expected_rejection") ++expected_rejections_;
                if (std::string(event) == "expected_standard_rejection") ++standard_rejections_;
                std::ostringstream out;
                out << "{\"kind\":\"publication_test\",\"suite\":" << JsonText(suite_)
                    << ",\"run_id\":" << JsonText(run_id_) << ",\"sequence\":" << ++sequence_
                    << ",\"utc_filetime\":" << ticks << ",\"utc\":" << utc
                    << ",\"pid\":" << GetCurrentProcessId() << ",\"tid\":" << GetCurrentThreadId()
                    << ",\"config\":" << EnvironmentJson("SYMOCRAFT_FIX1_CONFIG")
                    << ",\"source_sha\":" << EnvironmentJson("SYMOCRAFT_FIX1_SOURCE_SHA")
                    << ",\"exe_sha\":" << EnvironmentJson("SYMOCRAFT_FIX1_EXE_SHA")
                    << ",\"event\":" << JsonText(event) << ",\"outcome\":" << JsonText(event)
                    << ",\"phase\":" << JsonText(phase_) << ",\"operation\":" << JsonText(operation_)
                    << ",\"iteration\":";
                if (iteration_ < 0) out << "null";
                else out << iteration_;
                out << ",\"reader_share\":" << reader_share_ << ",\"held_reader\":" << JsonText(held_reader_)
                    << ",\"expected\":" << JsonText(expected_error_ != -1 ? "rejection" : "success")
                    << ",\"expected_win32\":";
                if (expected_error_ < 0) out << "null";
                else out << expected_error_;
                out << ",\"source\":" << JsonText(PathText(source_))
                    << ",\"destination\":" << JsonText(PathText(destination_))
                    << ",\"output_directory\":" << JsonText(PathText(destination_.parent_path()))
                    << ",\"before\":" << before_ << ",\"after\":" << ObservePair()
                    << ",\"numeric_win32\":";
                if (is_win32) out << code;
                else out << "null";
                out << ",\"error_code\":";
                if (system) out << code;
                else out << "null";
                out << ",\"error_category\":" << (system ? JsonText(system->code().category().name()) : "null")
                    << ",\"failure_class\":";
                if (unknown) out << "\"unknown_exception\"";
                else if (failure_class) out << JsonText(failure_class);
                else if (dynamic_cast<const ContractFailure*>(error)) out << "\"assertion_failure\"";
                else if (dynamic_cast<const std::ios_base::failure*>(error)) out << "\"write_stream_exception\"";
                else if (dynamic_cast<const std::filesystem::filesystem_error*>(error)) out << "\"filesystem_exception\"";
                else if (is_win32) out << "\"win32_system_error\"";
                else if (system) out << "\"system_error\"";
                else if (error) out << "\"standard_exception\"";
                else out << "null";
                out << ",\"message\":" << (error ? JsonText(error->what()) : "null")
                    << ",\"exception_type\":" << (error ? JsonText(typeid(*error).name()) : "null")
                    << ",\"operation_events\":" << operation_events_
                    << ",\"expected_rejections\":" << expected_rejections_
                    << ",\"expected_standard_rejections\":" << standard_rejections_ << '}';
                std::cerr << out.str() << '\n' << std::flush;
                if (!std::cerr) DiagnosticFailure();
            } catch (...) { DiagnosticFailure(); }
        }

        std::string suite_, run_id_, phase_ = "setup", operation_ = "setup", held_reader_ = "none";
        std::filesystem::path source_, destination_;
        std::string before_ = "null";
        int iteration_ = -1, expected_error_ = -1;
        DWORD reader_share_ = 0;
        std::uint64_t sequence_ = 0, operation_events_ = 0, expected_rejections_ = 0, standard_rejections_ = 0;
    };
}
