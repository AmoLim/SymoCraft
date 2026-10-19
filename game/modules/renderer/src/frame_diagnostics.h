#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace SymoCraft::Rendering::Detail {
// Private creating-thread diagnostics; ordinary game runs never install a sink.
enum class FramePhase : unsigned {
    FacadePrepare, BackendSetup, SlotWaitCollect, ResourceRetire, CpuPack,
    SelectionAllocate, SelectionWrite, UniformAllocate, UniformPackWrite,
    DescriptorAllocate, DescriptorUpdate, CommandBegin, AcquireImage,
    PresentFenceWait, CommandRecord, QueueSubmit, QueuePresent, BackendTail,
    FacadeResult, DescriptorRetire, QueryCollect, DebugValidation, PresentationLatency, Count
};
inline constexpr std::array<const char*, unsigned(FramePhase::Count)> FramePhaseNames{
    "facade_prepare", "backend_setup", "slot_wait_collect", "resource_retire", "cpu_pack",
    "selection_allocate", "selection_write", "uniform_allocate", "uniform_pack_write",
    "descriptor_allocate", "descriptor_update", "command_begin", "acquire_image",
    "present_fence_wait", "command_record", "queue_submit", "queue_present", "backend_tail",
    "facade_result", "descriptor_retire", "query_collect", "debug_validation", "presentation_latency"
};
enum class FrameWork : unsigned {
    LayerScan, MeshCreateTotal, MeshUpdateTotal, MeshValidate, MeshOwnedCopy,
    MeshBounds, MeshAllocate, MeshMapCopy, MeshDebug, LayerSummaryCheck, Count
};
inline constexpr std::array<const char*, unsigned(FrameWork::Count)> FrameWorkNames{
    "layer_scan", "mesh_create_total", "mesh_update_total", "mesh_validate", "mesh_owned_copy",
    "mesh_bounds", "mesh_allocate", "mesh_map_copy", "mesh_debug", "layer_summary_check"
};
struct FrameWorkTotals {
    std::array<std::uint64_t, unsigned(FrameWork::Count)> wall_ns{};
    std::array<unsigned, unsigned(FrameWork::Count)> visits{};
    std::uint64_t layer_vertices{};
};
struct FrameDiagnosticStamp {
    std::chrono::steady_clock::time_point wall;
    std::optional<std::uint64_t> cpu_ns;
};
struct FrameDiagnosticRecord {
    std::uint64_t frame_id{};
    bool screenshot{}, completed{}, cpu_valid{true};
    std::array<std::uint64_t, unsigned(FramePhase::Count)> wall_ns{}, cpu_ns{};
    std::array<unsigned, unsigned(FramePhase::Count)> visits{};
    FrameWorkTotals work;
};
struct FrameDiagnosticSink {
    void* context{};
    FrameDiagnosticStamp (*now)(void*) noexcept{};
    void (*accept)(void*, const FrameDiagnosticRecord&) noexcept{};
};
inline thread_local FrameDiagnosticSink* frame_diagnostic_sink = nullptr;
inline thread_local FrameWorkTotals pending_frame_work;
class FrameDiagnosticScope;
inline thread_local FrameDiagnosticScope* current_frame_diagnostic = nullptr;

class ScopedFrameDiagnostics {
public:
    explicit ScopedFrameDiagnostics(FrameDiagnosticSink& sink) {
        if (frame_diagnostic_sink || !sink.now || !sink.accept)
            throw std::logic_error("exclusive valid frame diagnostic sink required");
        frame_diagnostic_sink = &sink;
        pending_frame_work = {};
    }
    ~ScopedFrameDiagnostics() { frame_diagnostic_sink = nullptr; pending_frame_work = {}; }
    ScopedFrameDiagnostics(const ScopedFrameDiagnostics&) = delete;
    ScopedFrameDiagnostics& operator=(const ScopedFrameDiagnostics&) = delete;
};
class FrameDiagnosticScope {
    FrameDiagnosticSink* sink_{};
    std::optional<FrameDiagnosticRecord> record_;
    FrameDiagnosticStamp previous_;
    FramePhase phase_{FramePhase::FacadePrepare};
    void Close() noexcept {
        const auto next = sink_->now(sink_->context);
        const auto index = unsigned(phase_);
        const auto wall = std::chrono::duration_cast<std::chrono::nanoseconds>(next.wall - previous_.wall).count();
        if (wall >= 0) record_->wall_ns[index] += std::uint64_t(wall);
        if (previous_.cpu_ns && next.cpu_ns && *next.cpu_ns >= *previous_.cpu_ns)
            record_->cpu_ns[index] += *next.cpu_ns - *previous_.cpu_ns;
        else record_->cpu_valid = false;
        previous_ = next;
    }
public:
    FrameDiagnosticScope(bool enabled, std::uint64_t id, bool screenshot) noexcept
        : sink_(enabled ? frame_diagnostic_sink : nullptr) {
        if (!sink_) return;
        record_.emplace(); record_->frame_id = id; record_->screenshot = screenshot;
        record_->work = pending_frame_work; pending_frame_work = {};
        previous_ = sink_->now(sink_->context);
        record_->visits[unsigned(phase_)] = 1;
        current_frame_diagnostic = this;
    }
    void Next(FramePhase phase) noexcept {
        Close(); phase_ = phase; ++record_->visits[unsigned(phase)];
    }
    void Complete() noexcept { if (sink_) record_->completed = true; }
    FrameWorkTotals& Work() noexcept { return record_->work; }
    ~FrameDiagnosticScope() {
        if (!sink_) return;
        Close(); current_frame_diagnostic = nullptr;
        sink_->accept(sink_->context, *record_);
    }
    FrameDiagnosticScope(const FrameDiagnosticScope&) = delete;
    FrameDiagnosticScope& operator=(const FrameDiagnosticScope&) = delete;
};
inline void DiagnosticPhase(FramePhase phase) noexcept {
    if (current_frame_diagnostic) current_frame_diagnostic->Next(phase);
}
// Nested work is not added to the exclusive phase partition. Pre-Render mesh
// work is consumed once by the next frame, including startup on the first ID.
class DiagnosticWorkScope {
    FrameWorkTotals* totals_{};
    FrameWork work_;
    std::chrono::steady_clock::time_point begin_;
    std::uint64_t vertices_{};
public:
    explicit DiagnosticWorkScope(FrameWork work, bool enabled = true, std::uint64_t vertices = 0) noexcept
        : work_(work), vertices_(vertices) {
        if (!enabled || !frame_diagnostic_sink) return;
        totals_ = current_frame_diagnostic ? &current_frame_diagnostic->Work() : &pending_frame_work;
        begin_ = std::chrono::steady_clock::now();
    }
    ~DiagnosticWorkScope() {
        if (!totals_) return;
        const auto elapsed = std::chrono::steady_clock::now() - begin_;
        totals_->wall_ns[unsigned(work_)] += std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
        ++totals_->visits[unsigned(work_)]; totals_->layer_vertices += vertices_;
    }
    DiagnosticWorkScope(const DiagnosticWorkScope&) = delete;
    DiagnosticWorkScope& operator=(const DiagnosticWorkScope&) = delete;
};
}
