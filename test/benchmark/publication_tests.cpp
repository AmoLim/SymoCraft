#include "platform.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace {
    void Require(bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    }
    std::string ReadText(const Benchmark::fs::path& path) {
        std::ifstream input(path, std::ios::binary);
        Require(input.is_open(), "Cannot read publication fixture");
        return {std::istreambuf_iterator<char>(input), {}};
    }
    void WriteText(const Benchmark::fs::path& path, const char* text) {
        std::ofstream output(path, std::ios::binary);
        output.exceptions(std::ios::badbit | std::ios::failbit);
        output << text;
        output.close();
    }
    template<class F> std::system_error Reject(F action, DWORD expected) {
        try { action(); }
        catch (const std::system_error& error) {
            Require(error.code() == std::error_code(static_cast<int>(expected), std::system_category()),
                    "Publication changed the native error");
            return error;
        }
        throw std::runtime_error("Invalid publication was accepted");
    }
    void Diagnostic(const std::system_error& error, const char* api,
                    const Benchmark::fs::path& source, const Benchmark::fs::path& target) {
        const std::string text(error.what());
        Require(text.find(api) != std::string::npos && text.find("flags=0") != std::string::npos
                && text.find(Benchmark::Utf8(source.wstring())) != std::string::npos
                && text.find(Benchmark::Utf8(target.wstring())) != std::string::npos,
                "Publication error lost operation or paths");
    }
    struct Calls {
        unsigned replaces = 0, moves = 0;
        DWORD replace_error = 0, move_error = 0, observed_move_error = 0;
        bool race = false;
    } calls;
    BOOL WINAPI Replace(LPCWSTR target, LPCWSTR source, LPCWSTR backup, DWORD flags, LPVOID exclude, LPVOID reserved) {
        ++calls.replaces;
        Require(flags == 0 && !backup && !exclude && !reserved, "Unexpected replace policy");
        if (calls.replace_error) { SetLastError(calls.replace_error); return FALSE; }
        return ReplaceFileW(target, source, backup, flags, exclude, reserved);
    }
    BOOL WINAPI Move(LPCWSTR source, LPCWSTR target, DWORD flags) {
        ++calls.moves;
        Require(flags == 0, "First publication may overwrite a competitor");
        if (calls.race) {
            Benchmark::Handle competitor(CreateFileW(target, GENERIC_WRITE, 7, nullptr, CREATE_NEW, 0, nullptr));
            Require(competitor.value != INVALID_HANDLE_VALUE, "Cannot create deterministic competitor");
            constexpr char text[] = "competitor"; DWORD bytes = 0;
            Require(WriteFile(competitor.value, text, sizeof(text) - 1, &bytes, nullptr) != 0
                    && bytes == sizeof(text) - 1, "Cannot write competitor");
        }
        if (calls.move_error) { SetLastError(calls.move_error); return FALSE; }
        const auto success = MoveFileExW(source, target, flags);
        calls.observed_move_error = success ? 0 : GetLastError();
        SetLastError(calls.observed_move_error);
        return success;
    }
}

