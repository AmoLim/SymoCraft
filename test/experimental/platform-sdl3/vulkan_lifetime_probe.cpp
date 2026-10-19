#include "vulkan_probe_common.h"

#include <cmath>
#include <span>

namespace {
using namespace SymoCraftVulkanExperiment;

struct Counts {
    unsigned mesh_created{}, mesh_released{}, mesh_live{}, mesh_peak{};
    unsigned mesh_buffers_created{}, mesh_buffers_released{};
    unsigned texture_created{}, texture_released{}, texture_live{}, texture_peak{};
    unsigned stale_rejections{}, submissions{};
};

// Move only this resource's cleanup actions out of the device owner's stack.
struct NativeOwner {
    std::vector<std::function<void()>> cleanup;
    void Accept(Context& context, std::size_t start) {
        cleanup.reserve(context.cleanup.size() - start);
        for (auto i = start; i < context.cleanup.size(); ++i) cleanup.push_back(std::move(context.cleanup[i]));
        context.cleanup.resize(start);
    }
    ~NativeOwner() { while (!cleanup.empty()) { cleanup.back()(); cleanup.pop_back(); } }
};

struct MeshVersion {
    NativeOwner native;
    std::vector<Vertex> snapshot;
    Context::Buffer buffer;
    Counts* counts{};
    ~MeshVersion() {
        // The native owner must be released before counting actual reclamation.
        while (!native.cleanup.empty()) { native.cleanup.back()(); native.cleanup.pop_back(); }
        if (counts) {
            ++counts->mesh_released; --counts->mesh_live;
            if (buffer.buffer) ++counts->mesh_buffers_released;
        }
    }
};
struct TextureVersion {
    NativeOwner native;
    std::vector<std::uint8_t> snapshot;
    Context::Buffer staging;
    VkImage image{};
    VkImageView view{};
    Counts* counts{};
    bool uploaded = false;
    ~TextureVersion() {
        while (!native.cleanup.empty()) { native.cleanup.back()(); native.cleanup.pop_back(); }
        if (counts) { ++counts->texture_released; --counts->texture_live; }
    }
};
struct MeshHandle { std::uint64_t generation{}; };
struct TextureHandle { std::uint64_t generation{}; };

// Experiment-only single-slot registry; not a production Renderer or public API.
struct CandidateRegistry {
    Context& context;
    Counts& counts;
    std::uint64_t mesh_generation = 1, texture_generation = 1;
    std::shared_ptr<MeshVersion> mesh;
    std::shared_ptr<TextureVersion> texture;

