#include "symocraft/foundation/files.h"
#include "../../support/publication_diagnostics.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace {
    void Require(bool condition, const char* message) {
        if (!condition) throw SymoCraft::Test::ContractFailure(message);
    }
    void Write(const std::filesystem::path& path, const std::string& text) {
        std::ofstream file(path, std::ios::binary);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file << text;
        file.close();
    }
    std::string Read(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        file.exceptions(std::ios::badbit);
        Require(file.is_open(), "Cannot read published file");
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
    class Reader {
    public:
        Reader(const std::filesystem::path& path, DWORD share)
            : handle_(CreateFileW(path.c_str(), GENERIC_READ, share, nullptr, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL, nullptr)) {
            if (handle_ == INVALID_HANDLE_VALUE) {
                const auto error = GetLastError();
                throw std::system_error(static_cast<int>(error), std::system_category(), "Cannot open publication reader");
            }
        }
        ~Reader() { CloseHandle(handle_); }
        Reader(const Reader&) = delete;
        Reader& operator=(const Reader&) = delete;
        std::string Read() const {
            char buffer[128];
            DWORD bytes = 0;
            if (!ReadFile(handle_, buffer, sizeof(buffer), &bytes, nullptr)) {
                const auto error = GetLastError();
                throw std::system_error(static_cast<int>(error), std::system_category(), "Old publication reader failed");
            }
            return {buffer, bytes};
        }
    private:
        HANDLE handle_;
    };
    template<class Verify>
    void PublishCase(SymoCraft::Test::PublicationDiagnostics& diagnostic, const char* phase,
                     const std::filesystem::path& source, const std::filesystem::path& target,
                     Verify verify, int iteration = -1, DWORD share = 0,
                     const char* held_reader = "none", int expected_error = -1) {
        diagnostic.Begin(phase, "publish", source, target, iteration, share, held_reader, expected_error);
        try { SymoCraft::Files::Publish(source, target); }
        catch (const std::system_error& error) {
            if (expected_error < 0 || error.code() != std::error_code(expected_error, std::system_category())) {
                diagnostic.Failure("unexpected_failure", error);
                throw;
            }
            diagnostic.Failure("expected_rejection", error);
            verify();
            return;
        } catch (const std::exception& error) {
            diagnostic.Failure("unexpected_failure", error);
            throw;
        } catch (...) {
            diagnostic.UnknownFailure("unexpected_failure");
            throw;
        }
        Require(expected_error < 0, "Invalid publication was accepted");
        verify();
        diagnostic.Success();
    }

    void SetAttributes(const std::filesystem::path& path, DWORD attributes) {
        if (!SetFileAttributesW(path.c_str(), attributes)) {
            const auto error = GetLastError();
            throw std::system_error(static_cast<int>(error), std::system_category(), "Cannot set test file attributes");
        }
    }
}