int wmain(int argc, wchar_t** argv) {
    using namespace Benchmark;
    const char* phase = "setup";
    try {
        Require(argc == 2, "Missing publication output parent");
        const auto root = fs::absolute(argv[1]) / (Timestamp() + '-' + std::to_string(GetCurrentProcessId()));
        Require(fs::create_directories(root), "Publication output already exists");
        const auto path = root / "session.yaml";
        YAML::Node node; node["phase"] = "initializing";
        WriteYaml(path, node);
        Require(!fs::exists(fs::path(path.wstring() + L".tmp")), "First publication retained its temporary");
        phase = "compatible reader";
        Handle reader(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  nullptr, OPEN_EXISTING, 0, nullptr));
        Require(reader.value != INVALID_HANDLE_VALUE, "Cannot open compatible reader");
        node["phase"] = "running";
        WriteYaml(path, node);
        Require(ReadYaml(path)["phase"].as<std::string>() == "running", "New reader lost published content");
        char old[256]; DWORD count = 0;
        Require(ReadFile(reader.value, old, sizeof(old), &count, nullptr) != 0, "Old snapshot read failed");
        Require(std::string(old, count).find("initializing") != std::string::npos, "Old reader lost its snapshot");
        Require(!fs::exists(fs::path(path.wstring() + L".tmp")), "Published temporary remains");

        phase = "blocked reader and explicit caller recovery";
        const auto blocked = root / "blocked.yaml";
        WriteYaml(blocked, node);
        const auto previous = ReadText(blocked);
        {
            Handle blocker(CreateFileW(blocked.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
            Require(blocker.value != INVALID_HANDLE_VALUE, "Cannot open blocking reader");
            node["phase"] = "blocked";
            const auto error = Reject([&] { WriteYaml(blocked, node); }, ERROR_SHARING_VIOLATION);
            Diagnostic(error, "ReplaceFileW", fs::path(blocked.wstring() + L".tmp"), blocked);
            Require(ReadText(blocked) == previous && fs::exists(fs::path(blocked.wstring() + L".tmp")),
                    "Blocked publication lost old data or failed candidate");
        }
        node["phase"] = "explicit-recovery";
        WriteYaml(blocked, node);
        Require(ReadYaml(blocked)["phase"].as<std::string>() == "explicit-recovery", "Caller recovery failed");

        phase = "read-only target";
        const auto readonly = root / "readonly.yaml";
        WriteText(readonly, "old");
        const auto attributes = GetFileAttributesW(readonly.c_str());
        Require(attributes != INVALID_FILE_ATTRIBUTES && SetFileAttributesW(readonly.c_str(), attributes | FILE_ATTRIBUTE_READONLY),
                "Cannot prepare read-only fixture");
        {
            struct Restore { fs::path path; DWORD attributes; ~Restore() { SetFileAttributesW(path.c_str(), attributes); } } restore{readonly, attributes};
            Reject([&] { WriteYaml(readonly, node); }, ERROR_ACCESS_DENIED);
            Require(ReadText(readonly) == "old" && fs::exists(fs::path(readonly.wstring() + L".tmp")),
                    "Read-only rejection damaged files");
        }

        const auto metadata = root / L"paths \u4e2d\u6587";
        fs::create_directory(metadata);
        const auto source = metadata / "candidate.tmp", target = metadata / "target.yaml";
        phase = "missing source";
        WriteText(target, "old");
        calls = {};
        auto error = Reject([&] { Detail::PublishYamlFile(source, target, Replace, Move); }, ERROR_FILE_NOT_FOUND);
        Require(calls.replaces == 1 && calls.moves == 1 && ReadText(target) == "old", "Missing-source fallback changed old target");
        Diagnostic(error, "MoveFileExW", source, target);

        phase = "first publication competitor";
        const auto competitor = metadata / "competitor.yaml";
        WriteText(source, "new");
        calls = {}; calls.race = true;
        try { Detail::PublishYamlFile(source, competitor, Replace, Move); throw std::runtime_error("Competitor was overwritten"); }
        catch (const std::system_error& conflict) {
            Require(calls.observed_move_error == ERROR_ALREADY_EXISTS || calls.observed_move_error == ERROR_FILE_EXISTS,
                    "Unexpected native race result");
            Require(conflict.code() == std::error_code(static_cast<int>(calls.observed_move_error), std::system_category()),
                    "Race lost original native error");
            Diagnostic(conflict, "MoveFileExW", source, competitor);
        }
        Require(calls.replaces == 1 && calls.moves == 1 && ReadText(competitor) == "competitor"
                && ReadText(source) == "new", "First publication retried or clobbered a competing file");

        phase = "failed replacement propagation";
        for (const DWORD code : {DWORD(ERROR_ACCESS_DENIED), DWORD(ERROR_UNABLE_TO_REMOVE_REPLACED),
                                 DWORD(ERROR_UNABLE_TO_MOVE_REPLACEMENT), DWORD(ERROR_UNABLE_TO_MOVE_REPLACEMENT_2)}) {
            calls = {}; calls.replace_error = code;
            error = Reject([&] { Detail::PublishYamlFile(source, target, Replace, Move); }, code);
            Require(calls.replaces == 1 && calls.moves == 0, "Hard replace failure fell back or retried");
            Diagnostic(error, "ReplaceFileW", source, target);
        }
        phase = "failed first move propagation";
        calls = {}; calls.replace_error = ERROR_FILE_NOT_FOUND; calls.move_error = ERROR_SHARING_VIOLATION;
        error = Reject([&] { Detail::PublishYamlFile(source, metadata / "absent.yaml", Replace, Move); }, ERROR_SHARING_VIOLATION);
        Require(calls.replaces == 1 && calls.moves == 1 && fs::exists(source), "First move failure retried or erased input");
        Diagnostic(error, "MoveFileExW", source, metadata / "absent.yaml");
        Require(std::string(error.what()).find("replace_error=2") != std::string::npos, "First move lost replace context");
        std::cout << "benchmark publication passed\n";
        return 0;
    } catch (const std::system_error& error) {
        std::cerr << "phase=" << phase << "; " << error.what() << "; native_code=" << error.code().value()
                  << "; category=" << error.code().category().name() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "phase=" << phase << "; " << error.what() << '\n';
    }
    return 1;
}
