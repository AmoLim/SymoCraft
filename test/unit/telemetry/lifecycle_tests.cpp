#include <symocraft/telemetry/performance.h>
#include "export_probe.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace {
    using namespace SymoCraft::Performance;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    template<class Call> void Reject(Call&& call) {
        try { call(); } catch (const std::logic_error&) { return; }
        throw std::runtime_error("Sealed operation was accepted");
    }
    std::string Read(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
    SessionConfig Options(const std::filesystem::path& path) {
        SessionConfig options; options.output_directory = path.string(); return options;
    }
    Frame Sample() { Frame frame; frame.measured = frame.focused = true; frame.frame_ms = 10; return frame; }
    void SealAndRetry(const std::filesystem::path& parent) {
        Session session(Options(parent / "sealed"), Clock::now());
        Reject([&] { session.Export(); });
        session.Status("awaiting-foreground");
        Require(Read(session.Directory() / "status.yaml").find("awaiting-foreground") != std::string::npos,
            "Approved foreground preparation phase was rejected or not published");
        Reject([&] { session.Status("finished"); });
        Reject([&] { session.Status("exporting"); });
        session.Status("warmup");
        session.SetMetadata("owned", "before"); session.SetStartup("test", 1);
        session.Add(Sample()); session.Complete();
        const FinalFacts facts{true, true, true, {}, {}};
        session.Seal(facts); session.Seal(facts);
        Require(session.State() == RecordState::Sealed && session.LastExport().state == ExportState::NotAttempted,
            "Sealing conflated collection and export state");
        Reject([&] { session.Seal({false, true, true, "changed", {}}); });
        Reject([&] { session.Add(Sample()); });
        Reject([&] { session.Add(Memory{}); });
        Reject([&] { session.SetGpu(0, 1); });
        Reject([&] { session.SetGpuSample(0, 1); });
        Reject([&] { session.SetRendererInfo({}); });
        Reject([&] { session.AddStartupUpload({}); });
        Reject([&] { session.SetMetadata("owned", "after"); });
        Reject([&] { session.SetStartup("test", 2); });
        Reject([&] { session.Complete(); });
        Reject([&] { session.Invalidate("late"); });
        Reject([&] { session.Status("sampling"); });
        Reject([&] { session.Status("awaiting-foreground"); });
        Require(session.Metadata()["owned"].as<std::string>() == "before" && session.Startup()["test"].as<int>() == 1,
            "Sealed metadata changed");
        std::filesystem::create_directory(session.Directory() / "frames.csv");
        const auto failed = session.Export();
        Require(failed.state == ExportState::Failed && failed.stage == "frames" &&
            failed.path.filename() == "frames.csv" && session.State() == RecordState::Sealed,
            "Failed export lost its stage or reopened collection");
        std::filesystem::remove(session.Directory() / "frames.csv"); // Only this empty obstruction fixture.
        const auto saved = session.Export();
        Require(saved.state == ExportState::Saved && saved.valid_run, "Explicit export retry did not save");
        const auto summary = Read(session.Directory() / "summary.yaml");
        { std::ofstream marker(session.Directory() / "frames.csv"); marker << "external marker"; }
        const auto repeated = session.Export();
        Require(repeated.state == ExportState::Saved && repeated.valid_run &&
            Read(session.Directory() / "frames.csv") == "external marker" &&
            Read(session.Directory() / "summary.yaml") == summary, "Saved export rewrote files");
    }
    void FinalPublicationFailure(const std::filesystem::path& parent) {
        Session session(Options(parent / "terminal"), Clock::now());
        session.Add(Sample()); session.Complete(); session.Seal({true, true, true, {}, {}});
        struct BlockingReader { HANDLE handle = INVALID_HANDLE_VALUE; } reader;
        Detail::SetExportProbe([](void* context, const std::filesystem::path& directory, const char* phase) {
            if (std::string_view(phase) != "finished") return;
            auto& state = *static_cast<BlockingReader*>(context);
            state.handle = CreateFileW((directory / "status.yaml").c_str(), GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (state.handle == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open terminal publication blocker");
        }, &reader);
        const auto failed = session.Export();
        Detail::SetExportProbe(nullptr, nullptr);
        if (reader.handle != INVALID_HANDLE_VALUE) CloseHandle(reader.handle);
        Require(failed.state == ExportState::Failed && failed.stage == "finished" &&
            failed.native_error.value() == ERROR_SHARING_VIOLATION && failed.path.filename() == "status.yaml",
            "Final status failure lost the original native code/stage");
        Require(std::filesystem::exists(session.Directory() / "summary.yaml") &&
            Read(session.Directory() / "status.yaml").find("exporting") != std::string::npos &&
            Read(session.Directory() / "status.tmp").find("finished") != std::string::npos,
            "Late failure discarded partial files or falsely published finished");
        Require(session.State() == RecordState::Sealed && session.Export().state == ExportState::Saved,
            "Late failure could not be explicitly retried");
    }
}
int main(int argc, char** argv) {
    try {
        Require(argc == 2, "Expected test output parent");
        const auto parent = std::filesystem::absolute(argv[1]) / std::to_string(Clock::now().time_since_epoch().count());
        std::filesystem::create_directories(parent);
        SealAndRetry(parent); FinalPublicationFailure(parent);
        std::cout << "Sealed mutations, owned export failures, explicit retry and saved idempotence passed\n";
        return 0;
    } catch (const std::exception& error) {
        Detail::SetExportProbe(nullptr, nullptr);
        std::cerr << error.what() << '\n'; return 1;
    }
}