int main(int argc, char** argv) {
    SymoCraft::Test::PublicationDiagnostics diagnostic("foundation.files");
    try {
        Require(argc == 2, "Missing publication test output parent");
        const auto parent = std::filesystem::absolute(argv[1]);
        diagnostic.Context("setup", "filesystem_setup", {}, parent);
        std::filesystem::create_directories(parent);
        const auto root = parent / (std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
                                    + '-' + std::to_string(GetCurrentProcessId()));
        Require(std::filesystem::create_directory(root), "Publication test directory already exists");
        const auto source = root / "status.tmp";
        const auto target = root / "status.yaml";
        diagnostic.Context("first_publication", "fixture_write", source, target);
        Write(source, "initializing\n");
        PublishCase(diagnostic, "first_publication", source, target, [&] {
            Require(!std::filesystem::exists(source) && Read(target) == "initializing\n", "First publication failed");
        });
        std::cout << "First publication passed.\n";

        diagnostic.Context("closed_target", "fixture_write", source, target);
        Write(source, "exporting\n");
        PublishCase(diagnostic, "closed_target", source, target, [&] {
            Require(!std::filesystem::exists(source) && Read(target) == "exporting\n", "Closed-target replacement failed");
        });
        std::cout << "Closed-target replacement passed.\n";

        for (unsigned i = 0; i < 100; ++i) {
            diagnostic.Context("shared_reader", "fixture_prepare", source, target, static_cast<int>(i),
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, "target");
            const auto previous = Read(target);
            Reader reader(target, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
            const auto next = std::to_string(i) + " finished\n";
            Write(source, next);
            PublishCase(diagnostic, "shared_reader", source, target, [&] {
                Require(!std::filesystem::exists(source) && Read(target) == next, "Shared-reader publication failed");
                Require(reader.Read() == previous, "Existing reader observed a truncated or changed old file");
            }, static_cast<int>(i), FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, "target");
        }
        std::cout << "100 shared-delete-reader replacements passed.\n";

        diagnostic.Context("blocked_target", "fixture_prepare", source, target, -1, FILE_SHARE_READ | FILE_SHARE_WRITE, "target");
        Write(source, "blocked\n");
        const auto previous = Read(target);
        {
            Reader reader(target, FILE_SHARE_READ | FILE_SHARE_WRITE);
            PublishCase(diagnostic, "blocked_target", source, target, [&] {
                Require(Read(source) == "blocked\n" && Read(target) == previous, "Blocked publication damaged its files");
            }, -1, FILE_SHARE_READ | FILE_SHARE_WRITE, "target", ERROR_SHARING_VIOLATION);
        }
        PublishCase(diagnostic, "blocking_reader_closed", source, target, [&] {
            Require(!std::filesystem::exists(source) && Read(target) == "blocked\n", "Publication failed after blocking reader closed");
        });
        std::cout << "Non-delete-sharing reader rejection and close recovery passed.\n";

        diagnostic.Context("blocked_source", "fixture_prepare", source, target, -1, FILE_SHARE_READ | FILE_SHARE_WRITE, "source");
        Write(source, "source-blocked\n");
        {
            Reader reader(source, FILE_SHARE_READ | FILE_SHARE_WRITE);
            PublishCase(diagnostic, "blocked_source", source, target, [&] {
                Require(Read(source) == "source-blocked\n" && Read(target) == "blocked\n", "Blocked source damaged its files");
            }, -1, FILE_SHARE_READ | FILE_SHARE_WRITE, "source", ERROR_SHARING_VIOLATION);
        }
        std::cout << "Open temporary-file rejection passed.\n";

        diagnostic.Context("readonly_target", "fixture_attributes", source, target);
        SetAttributes(target, FILE_ATTRIBUTE_READONLY);
        try {
            PublishCase(diagnostic, "readonly_target", source, target, [&] {
                Require(Read(source) == "source-blocked\n" && Read(target) == "blocked\n", "Read-only rejection damaged its files");
            }, -1, 0, "none", ERROR_ACCESS_DENIED);
        } catch (...) {
            SetFileAttributesW(target.c_str(), FILE_ATTRIBUTE_NORMAL);
            throw;
        }
        SetAttributes(target, FILE_ATTRIBUTE_NORMAL);
        std::cout << "Read-only target rejection passed.\n";

        const auto directory_target = root / "directory.yaml";
        diagnostic.Context("directory_target", "fixture_prepare", source, directory_target);
        Require(std::filesystem::create_directory(directory_target), "Cannot create directory-target fixture");
        PublishCase(diagnostic, "directory_target", source, directory_target, [&] {
            Require(std::filesystem::is_directory(directory_target) && Read(source) == "source-blocked\n", "Directory rejection damaged its files");
        }, -1, 0, "none", ERROR_ACCESS_DENIED);
        std::cout << "Directory target rejection passed.\n";

        PublishCase(diagnostic, "missing_source", root / "missing.tmp", target, [&] {
            Require(Read(target) == "blocked\n", "Missing source changed destination");
        }, -1, 0, "none", ERROR_FILE_NOT_FOUND);
        std::cout << "Missing-source rejection passed.\n";
        std::cout << "Publication contracts passed; old reader identity and hard failures retained.\n";
        diagnostic.SuiteSuccess();
        return 0;
    } catch (const std::system_error& error) {
        diagnostic.Failure("outer_failure", error);
        std::cerr << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        diagnostic.Failure("outer_failure", error);
        std::cerr << error.what() << '\n';
        return 1;
    } catch (...) {
        diagnostic.UnknownFailure("outer_failure");
        return 1;
    }
}
