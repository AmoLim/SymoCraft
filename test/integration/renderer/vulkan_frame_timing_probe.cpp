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
    unsigned buffers_created{}, buffers_released{}, waits{}, swapchains{};
    std::array<VulkanDiagnosticAccess::CallStats, unsigned(VulkanDiagnosticAccess::Call::Count)> calls;
    std::array<std::uint64_t, unsigned(VulkanDiagnosticAccess::BufferRole::Count)> role_allocations{};
    std::array<std::uint64_t, 3> descriptor_writes{};
    std::uint64_t observed_frame{}, mesh_buffers{}, mesh_bytes{}, local_mesh_buffers{}, local_mesh_bytes{};
    unsigned slot{}, image{}, tracked_submits{}, tracked_presents{};
    VkResult before_present{}, after_present{};
};
struct Capture {
    std::vector<Row> rows;
    std::shared_ptr<VulkanDiagnosticAccess> native = std::make_shared<VulkanDiagnosticAccess>();
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
        Row row{timing, d.buffers_created, d.buffers_released, d.wait_calls, d.swapchain_creations};
        row.calls = d.native_calls;
        for (unsigned role = 0; role < row.role_allocations.size(); ++role)
            for (const auto& memory : d.buffer_allocations[role]) row.role_allocations[role] += memory.allocations;
        for (const auto& slot : d.binding_slots) for (const auto* bundle : {&slot.mesh, &slot.selection}) {
            row.descriptor_writes[0] += bundle->uniform_descriptors;
            row.descriptor_writes[1] += bundle->image_descriptors;
            row.descriptor_writes[2] += bundle->sampler_descriptors;
        }
        row.observed_frame = d.queue_observation_frame_id; row.slot = d.last_binding_slot; row.image = d.present_image;
        row.mesh_buffers = d.mesh_bound_buffers; row.mesh_bytes = d.mesh_bound_bytes;
        row.local_mesh_buffers = d.mesh_device_local_buffers; row.local_mesh_bytes = d.mesh_device_local_bytes;
        row.tracked_submits = d.tracked_submit_pending; row.tracked_presents = d.tracked_present_pending;
        row.before_present = d.submit_before_present; row.after_present = d.submit_after_present;
        self.rows.push_back(row);
    }
};
bool WriteNative(const std::filesystem::path& output, const VulkanDiagnosticAccess& d) {
    std::ofstream types(output.string() + ".memory-types.csv"), allocations(output.string() + ".buffer-allocations.csv"),
        device(output.string() + ".device.csv"), calls(output.string() + ".native-final.csv"), slots(output.string() + ".binding-slots.csv");
    types << "memory_type,property_flags,heap_index,heap_flags,heap_bytes\n";
    for (unsigned i = 0; i < d.memory_properties.memoryTypeCount; ++i) {
        const auto& type = d.memory_properties.memoryTypes[i]; const auto& heap = d.memory_properties.memoryHeaps[type.heapIndex];
        types << i << ',' << type.propertyFlags << ',' << type.heapIndex << ',' << heap.flags << ',' << heap.size << '\n';
    }
    allocations << "role,memory_type,allocations,binds,releases,requested_bytes,allocated_bytes,live_bytes,requirements_bits_or,requirements_bits_and\n";
    for (unsigned role = 0; role < unsigned(VulkanDiagnosticAccess::BufferRole::Count); ++role)
        for (unsigned type = 0; type < d.memory_properties.memoryTypeCount; ++type) {
            const auto& a = d.buffer_allocations[role][type];
            if (a.allocations) allocations << d.BufferRoleNames[role] << ',' << type << ',' << a.allocations << ',' << a.binds << ','
                << a.releases << ',' << a.requested_bytes << ',' << a.allocated_bytes << ',' << a.live_bytes << ','
                << a.requirements_bits_or << ',' << a.requirements_bits_and << '\n';
        }
    device << "graphics_family,present_family,same_queue,swapchain_images,timestamp_bits,timestamp_period_ns,record_capacity,record_bytes\n"
        << d.graphics_family << ',' << d.present_family << ',' << d.same_graphics_present_queue << ',' << d.swapchain_images << ','
        << d.timestamp_bits << ',' << d.timestamp_period << ",100000," << sizeof(Row) * 100000 << '\n';
    calls << "call,calls,successes,wall_ms\n" << std::setprecision(12);
    for (unsigned i = 0; i < d.native_calls.size(); ++i)
        calls << d.CallNames[i] << ',' << d.native_calls[i].calls << ',' << d.native_calls[i].successes << ',' << double(d.native_calls[i].wall_ns) / 1e6 << '\n';
    slots << "slot,kind,uniform_allocations,pools_created,pools_released,sets_allocated,update_calls,uniform_descriptors,image_descriptors,sampler_descriptors,uniform_writes,uniform_bytes,capacity\n";
    for (unsigned i = 0; i < d.binding_slots.size(); ++i) for (unsigned kind = 0; kind < 2; ++kind) {
        const auto& b = kind ? d.binding_slots[i].selection : d.binding_slots[i].mesh;
        slots << i << ',' << (kind ? "selection" : "mesh") << ',' << b.uniform_allocations << ',' << b.pools_created << ',' << b.pools_released << ','
            << b.sets_allocated << ',' << b.update_calls << ',' << b.uniform_descriptors << ',' << b.image_descriptors << ',' << b.sampler_descriptors << ','
            << b.uniform_writes << ',' << b.uniform_bytes << ',' << b.capacity << '\n';
    }
    types.flush(); allocations.flush(); device.flush(); calls.flush(); slots.flush();
    return types && allocations && device && calls && slots;
}
}
int main(int argc, char** argv) {
    if (argc < 3) return 64;
    try {
        const auto output = std::filesystem::absolute(argv[1]);
        if (std::filesystem::exists(output)) return 64;
        for (const auto* suffix : {".memory-types.csv", ".buffer-allocations.csv", ".device.csv", ".native-final.csv", ".binding-slots.csv"})
            if (std::filesystem::exists(output.string() + suffix)) return 64;
        Capture capture;
        capture.rows.reserve(100000);
        // The ordinary production deadline is unchanged by the diagnostic binding.
        capture.native->wait_timeout = RendererConfig{}.wait_timeout;
        capture.native->observe_performance = true;
        FrameDiagnosticSink sink{&capture, Capture::Now, Capture::Accept};
        ScopedFrameDiagnostics timing(sink);
        ScopedVulkanDiagnostics native(capture.native);
        const auto code = SymoCraft::Application::RunGame(argc - 2, argv + 2);
        std::ofstream file(output);
        file << "renderer_frame_id,screenshot,completed,cpu_valid,buffers_created,buffers_released,wait_calls,swapchain_creations,layer_vertices,layer_validation_model";
        for (const auto* phase : FramePhaseNames) file << ',' << phase << "_wall_ms," << phase << "_cpu_ms," << phase << "_visits";
        for (const auto* work : FrameWorkNames) file << ',' << work << "_work_ms," << work << "_work_visits";
        file << ",native_model,observed_frame_id,frame_slot,present_image,tracked_submit_pending,tracked_present_pending,submit_before_present,submit_after_present,mesh_bound_buffers,mesh_bound_bytes,mesh_device_local_buffers,mesh_device_local_bytes";
        for (const auto* name : VulkanDiagnosticAccess::CallNames) file << ',' << name << "_calls," << name << "_successes," << name << "_native_wall_ms";
        for (const auto* name : VulkanDiagnosticAccess::BufferRoleNames) file << ',' << name << "_allocations";
        file << ",ubo_descriptor_writes,image_descriptor_writes,sampler_descriptor_writes";
        file << '\n' << std::setprecision(12);
        for (const auto& row : capture.rows) {
            const auto& r = row.timing;
            file << r.frame_id << ',' << r.screenshot << ',' << r.completed << ',' << r.cpu_valid << ','
                 << row.buffers_created << ',' << row.buffers_released << ',' << row.waits << ',' << row.swapchains
                 << ',' << r.work.layer_vertices << ",cached_max_v1";
            for (unsigned i = 0; i < unsigned(FramePhase::Count); ++i) {
                file << ',' << double(r.wall_ns[i]) / 1e6 << ',';
                if (r.cpu_valid) file << double(r.cpu_ns[i]) / 1e6;
                file << ',' << r.visits[i];
            }
            for (unsigned i = 0; i < unsigned(FrameWork::Count); ++i)
                file << ',' << double(r.work.wall_ns[i]) / 1e6 << ',' << r.work.visits[i];
            file << ",native_calls_v1," << row.observed_frame << ',' << row.slot << ',' << row.image << ',' << row.tracked_submits << ','
                << row.tracked_presents << ',' << row.before_present << ',' << row.after_present << ',' << row.mesh_buffers << ',' << row.mesh_bytes << ','
                << row.local_mesh_buffers << ',' << row.local_mesh_bytes;
            for (const auto& call : row.calls) file << ',' << call.calls << ',' << call.successes << ',' << double(call.wall_ns) / 1e6;
            for (const auto count : row.role_allocations) file << ',' << count;
            for (const auto count : row.descriptor_writes) file << ',' << count;
            file << '\n';
        }
        file.flush();
        const auto& d = *capture.native;
        const auto native_written = WriteNative(output, d);
        std::cout << "[timing] records=" << capture.rows.size() << "; overflow=" << capture.overflow
                  << "; warnings=" << d.warnings << "; errors=" << d.errors
                  << "; retired=" << (d.native_destroyed && d.loader_released && !d.quarantined)
                  << "; production_deadline_ms=" << d.wait_timeout.count() << '\n';
        if (!file || !native_written || capture.overflow || capture.rows.empty() || !d.native_destroyed || !d.loader_released || d.quarantined || d.errors) return 2;
        return code;
    } catch (const std::exception& error) { std::cerr << "[timing] " << error.what() << '\n'; return 2; }
}