    std::shared_ptr<MeshVersion> Get(MeshHandle handle) {
        if (!mesh || handle.generation != mesh_generation) {
            ++counts.stale_rejections;
            throw std::invalid_argument("stale_mesh");
        }
        return mesh;
    }
    std::shared_ptr<TextureVersion> Get(TextureHandle handle) {
        if (!texture || handle.generation != texture_generation) {
            ++counts.stale_rejections;
            throw std::invalid_argument("stale_texture");
        }
        return texture;
    }
    std::shared_ptr<MeshVersion> Prepare(std::span<const Vertex> source) {
        Require(source.size() % 3 == 0 && source.size() <= std::numeric_limits<std::uint32_t>::max(), "invalid triangle input");
        for (const auto& vertex : source) {
            for (const auto value : vertex.uv_layer) Require(std::isfinite(value), "nonfinite mesh UV/layer");
            Require(vertex.uv_layer[2] == 0 || vertex.uv_layer[2] == 1, "invalid fixture texture layer");
        }
        auto prepared = std::make_shared<MeshVersion>();
        prepared->snapshot.assign(source.begin(), source.end());
        const auto start = context.cleanup.size();
        if (!prepared->snapshot.empty()) {
            prepared->buffer = context.MakeBuffer(prepared->snapshot.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            context.Write(prepared->buffer, prepared->snapshot.data());
        }
        prepared->native.Accept(context, start);
        prepared->counts = &counts;
        ++counts.mesh_created;
        if (prepared->buffer.buffer) ++counts.mesh_buffers_created;
        counts.mesh_peak = std::max(counts.mesh_peak, ++counts.mesh_live);
        return prepared;
    }
    MeshHandle Create(std::span<const Vertex> source) {
        Require(!mesh, "mesh slot already occupied");
        mesh = Prepare(source);
        return {mesh_generation};
    }
    void Update(MeshHandle handle, std::span<const Vertex> source) {
        (void)Get(handle);
        auto prepared = Prepare(source);
        mesh = std::move(prepared);
    }
    TextureHandle Create(std::span<const std::uint8_t> source) {
        Require(!texture && source.size() == 32, "invalid two-layer texture input");
        auto prepared = std::make_shared<TextureVersion>();
        prepared->snapshot.assign(source.begin(), source.end());
        const auto start = context.cleanup.size();
        prepared->staging = context.MakeBuffer(prepared->snapshot.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        context.Write(prepared->staging, prepared->snapshot.data());
        prepared->image = context.Texture(VK_FORMAT_R8G8B8A8_UNORM);
        prepared->view = context.View(prepared->image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 2);
        prepared->native.Accept(context, start);
        prepared->counts = &counts;
        ++counts.texture_created;
        counts.texture_peak = std::max(counts.texture_peak, ++counts.texture_live);
        texture = std::move(prepared);
        return {texture_generation};
    }
    void Destroy(MeshHandle handle) {
        (void)Get(handle);
        Require(mesh_generation < std::numeric_limits<std::uint64_t>::max(), "mesh generation exhausted");
        mesh.reset();
        ++mesh_generation;
    }
    void Destroy(TextureHandle handle) {
        (void)Get(handle);
        Require(texture_generation < std::numeric_limits<std::uint64_t>::max(), "texture generation exhausted");
        texture.reset();
        ++texture_generation;
    }
};

struct Event {
    std::string name;
    unsigned vertices{}, mesh_before{}, mesh_during{}, mesh_after{}, texture_before{}, texture_during{}, texture_after{};
    VkResult fence_before{}, fence_after_mutation{}, fence_completed{};
    std::uint64_t gate_before{}, gate_after_mutation{};
    bool retained_mesh{}, retained_texture{}, retired_mesh{}, retired_texture{}, distinct_buffer{};
    unsigned stale_before{}, stale_after{};
};
struct FrameReferences {
    std::shared_ptr<MeshVersion> mesh;
    std::shared_ptr<TextureVersion> texture;
};

struct Experiment {
    Context context;
    Counts counts;
    std::unique_ptr<CandidateRegistry> registry;
    std::vector<std::unique_ptr<FrameReferences>> frames;
    std::vector<Event> events;
    std::vector<std::uint32_t> vertex_shader, fragment_shader;
    std::filesystem::path output;

    template<class Function> void Reject(Function function, const char* category) {
        const auto before = counts.submissions;
        bool rejected = false;
        try { function(); }
        catch (const std::invalid_argument& error) { rejected = std::strcmp(error.what(), category) == 0; }
        Require(rejected && counts.submissions == before, "stale handle was not rejected before submission");
    }

    template<class Mutation> void Frame(const char* name, MeshHandle mesh_handle, TextureHandle texture_handle, Mutation mutation) {
        events.emplace_back();
        auto& event = events.back();
        event.name = name;
        auto references = std::make_unique<FrameReferences>();
        references->mesh = registry->Get(mesh_handle);
        references->texture = registry->Get(texture_handle);
        auto& frame = *references;
        const std::weak_ptr<MeshVersion> old_mesh = frame.mesh;
        const std::weak_ptr<TextureVersion> old_texture = frame.texture;
        frames.push_back(std::move(references));
        event.vertices = static_cast<unsigned>(frame.mesh->snapshot.size());
        event.mesh_before = counts.mesh_released;
        event.texture_before = counts.texture_released;
        event.stale_before = counts.stale_rejections;
        DrawResources resources;
        resources.vertex = frame.mesh->buffer;
        resources.vertex_count = event.vertices;
        resources.staging = frame.texture->staging;
        resources.texture = frame.texture->image;
        resources.texture_view = frame.texture->view;
        resources.upload_texture = !frame.texture->uploaded;
        resources.filename = event.name + ".rgba";
        VkFence submitted{};
        resources.while_pending = [&](VkFence fence, VkSemaphore gate) {
            submitted = fence;
            ++counts.submissions;
            event.fence_before = context.VK_FUNCTION(vkGetFenceStatus)(context.device, fence);
            Check(context.VK_FUNCTION(vkGetSemaphoreCounterValue)(context.device, gate, &event.gate_before), "vkGetSemaphoreCounterValue(before)");
            Require(event.fence_before == VK_NOT_READY && event.gate_before == 0, "no actual gated in-flight work before mutation");
            mutation(frame, event);
            event.mesh_during = counts.mesh_released;
            event.texture_during = counts.texture_released;
            event.stale_after = counts.stale_rejections;
            event.retained_mesh = !old_mesh.expired();
            event.retained_texture = !old_texture.expired();
            event.fence_after_mutation = context.VK_FUNCTION(vkGetFenceStatus)(context.device, fence);
            Check(context.VK_FUNCTION(vkGetSemaphoreCounterValue)(context.device, gate, &event.gate_after_mutation), "vkGetSemaphoreCounterValue(after mutation)");
            Require(event.retained_mesh && event.retained_texture && event.fence_after_mutation == VK_NOT_READY && event.gate_after_mutation == 0,
                "old native resource reclaimed or GPU gate completed during mutation");
        };
        const auto start = context.cleanup.size();
        Draw(context, vertex_shader, fragment_shader, false, output, &resources);
        event.fence_completed = context.VK_FUNCTION(vkGetFenceStatus)(context.device, submitted);
        Require(event.fence_completed == VK_SUCCESS && !context.pending, "draw/present retirement is incomplete");
        frame.texture->uploaded = true;
        // Descriptor/command references go first, then versions held by the completed frame.
        while (context.cleanup.size() > start) { context.cleanup.back()(); context.cleanup.pop_back(); }
        frame.mesh.reset(); frame.texture.reset();
        event.retired_mesh = old_mesh.expired(); event.retired_texture = old_texture.expired();
        event.mesh_after = counts.mesh_released; event.texture_after = counts.texture_released;
        context.RequireCleanValidation("after actual frame/version retirement");
        std::cout << "lifetime_frame=" << name << "; fence_before=" << event.fence_before
                  << "; fence_after_mutation=" << event.fence_after_mutation << "; gate_before=" << event.gate_before
                  << "; gate_after_mutation=" << event.gate_after_mutation << "; draw_fence_completed=" << event.fence_completed
                  << "; mesh_released=" << event.mesh_before << '/' << event.mesh_during << '/' << event.mesh_after
                  << "; texture_released=" << event.texture_before << '/' << event.texture_during << '/' << event.texture_after << '\n';
    }

    void Run() {
        context.Initialize(true);
        events.reserve(5); frames.reserve(5);
        registry = std::make_unique<CandidateRegistry>(CandidateRegistry{context, counts});
        auto geometry = Vertices();
        std::vector<Vertex> caller(geometry.begin(), geometry.end());
        const auto mesh = registry->Create(std::span<const Vertex>(caller));
        std::memset(caller.data(), 0, caller.size() * sizeof(Vertex));
        std::vector<Vertex>().swap(caller);
        std::vector<std::uint8_t> pixels{
            255,32,64,255,16,240,80,255,48,96,224,255,192,128,24,255,
            24,160,208,255,232,48,176,255,112,224,40,255,64,80,144,255};
        const auto original_texels = pixels;
        const auto texture = registry->Create(std::span<const std::uint8_t>(pixels));
        std::fill(pixels.begin(), pixels.end(), 0);
        std::vector<std::uint8_t>().swap(pixels);

        Frame("created-before-update", mesh, texture, [&](auto& old, auto& event) {
            caller.assign(geometry.begin(), geometry.end());
            for (auto& vertex : caller) vertex.uv_layer[2] = 1 - vertex.uv_layer[2];
            registry->Update(mesh, caller);
            event.distinct_buffer = registry->mesh->buffer.buffer != old.mesh->buffer.buffer;
            Require(event.distinct_buffer, "update overwrote the native in-flight vertex buffer");
            std::memset(caller.data(), 0, caller.size() * sizeof(Vertex));
            std::vector<Vertex>().swap(caller);
        });
        Require(events.back().retired_mesh && !events.back().retired_texture && counts.mesh_released == 1, "updated old mesh was not finally reclaimed");

        Frame("updated-before-clear", mesh, texture, [&](auto&, auto&) { registry->Update(mesh, {}); });
        Require(events.back().retired_mesh && counts.mesh_released == 2 && registry->mesh->snapshot.empty(), "clear did not replace/retire prior accepted mesh");

        Frame("cleared-before-restore", mesh, texture, [&](auto&, auto&) {
            caller.assign(geometry.begin(), geometry.end());
            registry->Update(mesh, caller);
            std::memset(caller.data(), 0, caller.size() * sizeof(Vertex));
            std::vector<Vertex>().swap(caller);
        });
        Require(events.back().vertices == 0 && events.back().retired_mesh && counts.mesh_released == 3, "empty version did not produce/retire a zero-draw frame");

        Frame("restored-before-destroy", mesh, texture, [&](auto&, auto&) {
            registry->Destroy(mesh); registry->Destroy(texture);
            Reject([&] { registry->Update(mesh, {}); }, "stale_mesh");
            Reject([&] { registry->Get(mesh); }, "stale_mesh");
            Reject([&] { registry->Destroy(mesh); }, "stale_mesh");
            Reject([&] { registry->Get(texture); }, "stale_texture");
            Reject([&] { registry->Destroy(texture); }, "stale_texture");
        });
        Require(events.back().retired_mesh && events.back().retired_texture && counts.mesh_live == 0 && counts.texture_live == 0,
            "logical deletion did not lead to actual native reclamation after completion");

        caller.assign(geometry.begin(), geometry.end());
        const auto recreated_mesh = registry->Create(std::span<const Vertex>(caller));
        std::memset(caller.data(), 0, caller.size() * sizeof(Vertex));
        std::vector<Vertex>().swap(caller);
        pixels.assign(original_texels.begin() + 16, original_texels.end());
        pixels.insert(pixels.end(), original_texels.begin(), original_texels.begin() + 16);
        const auto recreated_texture = registry->Create(std::span<const std::uint8_t>(pixels));
        std::fill(pixels.begin(), pixels.end(), 0); std::vector<std::uint8_t>().swap(pixels);
        Require(recreated_mesh.generation == 2 && recreated_texture.generation == 2, "recreated slot did not change handle generation");
        Reject([&] { registry->Get(mesh); }, "stale_mesh");
        Reject([&] { registry->Get(texture); }, "stale_texture");
        Frame("recreated-before-destroy", recreated_mesh, recreated_texture, [&](auto&, auto&) {
            registry->Destroy(recreated_mesh); registry->Destroy(recreated_texture);
            Reject([&] { registry->Get(recreated_mesh); }, "stale_mesh");
            Reject([&] { registry->Get(recreated_texture); }, "stale_texture");
        });
        Require(events.back().retired_mesh && events.back().retired_texture && counts.mesh_created == 5 && counts.mesh_released == 5
            && counts.texture_created == 2 && counts.texture_released == 2 && counts.mesh_live == 0 && counts.texture_live == 0
            && counts.mesh_buffers_created == 4 && counts.mesh_buffers_released == 4
            && counts.stale_rejections == 9 && counts.submissions == 5, "final lifetime/resource accounting mismatch");
        for (const auto& event : events) Require(event.mesh_during == event.mesh_before && event.texture_during == event.texture_before,
            "native version was freed before its gated frame completed");
    }

    void Close() {
        Require(!context.pending, "lifetime experiment has no completion proof; retain entire owner graph");
        frames.clear(); registry.reset(); context.Close();
    }
    void Report() {
        std::ofstream file(output / "native.json");
        file << "{\n\"schema\":\"symocraft.vulkan-r1-e02.native.v1\",\n\"device\":{\"device_name\":" << JsonString(context.device_name)
             << ",\"vendor_id\":4318,\"device_id\":11269,\"api_version\":" << context.api_version << ",\"driver_version\":"
             << JsonString(context.driver_version) << ",\"queue_family\":" << context.queue_family << "},\n"
             << "\"extent\":{\"width\":" << context.extent.width << ",\"height\":" << context.extent.height << "},"
             << "\"samples\":1,\"present_mode\":\"Fifo\",\"swapchain_format\":" << JsonString(context.format == VK_FORMAT_B8G8R8A8_UNORM ? "B8G8R8A8_UNORM" : "R8G8B8A8_UNORM")
             << ",\"swapchain_color_space\":\"SRGB_NONLINEAR\",\n\"validation\":{\"enabled\":true,\"synchronization_enabled\":true,\"warnings\":"
             << context.diagnostics.warnings.load() << ",\"errors\":" << context.diagnostics.errors.load() << "},\n"
             << "\"direction_shader_input\":" << (DirectionShaderInput ? "true" : "false")
             << ",\"controlled_native_timeline_gate\":true,\"caller_mesh_and_texture_poisoned_and_freed\":true,"
             << "\"cleanup_completed\":true,\"present_retirement\":\"EXT_swapchain_maintenance1_present_fence\",\n"
             << "\"counts\":{\"mesh_versions_created\":" << counts.mesh_created << ",\"mesh_versions_released\":" << counts.mesh_released << ",\"mesh_versions_live\":" << counts.mesh_live
             << ",\"mesh_peak\":" << counts.mesh_peak << ",\"texture_created\":" << counts.texture_created << ",\"texture_released\":" << counts.texture_released
             << ",\"mesh_buffers_created\":" << counts.mesh_buffers_created << ",\"mesh_buffers_released\":" << counts.mesh_buffers_released
             << ",\"texture_live\":" << counts.texture_live << ",\"texture_peak\":" << counts.texture_peak << ",\"stale_rejections\":" << counts.stale_rejections
             << ",\"submissions\":" << counts.submissions << "},\n\"frames\":[\n";
        for (std::size_t i = 0; i < events.size(); ++i) {
            if (i) file << ",\n";
            const auto& event = events[i];
            file << "{\"name\":" << JsonString(event.name) << ",\"file\":" << JsonString(event.name + ".rgba") << ",\"vertices\":" << event.vertices
                 << ",\"fence_before\":" << event.fence_before << ",\"fence_after_mutation\":" << event.fence_after_mutation << ",\"fence_completed\":" << event.fence_completed
                 << ",\"gate_before\":" << event.gate_before << ",\"gate_after_mutation\":" << event.gate_after_mutation
                 << ",\"mesh_released\": [" << event.mesh_before << ',' << event.mesh_during << ',' << event.mesh_after
                 << "],\"texture_released\": [" << event.texture_before << ',' << event.texture_during << ',' << event.texture_after
                 << "],\"retained_mesh\":" << (event.retained_mesh ? "true" : "false") << ",\"retained_texture\":" << (event.retained_texture ? "true" : "false")
                 << ",\"retired_mesh\":" << (event.retired_mesh ? "true" : "false") << ",\"retired_texture\":" << (event.retired_texture ? "true" : "false")
                 << ",\"distinct_update_buffer\":" << (event.distinct_buffer ? "true" : "false")
                 << ",\"stale_rejections\": [" << event.stale_before << ',' << event.stale_after << "],\"present_fence_completed\":true}";
        }
        file << "\n],\"not_run\": [\"production Vulkan\",\"E03\",\"full A08-A10/A15 capacity and failures\",\"natural long GPU stall\"]\n}\n";
        Require(file.good(), "native lifetime report output failed");
    }
};
}

int main(int argc, char** argv) {
    std::cout << std::unitbuf;
    auto experiment = std::make_unique<Experiment>();
    try {
        std::filesystem::path vertex, fragment;
        for (int i = 1; i < argc; i += 2) {
            Require(i + 1 < argc, "CLI requires --vertex --fragment --output pairs");
            const std::string option = argv[i];
            if (option == "--vertex") vertex = argv[i + 1];
            else if (option == "--fragment") fragment = argv[i + 1];
            else if (option == "--output") experiment->output = argv[i + 1];
            else throw std::runtime_error("unknown CLI option");
        }
        Require(!vertex.empty() && !fragment.empty() && !experiment->output.empty(), "missing lifetime CLI input");
        Require(!std::filesystem::exists(experiment->output), "output must be a fresh directory");
        std::filesystem::create_directories(experiment->output);
        experiment->vertex_shader = Shader(vertex); experiment->fragment_shader = Shader(fragment);
        experiment->Run();
        experiment->Close();
        experiment->context.RequireCleanValidation("after lifetime shutdown");
        experiment->Report();
        std::cout << "E02_exit=0; validation_warning=0; validation_error=0; synchronization_validation=1; cleanup_completed=1\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[Vulkan E02] " << error.what() << '\n';
        if (experiment->context.pending) {
            std::cerr << "[terminal retention] no completion proof; preserving entire frame/registry/native/Window/loader owner graph until process exit\n";
            (void)experiment.release();
        } else {
            try { experiment->Close(); }
            catch (const std::exception& close) { std::cerr << "[terminal retention] " << close.what() << '\n'; (void)experiment.release(); }
        }
        return 1;
    }
}
