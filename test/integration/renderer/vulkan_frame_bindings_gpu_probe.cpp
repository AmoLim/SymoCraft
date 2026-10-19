#include "diagnostic_access.h"
#include <symocraft/assets/image.h>
#include <symocraft/platform/window.h>
#include <symocraft/renderer/renderer.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using namespace SymoCraft::Rendering::Detail;
using Color = std::array<int, 3>;
using Pixels = std::vector<uint8_t>;
using Counters = VulkanDiagnosticAccess::BindingCounters;
using BufferRole = VulkanDiagnosticAccess::BufferRole;
using NativeCall = VulkanDiagnosticAccess::Call;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
VulkanDiagnosticAccess::AllocationStats AllocationTotals(const VulkanDiagnosticAccess& observed, BufferRole role) {
    VulkanDiagnosticAccess::AllocationStats total;
    for (const auto& type : observed.buffer_allocations[unsigned(role)]) {
        total.allocations += type.allocations; total.binds += type.binds; total.releases += type.releases;
        total.requested_bytes += type.requested_bytes; total.allocated_bytes += type.allocated_bytes;
        total.live_bytes += type.live_bytes;
    }
    return total;
}
void SaveBufferObservations(const VulkanDiagnosticAccess& observed, const std::filesystem::path& output) {
    std::ofstream allocations(output / "buffer-allocations.csv"), calls(output / "native-calls.csv");
    allocations << "role,memory_type,flags,heap,heap_flags,requirements_and,requirements_or,allocations,binds,releases,requested_bytes,allocated_bytes,live_bytes\n";
    for (unsigned role = 0; role < unsigned(BufferRole::Count); ++role) {
        for (uint32_t type = 0; type < observed.memory_properties.memoryTypeCount; ++type) {
            const auto& stats = observed.buffer_allocations[role][type];
            if (!stats.allocations) continue;
            const auto& memory = observed.memory_properties.memoryTypes[type];
            allocations << VulkanDiagnosticAccess::BufferRoleNames[role] << ',' << type << ',' << memory.propertyFlags << ','
                << memory.heapIndex << ',' << observed.memory_properties.memoryHeaps[memory.heapIndex].flags << ','
                << stats.requirements_bits_and << ',' << stats.requirements_bits_or << ',' << stats.allocations << ','
                << stats.binds << ',' << stats.releases << ',' << stats.requested_bytes << ',' << stats.allocated_bytes << ','
                << stats.live_bytes << '\n';
            Require(stats.allocations == stats.binds && stats.binds == stats.releases && stats.live_bytes == 0,
                "buffer role/type allocation did not retire normally");
            Require((stats.requirements_bits_and & (1u << type)) && (memory.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
                "actual buffer type was incompatible or not host visible");
            if (role == unsigned(BufferRole::Mesh))
                Require(memory.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "mesh did not use available mappable device-local memory");
        }
    }
    calls << "call,calls,successes\n";
    for (unsigned call = 0; call < unsigned(NativeCall::Count); ++call) {
        const auto& stats = observed.native_calls[call];
        calls << VulkanDiagnosticAccess::CallNames[call] << ',' << stats.calls << ',' << stats.successes << '\n';
    }
    allocations.flush(); calls.flush();
    Require(bool(allocations) && bool(calls), "cannot save native buffer observations");
}
struct OwnedWindow {
    Platform::Runtime runtime;
    std::unique_ptr<Window> window;
    OwnedWindow() {
        window = Window::Create(runtime, "SymoCraft Vulkan frame bindings GPU verification", 640, 480, false, true, WindowMode::Vulkan);
    }
    ~OwnedWindow() {
        if (window) { window->Destroy(); window.reset(); }
        runtime.Shutdown();
    }
};
MeshData Quad(bool right) {
    MeshData data;
    const int x = right ? 0 : -2;
    const glm::ivec3 p[]{{x,-1,0},{x+2,-1,0},{x+2,1,0},{x,-1,0},{x+2,1,0},{x,1,0}};
    const glm::vec2 uv[]{{0,1},{1,1},{1,0},{0,1},{1,0},{0,0}};
    for (size_t i = 0; i < 6; ++i) data.vertices.push_back({p[i], {uv[i], right ? 1.f : 0.f}, 0});
    return data;
}
const std::array<Color, 4> PaletteA{{{230,20,40},{20,220,50},{20,40,230},{230,200,20}}};
const std::array<Color, 4> PaletteC{{{20,180,210},{200,30,170},{240,100,20},{30,150,60}}};
const Color RightColor{180,40,190};
TextureArrayData Atlas(const std::array<Color, 4>& palette, Color solid) {
    TextureArrayData data; data.width = data.height = data.layers = 2;
    const auto pixel = [&](Color color) {
        for (int c : color) data.pixels.push_back(static_cast<uint8_t>(c));
        data.pixels.push_back(255);
    };
    for (auto color : palette) pixel(color);
    for (int i = 0; i < 4; ++i) pixel(solid);
    return data;
}
Color Pixel(const Pixels& image, unsigned x, unsigned y) {
    const auto offset = (size_t(y) * 640 + x) * 4;
    return {image.at(offset), image.at(offset + 1), image.at(offset + 2)};
}
void CheckImage(const Screenshot& image, const std::array<Color, 4>& palette, bool right, int mesh_sample_shift) {
    Require(image.extent.width == 640 && image.extent.height == 480 && image.pixels.size() == 640 * 480 * 4,
        "unexpected screenshot extent/storage");
    const std::array<std::array<unsigned, 2>, 5> points{{{192,192},{288,192},{192,288},{288,288},{400,240}}};
    for (size_t i = 0; i < (right ? 5u : 4u); ++i) {
        const auto expected = i < 4 ? palette[i] : RightColor;
        const auto actual = Pixel(image.pixels, unsigned(int(points[i][0]) + (i < 4 ? mesh_sample_shift : 0)), points[i][1]);
        for (size_t c = 0; c < 3; ++c)
            Require(std::abs(actual[c] - expected[c]) <= 1, "texture binding/UV/color mismatch");
    }
}
size_t Differences(const Pixels& a, const Pixels& b, unsigned xmin, unsigned xmax) {
    size_t changed = 0;
    for (unsigned y = 0; y < 480; ++y) for (unsigned x = xmin; x < xmax; ++x)
        if (Pixel(a,x,y) != Pixel(b,x,y)) ++changed;
    return changed;
}
void Run(const std::filesystem::path& shaders, const std::filesystem::path& output, bool buffer_policy) {
    OwnedWindow owner;
    auto observed = std::make_shared<VulkanDiagnosticAccess>();
    observed->observe_performance = buffer_policy; // Actual native counts/types only; no performance verdict.
    observed->wait_timeout = RendererConfig{}.wait_timeout;
    ScopedVulkanDiagnostics binding(observed);
    RendererConfig config; config.backend = Backend::Vulkan; config.shader_asset_root = shaders;
    config.requested_samples = 4; config.enable_validation = true; config.vsync = false;
    Renderer renderer(*owner.window, config);
    const auto info = renderer.GetInfo();
    Require(info.backend == Backend::Vulkan && info.device_name.find("RTX 5070 Ti") != std::string::npos &&
        info.actual_samples == 4 && info.validation_enabled && info.actual_present_mode == PresentMode::Immediate &&
        info.framebuffer_extent.width == 640 && info.framebuffer_extent.height == 480, "actual hardware/configuration mismatch");
    std::cout << "backend=Vulkan; device=" << info.device_name << "; identity=" << info.device_identity << "; driver="
        << info.driver_version << "; api=" << info.api_version << "; samples=4; present=Immediate; validation=1\n";
    const auto left = renderer.CreateMesh(Quad(false)), right = renderer.CreateMesh(Quad(true));
    const auto a = renderer.CreateTextureArray(Atlas(PaletteA, {100,100,100}));
    const auto b = renderer.CreateTextureArray(Atlas(PaletteA, RightColor));
    const auto c = renderer.CreateTextureArray(Atlas(PaletteC, {30,100,190}));
    std::array<MeshDraw, 2> draws{{{left,a},{right,b}}};
    const SelectionBox box{{-1.95f,-.95f,.02f},{-.05f,.95f,.1f}};
    FrameInput frame; frame.camera.position = {0,0,4}; frame.draws = draws;
    frame.selection = box; frame.request_screenshot = true;
    std::ofstream rows(output / "observations.csv");
    rows << "stage,frame_id,slot,kind,uniform_allocations_delta,pools_delta,sets_delta,update_calls_delta,ubo_descriptors_delta,image_descriptors_delta,sampler_descriptors_delta,uniform_writes_delta,uniform_bytes_delta,capacity\n";
    Require(bool(rows), "cannot open native observation output");
    std::ofstream buffer_rows;
    if (buffer_policy) {
        buffer_rows.open(output / "buffer-observations.csv");
        buffer_rows << "stage,frame_id,slot,selection_active,selection_allocations_delta,selection_binds_delta,selection_releases_delta,selection_live_bytes,selection_requested_bytes,mesh_allocations_delta,map_writes_delta,upload_bytes\n";
        Require(bool(buffer_rows), "cannot open buffer observation output");
    }
    unsigned captures = 0;
    auto check = [&](const Counters& before, const Counters& after, bool selection, unsigned slot,
                     const char* name, bool initial, unsigned images, bool active) {
        const unsigned count = selection ? 1u : 2u;
        const auto delta = [&](uint64_t Counters::* member) { return after.*member - before.*member; };
        rows << name << ',' << frame.frame_id << ',' << slot << ',' << (selection ? "selection" : "mesh") << ','
            << delta(&Counters::uniform_allocations) << ',' << delta(&Counters::pools_created) << ','
            << delta(&Counters::sets_allocated) << ',' << delta(&Counters::update_calls) << ','
            << delta(&Counters::uniform_descriptors) << ',' << delta(&Counters::image_descriptors) << ','
            << delta(&Counters::sampler_descriptors) << ',' << delta(&Counters::uniform_writes) << ','
            << delta(&Counters::uniform_bytes) << ',' << after.capacity << '\n'; rows.flush();
        Require(delta(&Counters::uniform_allocations) == unsigned(initial) && delta(&Counters::pools_created) == unsigned(initial) &&
            delta(&Counters::sets_allocated) == (initial ? count : 0) && after.capacity == count &&
            after.uniform_allocations == 1 && after.pools_created == 1 && after.pools_released == 0,
            "actual persistent buffer/pool/set reuse mismatch");
        Require(delta(&Counters::uniform_descriptors) == (initial ? count : 0) &&
            delta(&Counters::image_descriptors) == images && delta(&Counters::sampler_descriptors) == (initial && !selection ? count : 0) &&
            delta(&Counters::update_calls) == (selection ? unsigned(initial) : images), "actual differential descriptor writes mismatch");
        Require(delta(&Counters::uniform_writes) == unsigned(active), "persistent uniform content was not written each active frame");
        const auto stride = observed->binding_slots[slot].selection.uniform_bytes /
            observed->binding_slots[slot].selection.uniform_writes;
        Require(stride >= 64 && delta(&Counters::uniform_bytes) == (active ? stride * (selection ? 1 : frame.draws.size()) : 0),
            "actual uniform upload byte accounting mismatch");
    };
    auto stage = [&](const char* name, const std::array<Color, 4>& palette, unsigned images, bool initial,
                     const Pixels* reference = nullptr, int mesh_sample_shift = 0) {
        std::array<bool, 3> visited{};
        Pixels first;
        for (unsigned attempt = 0; attempt < 24 && !std::all_of(visited.begin(), visited.end(), [](bool x) { return x; }); ++attempt) {
            const auto before = observed->binding_slots;
            const auto before_selection = AllocationTotals(*observed, BufferRole::SelectionVertex);
            const auto before_mesh = AllocationTotals(*observed, BufferRole::Mesh);
            const auto before_maps = observed->native_calls[unsigned(NativeCall::MapWrite)].calls;
            owner.window->PollInt(); ++frame.frame_id;
            const auto result = renderer.Render(frame);
            Require(result.status == FrameStatus::Presented && result.screenshot && result.frame_id == frame.frame_id &&
                result.statistics.draw_calls == frame.draws.size() + unsigned(frame.selection.has_value()) &&
                result.statistics.drawn_vertices == frame.draws.size() * 6 + (frame.selection ? 24 : 0), "normal frame did not draw/present/read back");
            try { CheckImage(*result.screenshot, palette, frame.draws.size() == 2, mesh_sample_shift); }
            catch (...) {
                Assets::WritePng(output / (std::string(name) + "-failed.png"), 640, 480, 4, result.screenshot->pixels);
                throw;
            }
            if (reference) Require(result.screenshot->pixels == *reference, "repeat/reorder/restoration changed expected RGBA image");
            if (first.empty()) {
                first = result.screenshot->pixels;
                Assets::WritePng(output / (std::string(name) + ".png"), 640, 480, 4, first); ++captures;
            } else Require(result.screenshot->pixels == first, "actual frame slots produced different images");
            const auto slot = observed->last_binding_slot;
            Require(slot < visited.size() && observed->last_binding_frame_id == frame.frame_id, "missing current native slot/frame observation");
            if (buffer_policy) {
                const auto after_selection = AllocationTotals(*observed, BufferRole::SelectionVertex);
                const auto after_mesh = AllocationTotals(*observed, BufferRole::Mesh);
                const auto allocation_delta = after_selection.allocations - before_selection.allocations;
                const auto bind_delta = after_selection.binds - before_selection.binds;
                const auto release_delta = after_selection.releases - before_selection.releases;
                const auto mesh_delta = after_mesh.allocations - before_mesh.allocations;
                const auto map_delta = observed->native_calls[unsigned(NativeCall::MapWrite)].calls - before_maps;
                const unsigned first_selection = unsigned(initial && !visited[slot] && frame.selection.has_value());
                buffer_rows << name << ',' << frame.frame_id << ',' << slot << ',' << unsigned(frame.selection.has_value()) << ','
                    << allocation_delta << ',' << bind_delta << ',' << release_delta << ',' << after_selection.live_bytes << ','
                    << after_selection.requested_bytes << ',' << mesh_delta << ',' << map_delta << ',' << result.statistics.upload_bytes << '\n';
                buffer_rows.flush();
                Require(allocation_delta == first_selection && bind_delta == first_selection && release_delta == 0 && mesh_delta == 0,
                    "actual retained selection/mesh buffer allocation mismatch");
                Require(map_delta == 1 + 2 * unsigned(frame.selection.has_value()), "actual uniform/selection writes missing");
                const auto& selection = observed->binding_slots[slot].selection;
                const auto stride = selection.uniform_bytes / selection.uniform_writes;
                Require(result.statistics.upload_bytes == stride * frame.draws.size() +
                    (frame.selection ? stride + 288 : 0), "retained selection upload accounting mismatch");
            }
            check(before[slot].mesh, observed->binding_slots[slot].mesh, false, slot, name, initial && !visited[slot],
                visited[slot] ? 0 : images, true);
            check(before[slot].selection, observed->binding_slots[slot].selection, true, slot, name, initial && !visited[slot], 0,
                frame.selection.has_value());
            for (size_t other = 0; other < visited.size(); ++other) if (other != slot)
                Require(observed->binding_slots[other] == before[other], "another native frame slot was mutated");
            visited[slot] = true;
        }
        Require(std::all_of(visited.begin(), visited.end(), [](bool x) { return x; }), "all three actual frame slots were not visited");
        return first;
    };
    const auto baseline = stage("initial", PaletteA, 2, true);
    stage("initial-repeat", PaletteA, 0, false, &baseline);
    draws[0].texture_array = c;
    const auto switched = stage("texture-switch", PaletteC, 1, false);
    Require(switched != baseline, "texture switch did not change pixels");
    stage("switch-repeat", PaletteC, 0, false, &switched);
    std::swap(draws[0], draws[1]);
    stage("draw-reordered", PaletteC, 2, false, &switched);
    stage("reorder-repeat", PaletteC, 0, false, &switched);
    draws = {{{left,a},{right,b}}};
    stage("restored", PaletteA, 2, false, &baseline);
    frame.selection.reset();
    const auto hidden = stage("selection-hidden", PaletteA, 0, false);
    Require(Differences(hidden,baseline,0,320) > 100 && Differences(hidden,baseline,320,640) == 0,
        "selection did not affect only its expected left region");
    frame.selection = box;
    stage("selection-restored", PaletteA, 0, false, &baseline);
    frame.selection = SelectionBox{{.05f,-.95f,.02f},{1.95f,.95f,.1f}};
    const auto moved_box = stage("selection-moved", PaletteA, 0, false);
    Require(Differences(moved_box,hidden,0,320) == 0 && Differences(moved_box,hidden,320,640) > 100,
        "selection vertex update/binding did not move to expected right region");
    frame.selection = box;
    stage("selection-move-restored", PaletteA, 0, false, &baseline);
    frame.draws = std::span(draws).first(1);
    const auto one_mesh = stage("one-mesh-with-selection", PaletteA, 0, false);
    Require(Differences(one_mesh,baseline,0,320) == 0 && Differences(one_mesh,baseline,320,640) > 10000,
        "normal draw-count change aliased independent selection binding");
    frame.draws = draws;
    stage("two-meshes-restored", PaletteA, 0, false, &baseline);
    draws[0].object_transform[3][0] = -.3f;
    // At this fixed camera/extent, -0.3 world units shifts the left quad about 31 pixels.
    const auto translated = stage("object-translated", PaletteA, 0, false, nullptr, -32);
    Require(Differences(translated,baseline,0,320) > 1000 && Differences(translated,baseline,320,640) == 0,
        "mesh matrix upload did not move only the expected object");
    draws[0].object_transform = glm::mat4(1.f);
    stage("object-restored", PaletteA, 0, false, &baseline);
    if (buffer_policy) {
        auto flipped = Quad(false);
        for (auto& vertex : flipped.vertices) vertex.tex_coord.x = 1.f - vertex.tex_coord.x;
        renderer.UpdateMesh(left, flipped);
        const std::array<Color, 4> mirrored{{PaletteA[1], PaletteA[0], PaletteA[3], PaletteA[2]}};
        const auto updated = stage("mesh-uv-updated", mirrored, 0, false);
        Require(Differences(updated,baseline,0,320) > 10000 && Differences(updated,baseline,320,640) == 0,
            "accepted mesh update did not change only the expected UV region");
        renderer.UpdateMesh(left, Quad(false));
        stage("mesh-uv-restored", PaletteA, 0, false, &baseline);
        const auto meshes = AllocationTotals(*observed, BufferRole::Mesh);
        const auto selections = AllocationTotals(*observed, BufferRole::SelectionVertex);
        Require(meshes.allocations == 4 && meshes.binds == 4 && observed->mesh_bound_buffers == 4 &&
            observed->mesh_device_local_buffers == 4 && observed->mesh_device_local_bytes == 4 * 6 * 28,
            "actual created/updated meshes were not all device local");
        Require(selections.allocations == 3 && selections.binds == 3 && selections.releases == 0 &&
            selections.requested_bytes == 864 && selections.live_bytes >= 864,
            "actual three-slot selection buffers were not retained");
        Require(bool(buffer_rows), "buffer observation output failed");
    }
    Require(bool(rows), "native observation output failed");
    for (const auto& slot : observed->binding_slots)
        Require(slot.mesh.image_descriptors == 7 && slot.mesh.uniform_descriptors == 2 && slot.mesh.sampler_descriptors == 2 &&
            slot.selection.uniform_descriptors == 1 && slot.selection.image_descriptors == 0 && slot.selection.sampler_descriptors == 0,
            "final native descriptor totals mismatch");
    renderer.DestroyMesh(left); renderer.DestroyMesh(right);
    renderer.DestroyTextureArray(a); renderer.DestroyTextureArray(b); renderer.DestroyTextureArray(c);
    renderer.Shutdown();
    Require(observed->native_destroyed && observed->loader_released && !observed->quarantined && observed->live_bytes == 0 &&
        observed->buffers_created == observed->buffers_released && observed->images_created == observed->images_released,
        "normal shutdown did not retire native owners/loader");
    for (const auto& slot : observed->binding_slots)
        Require(slot.mesh.pools_released == slot.mesh.pools_created && slot.selection.pools_released == slot.selection.pools_created,
            "normal shutdown leaked descriptor pools");
    Require(observed->warnings == 0 && observed->errors == 0, "Vulkan validation reported diagnostics");
    if (buffer_policy) {
        SaveBufferObservations(*observed, output);
        std::cout << "normal_gpu_buffer_policy=passed; mesh_buffers=4; mesh_device_local_buffers=4; selection_buffers=3"
            << "; selection_requested_bytes=864; warm_selection_allocations=0; warnings=0; errors=0; normal_shutdown=passed\n";
    }
    std::cout << "normal_gpu_frame_bindings=passed; frames=" << frame.frame_id << "; saved_captures=" << captures
        << "; slots=3; uniform_allocations=6; descriptor_pools=6; descriptor_sets=9; image_writes=21; ubo_writes=9; sampler_writes=6"
        << "; warnings=0; errors=0; normal_shutdown=passed\n";
}
}
int main(int argc, char** argv) {
    try {
        Require(argc == 3 || (argc == 4 && std::string(argv[3]) == "--buffer-policy"),
            "usage: probe shader-root new-output-directory [--buffer-policy]");
        const auto output = std::filesystem::absolute(argv[2]);
        Require(!std::filesystem::exists(output), "output must be new"); std::filesystem::create_directories(output);
        Run(std::filesystem::absolute(argv[1]), output, argc == 4); return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
