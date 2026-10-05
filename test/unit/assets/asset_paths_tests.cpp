#include "symocraft/assets/asset_paths.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
namespace Assets = SymoCraft::Assets;

namespace
{
    void Require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    class TemporaryDirectory
    {
    public:
        TemporaryDirectory()
        {
            parent_ = fs::weakly_canonical(fs::temp_directory_path());
            path_ = parent_ / (L"symocraft-assets-" + std::to_wstring(GetCurrentProcessId()) +
                               L"-" + std::to_wstring(GetTickCount64()));
            Require(fs::create_directory(path_), "Cannot create isolated test directory");
        }

        ~TemporaryDirectory()
        {
            // Only remove the isolated direct child that this instance successfully created.
            if (path_.parent_path() == parent_ &&
                path_.filename().native().starts_with(L"symocraft-assets-"))
            {
                std::error_code error;
                fs::remove_all(path_, error);
                if (error)
                    std::cerr << "Test cleanup failed: " << error.message() << '\n';
            }
        }

        const fs::path& Path() const { return path_; }

    private:
        fs::path parent_;
        fs::path path_;
    };

    void CreatePackage(const fs::path& package, bool complete)
    {
        fs::create_directories(package);
        fs::copy_file(Assets::ExecutableDirectory() / "asset_paths_tests.exe", package / "asset_paths_tests.exe");
        for (const auto relative : Assets::RequiredFiles())
        {
            if (!complete && relative == "textures/texture_atlas.png")
                continue;
            const auto path = package / "assets" / relative;
            fs::create_directories(path.parent_path());
            std::ofstream output(path, std::ios::binary);
            output << "Asset existence test fixture; not game content.\n";
            output.close();
            Require(static_cast<bool>(output), "Cannot write test asset fixture");
        }
    }

    void RunPackageProbe(const fs::path& package, const fs::path& working_directory, bool complete)
    {
        const auto executable = package / "asset_paths_tests.exe";
        std::wstring command = L"\"" + executable.native() + L"\" --probe \"" +
                               (package / "assets").native() + L"\" " + (complete ? L"complete" : L"missing");
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                            nullptr, working_directory.c_str(), &startup, &process))
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                    "Cannot start relocated asset probe");

        const DWORD wait_result = WaitForSingleObject(process.hProcess, 10000);
        DWORD exit_code = 1;
        const bool exited = wait_result == WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess, &exit_code);
        if (!exited)
        {
            TerminateProcess(process.hProcess, 124);
            WaitForSingleObject(process.hProcess, INFINITE);
        }
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        Require(exited, "Relocated asset probe did not complete within its deadline");
        if (exit_code != 0)
            throw std::runtime_error("Relocated asset probe failed with exit code " +
                                     std::to_string(exit_code));
    }

    void CheckPathContracts()
    {
        const auto working_directory = fs::current_path();
        const auto root = fs::weakly_canonical(Assets::ExecutableDirectory() / "assets");
        Require(fs::weakly_canonical(Assets::Root()) == root, "Assets root is not next to the executable");
        const auto shader = Assets::Resolve("shaders/vs_BlockShader.glsl");
        Require(shader == root / "shaders" / "vs_BlockShader.glsl", "Valid shader path resolved incorrectly");
        Require(Assets::Resolve("shaders/./vs_BlockShader.glsl") == shader, "Dot normalization failed");
        Require(Assets::Resolve("not-present/file.txt") == root / "not-present" / "file.txt",
                "Resolve must not require a file to exist");

        const std::vector<fs::path> invalid{
            "", ".", "..", "../outside.txt", "shaders/../../outside.txt",
            "shaders/../vs_BlockShader.glsl", L"..\\outside.txt", L"C:\\outside.txt",
            L"C:outside.txt", L"\\outside.txt", L"\\\\server\\share\\outside.txt",
            L"textures/texture_atlas.png:stream", std::wstring(L"file\0hidden", 11)
        };
        for (const auto& path : invalid)
        {
            bool rejected = false;
            try
            {
                static_cast<void>(Assets::Resolve(path));
            }
            catch (const std::invalid_argument&)
            {
                rejected = true;
            }
            Require(rejected, "An invalid or escaping path was accepted");
        }
        Require(fs::current_path() == working_directory, "Asset resolution changed the working directory");
    }

    void CheckProbe(const fs::path& expected_root, bool complete)
    {
        const auto original_cwd = fs::current_path();
        Require(fs::equivalent(Assets::Root(), expected_root), "Relocated root points to the wrong package");
        Require(!fs::equivalent(Assets::ExecutableDirectory(), original_cwd),
                "Probe must run from a directory different from its executable");
        CheckPathContracts();
        std::ostringstream errors;
        Require(Assets::CheckRequiredAssets(errors) == complete, "Required-asset result does not match package");
        if (complete)
            Require(errors.str().empty(), "A complete package produced asset errors");
        else
            Require(errors.str().find("textures/texture_atlas.png") != std::string::npos,
                    "Missing-file diagnostic did not identify the required file");
        Require(fs::current_path() == original_cwd, "Asset preflight changed the working directory");
    }
}

int wmain(int argc, wchar_t* argv[])
{
    try
    {
        if (argc == 4 && std::wstring_view(argv[1]) == L"--probe")
        {
            CheckProbe(argv[2], std::wstring_view(argv[3]) == L"complete");
            return 0;
        }
        Require(argc == 1, "Unexpected asset test arguments");
        CheckPathContracts();
        std::ostringstream errors;
        Require(Assets::CheckRequiredAssets(errors), "Build did not stage all six required assets");
        Require(errors.str().empty(), "Staged assets produced errors");

        TemporaryDirectory temporary;
        const auto working_directory = temporary.Path() / "unrelated working directory";
        fs::create_directory(working_directory);

        const auto complete_package = temporary.Path() / "package with spaces";
        CreatePackage(complete_package, true);
        RunPackageProbe(complete_package, working_directory, true);

        const auto missing_package = temporary.Path() / "missing file package";
        CreatePackage(missing_package, false);
        RunPackageProbe(missing_package, working_directory, false);

        // Only the path module is Unicode-aware; this does not exercise legacy game loaders.
        const auto unicode_package = temporary.Path() / L"package-\u6D4B\u8BD5";
        CreatePackage(unicode_package, true);
        RunPackageProbe(unicode_package, working_directory, true);

        std::cout << "Asset path contracts and relocated-package probes passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Asset path test failed: " << error.what() << '\n';
        return 1;
    }
}
