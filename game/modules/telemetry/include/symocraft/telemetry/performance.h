#pragma once
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "symocraft/foundation/document.h"

namespace SymoCraft::Performance {
    using Clock = std::chrono::steady_clock;
    inline double Milliseconds(Clock::time_point start, Clock::time_point end = Clock::now())
    { return std::chrono::duration<double, std::milli>(end - start).count(); }

    struct Frame {
        double elapsed{}, frame_ms{}, simulation_delta_ms{}, event_ms{}, simulation_ms{}, mesh_ms{};
        double pack_ms{}, render_cpu_ms{}, upload_cpu_ms{}, present_ms{}, instrumentation_ms{};
        std::optional<double> gpu_draw_ms;
        std::uint64_t rebuilt_chunks{}, vertices{}, upload_bytes{}, draw_calls{}, edits{};
        double edit_lateness_ms{}, x{}, y{}, z{}, yaw{};
        bool measured{}, focused{};
    };
    struct Memory {
        double elapsed{};
        std::optional<std::uint64_t> working_set_bytes, private_bytes;
        std::optional<std::uint64_t> device_dedicated_kib, device_available_kib;
    };
    Data::Value Statistics(std::vector<double> values);

    struct SessionConfig {
        std::string output_directory;
        std::string focus_policy = "strict";
        std::string benchmark;
        unsigned width = 1920, height = 1080;
        unsigned warmup_seconds = 60, sample_seconds = 180;
        bool vsync = false;
    };

    class Session {
    public:
        Session(const SessionConfig& options, Clock::time_point entry);
        Session(const Session&) = delete;
        Session& operator=(const Session&) = delete;
        std::size_t NextFrame() const { return frames_.size(); }
        void Add(Frame frame);
        void Add(Memory memory) { memory_.push_back(memory); }
        void SetGpu(std::size_t frame, double milliseconds);
        void Invalidate(const std::string& reason);
        const std::filesystem::path& Directory() const { return directory_; }
        bool Export(bool normal_exit);
        void Status(const char* phase, double elapsed = 0);
        Data::Value metadata;
        Data::Value startup;
        bool completed = false;
        Clock::time_point entry;
    private:
        std::filesystem::path directory_;
        std::vector<Frame> frames_;
        std::vector<Memory> memory_;
        std::vector<std::string> invalid_reasons_;
        bool allow_unfocused_ = false;
    };
}
