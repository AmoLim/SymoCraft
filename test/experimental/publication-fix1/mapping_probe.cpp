#include "symocraft/foundation/files.h"
#include "publication_diagnostics.h"
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
    namespace fs = std::filesystem;
    namespace Diagnostic = SymoCraft::Test;
    constexpr DWORD Share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    constexpr const char* Old = "old-complete-status\n";
    constexpr const char* New = "new-complete-status\n";

    void Check(bool success, const char* operation) {
        if (!success) {
            const DWORD error = GetLastError();
            throw std::system_error(static_cast<int>(error), std::system_category(), operation);
        }
    }

    struct Mapping {
        HANDLE reader = INVALID_HANDLE_VALUE;
        HANDLE section = nullptr;
        const void* view = nullptr;
        ~Mapping() {
            if (view) UnmapViewOfFile(view);
            if (section) CloseHandle(section);
            if (reader != INVALID_HANDLE_VALUE) CloseHandle(reader);
        }
        Mapping() = default;
        Mapping(const Mapping&) = delete;
        Mapping& operator=(const Mapping&) = delete;
        void Open(const fs::path& path, bool mapped) {
            reader = CreateFileW(path.c_str(), GENERIC_READ, Share, nullptr, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL, nullptr);
            Check(reader != INVALID_HANDLE_VALUE, "Open controlled compatible reader");
            if (!mapped) return;
            section = CreateFileMappingW(reader, nullptr, PAGE_READONLY, 0, 0, nullptr);
            Check(section != nullptr, "Create controlled readonly file mapping");
            view = MapViewOfFile(section, FILE_MAP_READ, 0, 0, 0);
            Check(view != nullptr, "Map controlled readonly view");
        }
        void CloseReader() {
            Check(CloseHandle(reader) != 0, "Close controlled original reader");
            reader = INVALID_HANDLE_VALUE;
        }
        void Release() {
            if (view) {
                Check(UnmapViewOfFile(view) != 0, "Unmap controlled readonly view");
                view = nullptr;
            }
            if (section) {
                Check(CloseHandle(section) != 0, "Close controlled mapping section");
                section = nullptr;
            }
            if (reader != INVALID_HANDLE_VALUE) CloseReader();
        }
        bool OldReaderIntact() const {
            if (reader == INVALID_HANDLE_VALUE) return true;
            LARGE_INTEGER start{};
            Check(SetFilePointerEx(reader, start, nullptr, FILE_BEGIN) != 0, "Rewind controlled old snapshot");
            char text[64]{};
            DWORD read = 0;
            Check(ReadFile(reader, text, sizeof(text), &read, nullptr) != 0, "Read controlled old snapshot");
            return std::string(text, read) == Old;
        }
        bool OldViewIntact() const {
            return !view || std::memcmp(view, Old, std::strlen(Old)) == 0;
        }
    };

    void Write(const fs::path& path, const char* text) {
        std::ofstream file(path, std::ios::binary);
        file.exceptions(std::ios::badbit | std::ios::failbit);
        file << text;
        file.close();
    }

    std::string ReadNewSnapshot(const fs::path& path) {
        Mapping snapshot;
        snapshot.Open(path, false);
        char text[64]{};
        DWORD read = 0;
        Check(ReadFile(snapshot.reader, text, sizeof(text), &read, nullptr) != 0,
              "Read controlled new snapshot");
        return {text, read};
    }

    std::string ObservePair(const fs::path& source, const fs::path& target) {
        return "{\"source\":" + Diagnostic::ObservePath(source)
            + ",\"target\":" + Diagnostic::ObservePath(target) + '}';
    }

    struct Error {
        bool present = false;
        bool win32 = false;
        bool has_code = false;
        int code = 0;
        std::string category;
        std::string message;
        std::string failure_class;
        void Capture(const std::exception& exception, const char* classification) {
            // Preserve the original exception code before any filesystem observations.
            const auto system = dynamic_cast<const std::system_error*>(&exception);
            present = true;
            if (system) {
                has_code = true;
                code = system->code().value();
                win32 = system->code().category() == std::system_category();
                category = system->code().category().name();
            }
            message = exception.what();
            failure_class = classification;
        }
    };

    void Record(const char* event, const char* name, bool exploratory,
                const fs::path& source, const fs::path& target, const Mapping& mapping,
                const std::string& before, const Error& error, bool published,
                bool validated, bool old_reader_intact, bool old_view_intact,
                std::uint64_t& sequence) {
        std::uint64_t ticks = 0;
        const auto utc = Diagnostic::Timestamp(ticks);
        const bool snapshots_observed = std::string(event) == "result";
        std::ostringstream out;
        out << "{\"kind\":\"publication_mapping\",\"scope\":\"controlled_readonly_mapping_not_root_cause\""
            << ",\"suite\":\"foundation.mapping_probe\",\"event\":" << Diagnostic::JsonText(event)
            << ",\"case\":" << Diagnostic::JsonText(name) << ",\"sequence\":" << ++sequence
            << ",\"utc\":" << utc << ",\"utc_filetime\":" << ticks
            << ",\"pid\":" << GetCurrentProcessId() << ",\"tid\":" << GetCurrentThreadId()
            << ",\"run_id\":" << Diagnostic::EnvironmentJson("SYMOCRAFT_FIX1_RUN_ID")
            << ",\"config\":" << Diagnostic::EnvironmentJson("SYMOCRAFT_FIX1_CONFIG")
            << ",\"source_sha\":" << Diagnostic::EnvironmentJson("SYMOCRAFT_FIX1_SOURCE_SHA")
            << ",\"exe_sha\":" << Diagnostic::EnvironmentJson("SYMOCRAFT_FIX1_EXE_SHA")
            << ",\"expected\":" << Diagnostic::JsonText(exploratory ? "exploratory" : "success")
            << ",\"operation\":\"Files::Publish\",\"api_record_kind\":\"publish_api\""
            << ",\"source\":" << Diagnostic::JsonText(Diagnostic::PathText(source))
            << ",\"destination\":" << Diagnostic::JsonText(Diagnostic::PathText(target))
            << ",\"reader_open\":" << (mapping.reader != INVALID_HANDLE_VALUE ? "true" : "false")
            << ",\"reader_share\":" << Share
            << ",\"section_open\":" << (mapping.section ? "true" : "false")
            << ",\"view_mapped\":" << (mapping.view ? "true" : "false")
            << ",\"mapping_protection\":\"PAGE_READONLY\",\"view_access\":\"FILE_MAP_READ\""
            << ",\"before\":" << before << ",\"after\":" << ObservePair(source, target)
            << ",\"publish_success\":" << (published ? "true" : "false")
            << ",\"snapshot_validated\":" << (validated ? "true" : "false")
            << ",\"old_reader_intact\":" << (snapshots_observed && mapping.reader != INVALID_HANDLE_VALUE
                ? (old_reader_intact ? "true" : "false") : "null")
            << ",\"old_view_intact\":" << (snapshots_observed && mapping.view
                ? (old_view_intact ? "true" : "false") : "null")
            << ",\"numeric_win32\":" << (error.win32 ? std::to_string(error.code) : "null")
            << ",\"error_code\":" << (error.has_code ? std::to_string(error.code) : "null")
            << ",\"error_category\":" << (error.has_code ? Diagnostic::JsonText(error.category) : "null")
            << ",\"failure_class\":" << (error.present ? Diagnostic::JsonText(error.failure_class) : "null")
            << ",\"message\":" << (error.present ? Diagnostic::JsonText(error.message) : "null")
            << ",\"historical_root_cause_confirmed\":false}";
        std::cerr << out.str() << '\n' << std::flush;
        if (!std::cerr) throw std::runtime_error("Mapping diagnostic output failed");
    }

    struct Case {
        const char* name;
        bool reader;
        bool mapping;
        bool close_reader;
        bool release_all;
        bool exploratory;
    };

    bool RunCase(const fs::path& root, const Case& scenario, std::uint64_t& sequence,
                 unsigned& calls, unsigned& exploratory_failures) {
        const auto directory = root / scenario.name;
        const auto source = directory / "status.tmp";
        const auto target = directory / "status.yaml";
        Mapping mapping;
        std::string before = "null";
        try {
            if (!fs::create_directory(directory)) throw std::runtime_error("Mapping case directory already exists");
            Write(source, New);
            Write(target, Old);
            if (scenario.reader) mapping.Open(target, scenario.mapping);
            if (scenario.release_all) mapping.Release();
            else if (scenario.close_reader) mapping.CloseReader();
            before = ObservePair(source, target);
            Error error;
            Record("begin", scenario.name, scenario.exploratory, source, target, mapping,
                   before, error, false, false, true, true, sequence);
            bool published = false;
            ++calls;
            try {
                SymoCraft::Files::Publish(source, target);
                published = true;
            } catch (const std::exception& exception) {
                error.Capture(exception, "publication_exception");
            }
            if (error.present && scenario.exploratory) ++exploratory_failures;
            Record("publication_return", scenario.name, scenario.exploratory, source, target, mapping,
                   before, error, published, false, false, false, sequence);
            const bool old_reader_intact = mapping.OldReaderIntact();
            const bool old_view_intact = mapping.OldViewIntact();
            bool validated = old_reader_intact && old_view_intact;
            if (published) validated = validated && !fs::exists(source) && ReadNewSnapshot(target) == New;
            Record("result", scenario.name, scenario.exploratory, source, target, mapping,
                   before, error, published, validated, old_reader_intact, old_view_intact, sequence);
            mapping.Release();
            return validated && (published || scenario.exploratory);
        } catch (const std::exception& exception) {
            Error error;
            error.Capture(exception, "fixture_or_snapshot_exception");
            Record("fixture_failure", scenario.name, scenario.exploratory, source, target, mapping,
                   before, error, false, false, false, false, sequence);
            return false;
        }
    }
}

