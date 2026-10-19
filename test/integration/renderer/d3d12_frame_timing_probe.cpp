#include "frame_diagnostics.h"
#include "diagnostic_access.h"
#include "game_entry.h"
#include <symocraft/renderer/types.h>
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {
using namespace SymoCraft::Rendering;
using namespace SymoCraft::Rendering::Detail;
struct Row {
    FrameDiagnosticRecord timing;
    std::uint64_t meshes_created{}, meshes_released{};
    unsigned waits{};
    std::uint64_t srv_heaps_created{}, srv_views_written{};
    unsigned srv_slot{}, srv_capacity{};
};
struct Capture {
    std::vector<Row> rows;
    std::shared_ptr<D3D12DiagnosticAccess> native = std::make_shared<D3D12DiagnosticAccess>();
    bool overflow{};
    static FrameDiagnosticStamp Now(void*) noexcept {
        FILETIME created{}, exited{}, kernel{}, user{};
        std::optional<std::uint64_t> cpu;
        if (GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user)) {
            const auto value = [](FILETIME time) { return (std::uint64_t(time.dwHighDateTime) << 32) | time.dwLowDateTime; };
            cpu = (value(kernel) + value(user)) * 100;
        }
        return {std::chrono::steady_clock::now(), cpu};
    }
    static void Accept(void* context, const FrameDiagnosticRecord& timing) noexcept {
        auto& self = *static_cast<Capture*>(context);
        if (self.rows.size() == self.rows.capacity()) { self.overflow = true; return; }
        const auto& d = *self.native;
        std::uint64_t heaps{}, views{};
        for (const auto& slot : d.srv_slots) { heaps += slot.heaps_created; views += slot.views_written; }
        self.rows.push_back({timing, d.mesh_buffers_created, d.mesh_buffers_released, d.wait_calls,
            heaps, views, d.last_srv_slot, d.srv_slots[d.last_srv_slot].capacity});
    }
};
}
int main(int argc, char** argv) {
    if (argc < 3) return 64;
    try {
        const auto output = std::filesystem::absolute(argv[1]);
        if (std::filesystem::exists(output)) return 64;
        Capture capture; capture.rows.reserve(100000);
        capture.native->wait_timeout = RendererConfig{}.wait_timeout;
        FrameDiagnosticSink sink{&capture, Capture::Now, Capture::Accept};
        ScopedFrameDiagnostics timing(sink);
        ScopedD3D12Diagnostics native(capture.native);
        const auto code = SymoCraft::Application::RunGame(argc - 2, argv + 2);
        auto& d = *capture.native;
        const bool retired = d.native_cleared && !d.quarantined && !d.live_resource_bytes
            && d.mesh_buffers_created == d.mesh_buffers_released && d.textures_created == d.textures_released;
        // The backend has retired its owners; release the probe's observational COM references too.
        if (retired) { d.queue.Reset(); d.completion.Reset(); d.device.Reset(); }
        std::ofstream file(output);
        file << "renderer_frame_id,screenshot,completed,cpu_valid,mesh_buffers_created,mesh_buffers_released,wait_calls,layer_vertices,layer_validation_model,srv_binding_model,srv_heaps_created,srv_views_written,srv_slot,srv_capacity";
        for (const auto* phase : FramePhaseNames) file << ',' << phase << "_wall_ms," << phase << "_cpu_ms," << phase << "_visits";
        for (const auto* work : FrameWorkNames) file << ',' << work << "_work_ms," << work << "_work_visits";
        file << '\n' << std::setprecision(12);
        for (const auto& row : capture.rows) {
            const auto& r = row.timing;
            file << r.frame_id << ',' << r.screenshot << ',' << r.completed << ',' << r.cpu_valid << ','
                 << row.meshes_created << ',' << row.meshes_released << ',' << row.waits << ',' << r.work.layer_vertices
                 << ",cached_max_v1,frame_slot_delta_v1," << row.srv_heaps_created << ',' << row.srv_views_written
                 << ',' << row.srv_slot << ',' << row.srv_capacity;
            for (unsigned i = 0; i < unsigned(FramePhase::Count); ++i) {
                file << ',' << double(r.wall_ns[i]) / 1e6 << ',';
                if (r.cpu_valid) file << double(r.cpu_ns[i]) / 1e6;
                file << ',' << r.visits[i];
            }
            for (unsigned i = 0; i < unsigned(FrameWork::Count); ++i)
                file << ',' << double(r.work.wall_ns[i]) / 1e6 << ',' << r.work.visits[i];
            file << '\n';
        }
        std::cout << "[timing] records=" << capture.rows.size() << "; overflow=" << capture.overflow
                  << "; warnings=" << d.validation_warnings << "; errors=" << d.validation_errors
                  << "; corruptions=" << d.validation_corruptions << "; retired=" << retired
                  << "; production_deadline_ms=" << d.wait_timeout.count()
                  << "; recorder_capacity_bytes=" << capture.rows.capacity() * sizeof(Row) << '\n';
        if (!file || capture.overflow || capture.rows.empty() || !retired || d.validation_warnings || d.validation_errors || d.validation_corruptions) return 2;
        return code;
    } catch (const std::exception& error) { std::cerr << "[timing] " << error.what() << '\n'; return 2; }
}
