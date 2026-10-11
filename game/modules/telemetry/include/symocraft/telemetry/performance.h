#pragma once
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
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
        // Stable renderer identity is separate from the legacy CSV row number.
        std::optional<std::uint64_t> frame_id;
        std::optional<double> cpu_prepare_ms, cpu_submit_ms;
        std::string gpu_missing_reason;
        std::uint64_t resource_upload_bytes{}, render_upload_bytes{}, upload_calls{};
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

    struct RendererInfo {
        std::string backend, device_name, device_identity, driver_version, api_version;
        std::string framebuffer_format, actual_present_mode, gpu_time_scope;
        unsigned framebuffer_width{}, framebuffer_height{}, actual_samples{};
        bool validation_enabled{}, texture_arrays{}, selection_boxes{}, screenshots{}, gpu_timestamps{};
    };

    struct UploadStatistics {
        double cpu_ms{};
        std::uint64_t bytes{}, calls{};
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
        void SetGpuSample(std::uint64_t frame_id, std::optional<double> milliseconds,
                          const std::string& missing_reason = {});
        void SetRendererInfo(const RendererInfo& info);
        void AddStartupUpload(UploadStatistics statistics);
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
        std::unordered_map<std::uint64_t, std::size_t> renderer_frames_;
        std::vector<bool> gpu_resolved_;
        std::vector<Memory> memory_;
        std::vector<std::string> invalid_reasons_;
        bool allow_unfocused_ = false;
        bool renderer_info_set_ = false;
        UploadStatistics startup_upload_;
    };
}
