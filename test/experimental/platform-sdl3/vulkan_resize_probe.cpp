#include "vulkan_probe_common.h"
#include <native_bridge.h>

#include <chrono>
#include <iomanip>
#include <thread>

namespace {
using namespace SymoCraftVulkanExperiment;
using Clock = std::chrono::steady_clock;

struct Observation {
    VkExtent2D sdl{}, client{}, surface{};
    bool minimized{}, iconic{};
};
struct Frame {
    std::string name;
    Observation pixels;
    VkExtent2D extent{};
    float projection_x{};
    unsigned generation{};
};
struct Probe {
    Context context;
    HWND hwnd{};
    VkInstance original_instance{};
    VkSurfaceKHR original_surface{};
    VkDevice original_device{};
    std::vector<std::uint32_t> vs, ps;
    DrawResources resources;
    std::vector<Frame> frames;
    std::filesystem::path output;
    Observation minimized_pixels;
    std::array<unsigned, 4> pause_before{}, pause_after{};
    double pause_seconds{};
    unsigned paused_steps{}, geometry_uploads{}, texture_uploads{};
    bool suspended = false;

    Observation Observe() {
        context.window->PollInt();
        Observation value;
        Require(context.window->width >= 0 && context.window->height >= 0, "negative SDL drawable extent");
        value.sdl = {static_cast<unsigned>(context.window->width), static_cast<unsigned>(context.window->height)};
        value.minimized = context.window->Minimized();
        value.iconic = IsIconic(hwnd) != 0;
        const auto previous = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        Require(previous != nullptr, "cannot establish a scoped physical-pixel query context");
        RECT rectangle{};
        const auto success = GetClientRect(hwnd, &rectangle);
        const auto restored = SetThreadDpiAwarenessContext(previous);
        Require(success && restored, "physical client query/DPI context restore failed");
        Require(rectangle.right >= rectangle.left && rectangle.bottom >= rectangle.top, "invalid Win32 client rectangle");
        value.client = {static_cast<unsigned>(rectangle.right - rectangle.left), static_cast<unsigned>(rectangle.bottom - rectangle.top)};
        const auto query = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(context.get(context.instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
        Require(query != nullptr, "missing instance-level surface capability query");
        VkSurfaceCapabilitiesKHR caps{};
        Check(query(context.physical, context.surface, &caps), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR(observe)");
        value.surface = caps.currentExtent;
        return value;
    }
    Observation Await(bool minimized) {
        const auto deadline = Clock::now() + std::chrono::seconds(3);
        do {
            const auto value = Observe();
            if (minimized) {
                if (value.minimized && value.iconic) return value;
            } else if (!value.minimized && !value.iconic && value.sdl.width && value.sdl.height &&
                       value.sdl.width == value.client.width && value.sdl.height == value.client.height &&
                       value.sdl.width == value.surface.width && value.sdl.height == value.surface.height) return value;
            Require(!context.window->ShouldClose(), "probe window closed during resize/minimize");
            context.window->WaitEvents(.02);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } while (Clock::now() < deadline);
        throw std::runtime_error("actual SDL/client/surface state did not converge within 3 seconds");
    }
    std::array<unsigned, 4> Calls() const {
        return {context.acquire_calls, context.submit_calls, context.present_calls, context.swapchain_create_calls};
    }
    void StableOwners() const {
        Require(SymoCraft::Platform::NativeBridge::GetHandle(*context.window) == hwnd && context.instance == original_instance &&
                context.surface == original_surface && context.device == original_device,
                "E03 accidentally hot-recreated its Window/surface/device");
    }
    void Capture(const char* name, Observation pixels, bool rebuild) {
        Require(!pixels.minimized && !pixels.iconic && pixels.surface.width && pixels.surface.height, "non-drawable capture rejected");
        if (rebuild) {
            const auto old = context.swapchain;
            const auto creates = context.swapchain_create_calls, destroys = context.swapchain_destroy_calls;
            context.RebuildSwapchain();
            Require(context.swapchain != old && context.swapchain_create_calls == creates + 1 &&
                    context.swapchain_destroy_calls == destroys + 1, "native swapchain was not actually replaced/retired");
        }
        Require(context.extent.width == pixels.sdl.width && context.extent.height == pixels.sdl.height, "swapchain extent does not match actual drawable pixels");
        suspended = false;
        resources.matrix[0] = static_cast<float>(static_cast<double>(context.extent.height) / (3 * static_cast<double>(context.extent.width)));
        resources.filename = std::string(name) + ".rgba";
        const auto start = context.cleanup.size();
        const auto before = Calls();
        Draw(context, vs, ps, false, output, &resources);
        if (resources.upload_texture) ++texture_uploads;
        resources.upload_texture = false;
        const auto after = Calls();
        Require(after[0] == before[0] + 1 && after[1] == before[1] + 1 && after[2] == before[2] + 1 && !context.pending,
                "capture did not complete actual acquire/submit/present retirement");
        while (context.cleanup.size() > start) { context.cleanup.back()(); context.cleanup.pop_back(); }
        Require(context.swap_views_live == 0 && context.framebuffers_live == 0, "old-size framebuffer/image views survived completion");
        StableOwners();
        context.RequireCleanValidation("after resized frame retirement");
        frames.push_back({name, pixels, context.extent, resources.matrix[0], context.swapchain_create_calls});
        std::cout << "resize_frame=" << name << "; pixels=" << context.extent.width << 'x' << context.extent.height
                  << "; swapchain_generation=" << context.swapchain_create_calls << "; projection_x=" << resources.matrix[0]
                  << "; old_size_views_live=" << context.swap_views_live << "; framebuffers_live=" << context.framebuffers_live << '\n';
    }
    void Resize(const char* name, int width, int height) {
        context.window->SetSize(width, height);
        Capture(name, Await(false), true);
    }
    void Pause() {
        ShowWindow(hwnd, SW_MINIMIZE);
        minimized_pixels = Await(true);
        // Match GameRenderer's existing minimized-to-zero policy, keeping raw pixels separate.
        suspended = true;
        context.extent = {};
        pause_before = Calls();
        const auto start = Clock::now();
        while (Clock::now() - start < std::chrono::seconds(10)) {
            const auto wake = Clock::now() + std::chrono::milliseconds(100);
            const auto observed = Observe();
            Require(observed.minimized && observed.iconic && suspended && !context.extent.width && !context.extent.height,
                    "real minimized/zero-render-extent state changed during the pause");
            Require(resources.vertex.buffer && resources.texture && resources.texture_view && resources.vertex_count == 12,
                    "accepted persistent input became invalid while suspended");
            ++paused_steps;
            Require(Calls() == pause_before, "zero-size candidate issued acquire/submit/present/rebuild");
            context.window->WaitEvents(.09);
            std::this_thread::sleep_until(wake);
        }
        pause_seconds = std::chrono::duration<double>(Clock::now() - start).count();
        pause_after = Calls();
        Require(pause_before == pause_after && paused_steps >= 50 && paused_steps <= 150, "zero-size pause busy-loop/native call accounting mismatch");
        ShowWindow(hwnd, SW_RESTORE);
        Capture("restored", Await(false), true);
    }
    void Run() {
        context.Initialize(true, "SymoCraft Vulkan presentation E03");
        hwnd = SymoCraft::Platform::NativeBridge::GetHandle(*context.window);
        original_instance = context.instance; original_surface = context.surface; original_device = context.device;
        const auto vertices = Vertices();
        const std::array<std::uint8_t, 32> texels{
            255,32,64,255,16,240,80,255,48,96,224,255,192,128,24,255,
            24,160,208,255,232,48,176,255,112,224,40,255,64,80,144,255};
        resources.vertex = context.MakeBuffer(sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        context.Write(resources.vertex, vertices.data()); ++geometry_uploads;
        resources.vertex_count = static_cast<unsigned>(vertices.size());
        resources.staging = context.MakeBuffer(sizeof(texels), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        context.Write(resources.staging, texels.data());
        resources.texture = context.Texture(VK_FORMAT_R8G8B8A8_UNORM);
        resources.texture_view = context.View(resources.texture, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 2);
        resources.upload_texture = true;
        frames.reserve(6);
        Capture("initial", Await(false), false);
        Resize("larger", 800, 600);
        Resize("wide", 960, 540);
        Resize("narrow", 720, 600);
        Pause();
        Resize("final", 640, 480);
        Require(context.acquire_calls == 6 && context.submit_calls == 6 && context.present_calls == 6 && context.swapchain_create_calls == 6 &&
                context.swapchain_destroy_calls == 5 && geometry_uploads == 1 && texture_uploads == 1 && !suspended,
                "E03 persistent resource/actual presentation accounting mismatch");
    }
    void Report() {
        std::ofstream file(output / "native.json");
        file << std::setprecision(12);
        file << "{\"schema\":\"symocraft.vulkan-r1-e03.native.v1\",\n\"device\":{\"device_name\":" << JsonString(context.device_name)
             << ",\"vendor_id\":4318,\"device_id\":11269,\"api_version\":" << context.api_version << ",\"driver_version\":"
             << JsonString(context.driver_version) << ",\"queue_family\":" << context.queue_family << "},\n"
             << "\"samples\":1,\"present_mode\":\"Fifo\",\"swapchain_format\":" << JsonString(context.format == VK_FORMAT_B8G8R8A8_UNORM ? "B8G8R8A8_UNORM" : "R8G8B8A8_UNORM")
             << ",\"swapchain_color_space\":\"SRGB_NONLINEAR\",\"swapchain_clipped\":false,\n\"validation\":{\"enabled\":true,\"synchronization_enabled\":true,\"warnings\":"
             << context.diagnostics.warnings.load() << ",\"errors\":" << context.diagnostics.errors.load() << "},\n"
             << "\"direction_shader_input\":" << (DirectionShaderInput ? "true" : "false") << ",\"window_surface_device_unchanged\":true,"
             << "\"geometry_uploads\":" << geometry_uploads << ",\"texture_uploads\":" << texture_uploads
             << ",\"acquire_calls\":" << context.acquire_calls << ",\"submit_calls\":" << context.submit_calls << ",\"present_calls\":" << context.present_calls
             << ",\"swapchain_create_calls\":" << context.swapchain_create_calls << ",\"swapchain_destroy_calls\":" << context.swapchain_destroy_calls
             << ",\"swap_views_live\":" << context.swap_views_live << ",\"framebuffers_live\":" << context.framebuffers_live
             << ",\"cleanup_completed\":true,\"present_retirement\":\"EXT_swapchain_maintenance1_present_fence\",\n"
             << "\"pause\":{\"duration_seconds\":" << pause_seconds << ",\"steps\":" << paused_steps
             << ",\"sdl_pixels\": [" << minimized_pixels.sdl.width << ',' << minimized_pixels.sdl.height << "],\"client_pixels\": ["
             << minimized_pixels.client.width << ',' << minimized_pixels.client.height << "],\"surface_pixels\": [" << minimized_pixels.surface.width << ',' << minimized_pixels.surface.height
             << "],\"minimized\":true,\"iconic\":true,\"effective_render_extent\": [0,0],\"calls_before\": ["
             << pause_before[0] << ',' << pause_before[1] << ',' << pause_before[2] << ',' << pause_before[3] << "],\"calls_after\": ["
             << pause_after[0] << ',' << pause_after[1] << ',' << pause_after[2] << ',' << pause_after[3] << "]},\n\"frames\":[\n";
        for (std::size_t i = 0; i < frames.size(); ++i) {
            if (i) file << ",\n";
            const auto& frame = frames[i];
            file << "{\"name\":" << JsonString(frame.name) << ",\"file\":" << JsonString(frame.name + ".rgba") << ",\"extent\": [" << frame.extent.width << ',' << frame.extent.height
                 << "],\"sdl_pixels\": [" << frame.pixels.sdl.width << ',' << frame.pixels.sdl.height << "],\"client_pixels\": ["
                 << frame.pixels.client.width << ',' << frame.pixels.client.height << "],\"surface_pixels\": [" << frame.pixels.surface.width << ',' << frame.pixels.surface.height
                 << "],\"generation\":" << frame.generation << ",\"projection_x\":" << frame.projection_x << ",\"draw_and_present_completed\":true,\"old_size_views_live\":0,\"framebuffers_live\":0}";
        }
        file << "\n],\"not_run\": [\"production Vulkan\",\"MSAA/depth rebuild\",\"out-of-date/suboptimal failure injection\",\"full A11/A15\"]}\n";
        Require(file.good(), "E03 native report write failed");
    }
};
}

int main(int argc, char** argv) {
    std::cout << std::unitbuf;
    auto probe = std::make_unique<Probe>();
    try {
        std::filesystem::path vertex, fragment;
        for (int i = 1; i < argc; i += 2) {
            Require(i + 1 < argc, "CLI requires --vertex --fragment --output pairs");
            const std::string option = argv[i];
            if (option == "--vertex") vertex = argv[i + 1];
            else if (option == "--fragment") fragment = argv[i + 1];
            else if (option == "--output") probe->output = argv[i + 1];
            else throw std::runtime_error("unknown E03 CLI option");
        }
        Require(!vertex.empty() && !fragment.empty() && !probe->output.empty(), "missing E03 input");
        Require(!std::filesystem::exists(probe->output), "E03 output must be a fresh directory");
        std::filesystem::create_directories(probe->output);
        probe->vs = Shader(vertex); probe->ps = Shader(fragment);
        probe->Run();
        probe->context.Close();
        probe->context.RequireCleanValidation("after E03 shutdown");
        Require(probe->context.swapchain_destroy_calls == 6, "final swapchain did not release");
        probe->Report();
        std::cout << "E03_exit=0; synchronization_validation=1; validation_warning=0; validation_error=0; cleanup_completed=1\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[Vulkan E03] " << error.what() << '\n';
        if (probe->context.pending) { std::cerr << "[terminal retention] pending native/present work: keep full owner graph until process exit\n"; (void)probe.release(); }
        else {
            try { probe->context.Close(); }
            catch (const std::exception& close) { std::cerr << "[terminal retention] " << close.what() << '\n'; (void)probe.release(); }
        }
        return 1;
    }
}
