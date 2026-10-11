#include "symocraft/telemetry/performance.h"
#include "../../support/publication_diagnostics.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <system_error>

namespace {
    void Require(bool value, const char* message) { if (!value) throw SymoCraft::Test::ContractFailure(message); }
    class StatusReader {
    public:
        explicit StatusReader(const std::filesystem::path& path,
                              DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
            : handle_(CreateFileW(path.c_str(), GENERIC_READ,
                                  share,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)) {
            if (handle_ == INVALID_HANDLE_VALUE) {
                const auto error = GetLastError();
                throw std::system_error(static_cast<int>(error), std::system_category(), "Cannot open status reader");
            }
        }
        ~StatusReader() { CloseHandle(handle_); }
        StatusReader(const StatusReader&) = delete;
        StatusReader& operator=(const StatusReader&) = delete;
        std::string Read() const {
            char buffer[256];
            DWORD bytes = 0;
            if (!ReadFile(handle_, buffer, sizeof(buffer), &bytes, nullptr)) {
                const auto error = GetLastError();
                throw std::system_error(static_cast<int>(error), std::system_category(), "Cannot read old status snapshot");
            }
            return {buffer, bytes};
        }
    private:
        HANDLE handle_;
    };

    template<class Action>
    void SessionOperation(SymoCraft::Test::PublicationDiagnostics& diagnostic, const char* phase,
                          const char* operation, const std::filesystem::path& output, Action action,
                          DWORD reader_share = 0, const char* held_reader = "none", int expected_error = -1) {
        diagnostic.Begin(phase, operation, output / "status.tmp", output / "status.yaml",
                         -1, reader_share, held_reader, expected_error);
        try { action(); }
        catch (const std::system_error& error) {
            if (expected_error >= 0 && error.code() == std::error_code(expected_error, std::system_category())) {
                diagnostic.Failure("expected_rejection", error);
                return;
            }
            diagnostic.Failure("unexpected_failure", error);
            throw;
        } catch (const YAML::Exception& error) {
            diagnostic.Failure("unexpected_failure", error, "serialization_exception");
            throw;
        } catch (const std::exception& error) {
            diagnostic.Failure("unexpected_failure", error);
            throw;
        } catch (...) {
            diagnostic.UnknownFailure("unexpected_failure");
            throw;
        }
        Require(expected_error < 0, "Blocked telemetry status publication was accepted");
        diagnostic.Success();
    }
}
int main(int argc, char** argv)
{
    using namespace SymoCraft;
    Test::PublicationDiagnostics diagnostic("performance.export");
    try {
        Require(argc == 2, "Missing test output parent");
        std::vector<double> values;
        for (int i = 100; i > 0; --i) values.push_back(i);
        const auto stats = Performance::Statistics(values);
        Require(stats["p95"].as<double>() == 95 && stats["p99"].as<double>() == 99 && stats["mean"].as<double>() == 50.5, "Percentile definition changed");
        Require(Performance::Statistics({})["count"].as<int>() == 0, "Missing samples were fabricated");
        bool rejected = false;
        try { Performance::Statistics({std::numeric_limits<double>::infinity()}); }
        catch (const std::invalid_argument&) { rejected = true; }
        Require(rejected, "Invalid metric accepted");
        const auto parent = std::filesystem::absolute(argv[1]);
        diagnostic.Context("setup", "filesystem_setup", {}, parent);
        std::filesystem::create_directories(parent);
        const auto path = parent / (std::to_string(Performance::Clock::now().time_since_epoch().count())
                                    + '-' + std::to_string(GetCurrentProcessId()));
        const auto path_text = path.string();
        Performance::SessionConfig options;
        options.benchmark = "edit"; options.output_directory = path_text;
        diagnostic.Begin("init_normal", "telemetry_initialize", path / "status.tmp", path / "status.yaml");
        Performance::Session session(options, Performance::Clock::now());
        diagnostic.Success();
        StatusReader initial_status(path / "status.yaml");
        SessionOperation(diagnostic, "warmup", "telemetry_status", path, [&] { session.Status("warmup"); },
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, "target");
        Performance::Frame warmup;
        warmup.frame_ms = 1000; warmup.focused = true;
        session.Add(warmup);
        SessionOperation(diagnostic, "sampling", "telemetry_status", path, [&] { session.Status("sampling", 1.0); },
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, "target");
        Performance::Frame measured;
        measured.measured = true; measured.focused = true; measured.frame_ms = 10;
        session.Add(measured);
        measured.frame_ms = 200; measured.edits = 1; measured.rebuilt_chunks = 2;
        session.Add(measured);
        session.SetGpu(1, 2.5);
        session.completed = true;
        SessionOperation(diagnostic, "export_normal", "telemetry_export", path, [&] { session.Export(true); },
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, "target");
        Require(YAML::Load(initial_status.Read())["phase"].as<std::string>() == "initializing", "Existing status reader lost its old snapshot");
        Require(YAML::LoadFile((path / "status.yaml").string())["phase"].as<std::string>() == "finished", "New status reader did not observe finished");
        Require(!std::filesystem::exists(path / "status.tmp"), "Successful export retained uncommitted status");
        const auto report = YAML::LoadFile((path / "summary.yaml").string());
        Require(report["valid_run"].as<bool>() && report["frame_ms"]["count"].as<int>() == 2, "Warmup leaked into summary");
        Require(report["frame_ms"]["p95"].as<double>() == 200 && report["sample_edits"].as<int>() == 1, "Edit slow frame was discarded");
        Require(report["gpu_missing_sample_frames"].as<int>() == 1, "Unavailable GPU result was fabricated");
        rejected = false;
        diagnostic.Begin("duplicate_directory", "telemetry_initialize", path / "status.tmp", path / "status.yaml",
                         -1, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, "target", -2);
        try { Performance::Session duplicate(options, Performance::Clock::now()); }
        catch (const std::system_error& error) {
            diagnostic.Failure("unexpected_failure", error);
            throw;
        } catch (const std::runtime_error& error) {
            diagnostic.Failure("expected_standard_rejection", error);
            rejected = true;
        }
        Require(rejected, "Existing result directory was overwritten");
        session.Invalidate("test-interference");
        session.completed = false;
        SessionOperation(diagnostic, "export_interrupted", "telemetry_export", path, [&] { session.Export(false); },
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, "target");
        Require(!YAML::LoadFile((path / "summary.yaml").string())["valid_run"].as<bool>(), "Interrupted capture marked valid");
        std::ifstream csv(path / "frames.csv");
        std::string line; int lines = 0;
        while (std::getline(csv, line)) ++lines;
        Require(lines == 4, "Raw frames missing");
        for (const auto policy : {"strict", "allow-unfocused"}) {
            const auto focus_path = path.string() + policy;
            options.output_directory = focus_path; options.focus_policy = policy;
            const auto initialize_phase = std::string("init_") + policy;
            diagnostic.Begin(initialize_phase.c_str(), "telemetry_initialize", std::filesystem::path(focus_path) / "status.tmp",
                             std::filesystem::path(focus_path) / "status.yaml");
            Performance::Session focus_session(options, Performance::Clock::now());
            diagnostic.Success();
            measured.focused = false; measured.frame_ms = 100;
            focus_session.Add(measured);
            measured.focused = true; focus_session.Add(measured);
            focus_session.completed = true;
            const auto export_phase = std::string("export_") + policy;
            SessionOperation(diagnostic, export_phase.c_str(), "telemetry_export", focus_path, [&] {
                Require(focus_session.Export(true) == (options.focus_policy == "allow-unfocused"), "Wrong focus validity policy");
            });
            const auto focus_report = YAML::LoadFile((std::filesystem::path(focus_path) / "summary.yaml").string());
            Require(focus_report["focus"]["transitions"].as<int>() == 1 && focus_report["focus"]["unfocused_frames"].as<int>() == 1, "Focus accounting failed");
            focus_session.Invalidate("framebuffer-changed-or-minimized");
            const auto invalid_phase = std::string("export_invalid_") + policy;
            SessionOperation(diagnostic, invalid_phase.c_str(), "telemetry_export", focus_path, [&] {
                Require(!focus_session.Export(true), "Allow-unfocused accepted minimized capture");
            });
        }

        const auto blocked_path = std::filesystem::path(path.string() + "-blocked");
        options.output_directory = blocked_path.string(); options.focus_policy = "strict";
        diagnostic.Begin("init_blocked_fixture", "telemetry_initialize", blocked_path / "status.tmp", blocked_path / "status.yaml");
        Performance::Session blocked_session(options, Performance::Clock::now());
        diagnostic.Success();
        blocked_session.Add(measured); blocked_session.completed = true;
        {
            StatusReader blocking_status(blocked_path / "status.yaml", FILE_SHARE_READ | FILE_SHARE_WRITE);
            SessionOperation(diagnostic, "blocked_exporting", "telemetry_export", blocked_path,
                             [&] { blocked_session.Export(true); }, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             "target", ERROR_SHARING_VIOLATION);
            Require(YAML::LoadFile((blocked_path / "status.tmp").string())["phase"].as<std::string>() == "exporting",
                    "Rejected export lost its failed status stage");
            Require(YAML::LoadFile((blocked_path / "status.yaml").string())["phase"].as<std::string>() == "initializing",
                    "Rejected export incorrectly advanced its published status");
            Require(!std::filesystem::exists(blocked_path / "summary.yaml") && !std::filesystem::exists(blocked_path / "frames.csv"),
                    "Rejected exporting status continued to write result files");
        }
        SessionOperation(diagnostic, "blocking_status_reader_closed", "telemetry_export", blocked_path,
                         [&] { Require(blocked_session.Export(true), "Status did not recover after blocking reader closed"); });
        Require(YAML::LoadFile((blocked_path / "status.yaml").string())["phase"].as<std::string>() == "finished"
                && !std::filesystem::exists(blocked_path / "status.tmp"), "Recovered export did not commit finished");
        std::cout << "Percentiles, warmup, slow frames, missing GPU values and partial exports passed.\n";
        diagnostic.SuiteSuccess();
        return 0;
    } catch (const std::system_error& error) {
        diagnostic.Failure("outer_failure", error);
        std::cerr << error.what() << '\n'; return 1;
    } catch (const YAML::Exception& error) {
        diagnostic.Failure("outer_failure", error, "serialization_exception");
        std::cerr << error.what() << '\n'; return 1;
    } catch (const std::exception& error) {
        diagnostic.Failure("outer_failure", error);
        std::cerr << error.what() << '\n'; return 1;
    } catch (...) {
        diagnostic.UnknownFailure("outer_failure"); return 1;
    }
}