int wmain(int argc, wchar_t** argv) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: publication_mapping_probe <new-absolute-output-directory>\n";
            return 64;
        }
        const fs::path root(argv[1]);
        if (!root.is_absolute()) throw std::runtime_error("Mapping output path must be absolute");
        if (!fs::create_directory(root)) throw std::runtime_error("Mapping output directory already exists");
        const Case scenarios[] = {
            {"closed-target", false, false, false, false, false},
            {"compatible-reader", true, false, false, false, false},
            {"reader-and-mapping-alive", true, true, false, false, true},
            {"reader-closed-mapping-alive", true, true, true, false, true},
            {"all-mapping-released", true, true, false, true, false},
        };
        std::uint64_t sequence = 0;
        unsigned calls = 0, processed = 0, failures = 0, exploratory_failures = 0;
        for (const auto& scenario : scenarios) {
            if (!RunCase(root, scenario, sequence, calls, exploratory_failures)) ++failures;
            ++processed;
        }
        std::cerr << "{\"kind\":\"publication_mapping\",\"scope\":\"controlled_readonly_mapping_not_root_cause\","
                     "\"event\":\"summary\",\"planned_cases\":5,\"processed_cases\":" << processed
                  << ",\"actual_publish_calls\":" << calls << ",\"fixture_or_contract_failures\":" << failures
                  << ",\"exploratory_publish_failures\":" << exploratory_failures
                  << ",\"process_exit_is_not_stability_acceptance\":true,\"historical_root_cause_confirmed\":false}\n"
                  << std::flush;
        return failures || !std::cerr ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "Mapping probe setup failed: " << error.what() << '\n';
        return 1;
    }
}
