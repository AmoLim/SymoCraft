#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <symocraft/platform/window.h>
#include <graphics_bridge.h>
#include <native_bridge.h>
#include <glad/glad.h>
#ifdef SYMOCRAFT_PLATFORM_PROBE_VULKAN
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#include <vulkan_bridge.h>
#endif
#include <array>
#include <cstdint>
#include <cstdio>
#include <io.h>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "failure_fixture.h"

namespace {
    using SymoCraft::Window;
    using SymoCraft::WindowMode;
    using SymoCraft::Platform::GraphicsBridge;
    using SymoCraft::Platform::NativeBridge;

    std::string JsonString(const std::string& value) {
        std::ostringstream out;
        out << '"';
        constexpr char digits[] = "0123456789abcdef";
        for (const auto character : value) {
            const auto byte = static_cast<unsigned char>(character);
            if (byte == '"' || byte == '\\') out << '\\' << character;
            else if (byte < 0x20) out << "\\u00" << digits[byte >> 4] << digits[byte & 15];
            else out << character;
        }
        out << '"';
        return out.str();
    }

    struct Report {
        std::map<std::string, std::string> fields;
        void String(const std::string& key, const std::string& value) { fields[key] = JsonString(value); }
        template<typename Value> void Number(const std::string& key, Value value) {
            std::ostringstream out;
            out << value;
            fields[key] = out.str();
        }
        void Boolean(const std::string& key, bool value) { fields[key] = value ? "true" : "false"; }
        std::string Json() const {
            std::ostringstream out;
            out << "{\n";
            bool first = true;
            for (const auto& [key, value] : fields) {
                if (!first) out << ",\n";
                out << "  " << JsonString(key) << ": " << value;
                first = false;
            }
            out << "\n}\n";
            return out.str();
        }
    };

    void Require(bool condition, const std::string& operation) {
        if (!condition) throw std::runtime_error(operation);
    }

    template<typename Callable> void Reject(Callable&& callable, Report& report, const std::string& name) {
        std::string diagnostic;
        try { callable(); }
        catch (const std::exception& error) {
            diagnostic = error.what();
            SDL_SetError("probe changed the SDL diagnostic while the production exception was alive");
            Require(std::string(error.what()) == diagnostic, "Production exception borrowed mutable SDL diagnostic text: " + name);
        }
        Require(!diagnostic.empty(), "Expected a diagnostic exception: " + name);
        SDL_SetError("probe changed the SDL diagnostic after catching the production exception");
        report.String(name + "_diagnostic", diagnostic);
        report.Boolean(name + "_rejected", true);
    }

    struct Runtime {
        bool active{};
        Runtime() { Window::Init(); active = true; }
        ~Runtime() {
            if (active) {
                try { Window::Free(); }
                catch (const std::exception& error) { std::cerr << "cleanup: Window::Free: " << error.what() << '\n'; }
            }
        }
        void Close(Report& report) {
            Window::Free();
            active = false;
            Window::Free();
            report.Boolean("sdl_shutdown_completed", (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0);
            Require((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0, "Platform did not release its video lifetime");
            report.Boolean("free_repeated", true);
        }
    };

    struct Options {
        std::string mode{"gl"};
        std::string fault;
        bool wait_close{}, benchmark{}, allow_unfocused{};
        int frames{8};
    };

    Options Parse(int argc, char** argv) {
        Options options;
        for (int index = 1; index < argc; ++index) {
            const std::string argument{argv[index]};
            if (argument == "--wait-close") options.wait_close = true;
            else if (argument == "--benchmark") options.benchmark = true;
            else if (argument == "--allow-unfocused") options.allow_unfocused = true;
            else if (argument == "--mode" && index + 1 < argc) options.mode = argv[++index];
            else if (argument == "--fault" && index + 1 < argc) options.fault = argv[++index];
            else if (argument == "--frames" && index + 1 < argc) {
                std::size_t consumed{};
                const std::string value{argv[++index]};
                options.frames = std::stoi(value, &consumed);
                Require(consumed == value.size() && options.frames > 0 && options.frames <= 120, "Invalid --frames");
            } else throw std::runtime_error("Unknown or incomplete argument: " + argument);
        }
        Require(options.mode == "gl" || options.mode == "native" || options.mode == "vulkan" ||
            options.mode == "lifecycle", "Unsupported --mode");
#ifndef SYMOCRAFT_PLATFORM_PROBE_VULKAN
        Require(options.mode != "vulkan", "Vulkan mode was not compiled into this production bridge probe");
#endif
        return options;
    }

    void ArmFailure(const std::string& name) {
        const auto fixture = GetModuleHandleW(L"symocraft_platform_failure_fixture.dll");
        Require(fixture != nullptr, "Requested fault fixture was not loaded by the actual static SDL dynamic API");
        const auto arm = reinterpret_cast<ArmPlatformFailure>(GetProcAddress(fixture, "ProbeArmFailure"));
        Require(arm != nullptr, "Fault fixture arm entry is missing");
        arm(name.c_str());
    }

    void AddFailureTrace(Report& report) {
        const auto fixture = GetModuleHandleW(L"symocraft_platform_failure_fixture.dll");
        if (!fixture) return;
        const auto read = reinterpret_cast<ReadPlatformFailureTrace>(GetProcAddress(fixture, "ProbeReadFailureTrace"));
        if (!read) return;
        const auto trace = read();
        report.Boolean("restricted_fixture_loaded", trace.fixture_loaded != 0);
        report.Number("fixture_failures_injected", trace.failures_injected);
        report.Number("fixture_windows_created", trace.windows_created);
        report.Number("fixture_windows_destroyed", trace.windows_destroyed);
        report.Number("fixture_contexts_created", trace.contexts_created);
        report.Number("fixture_contexts_destroyed", trace.contexts_destroyed);
        report.Number("fixture_video_quit_calls", trace.video_quit_calls);
        report.Number("fixture_surface_create_calls", trace.surface_create_calls);
        report.Number("fixture_surfaces_destroyed", trace.surfaces_destroyed);
    }

    void* GlProcedure(const char* name) { return reinterpret_cast<void*>(GraphicsBridge::GetProcedure(name)); }

    void PrepareGl(Window& window, Report& report) {
        GraphicsBridge::MakeCurrent(window);
        Require(GraphicsBridge::HasCurrentContext(), "The production GL bridge did not publish a current context");
        Require(gladLoadGLLoader(GlProcedure) != 0 && GLAD_GL_VERSION_4_6, "GLAD could not load actual GL 4.6");
        GLint profile{}, samples{}, buffers{}, flags{}, major{}, minor{};
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
        glGetIntegerv(GL_SAMPLES, &samples);
        glGetIntegerv(GL_SAMPLE_BUFFERS, &buffers);
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        report.Number("gl_major", major); report.Number("gl_minor", minor);
        report.Number("gl_profile", profile); report.Number("gl_samples", samples);
        report.Number("gl_sample_buffers", buffers); report.Number("gl_context_flags", flags);
        const auto text = [](GLenum name) {
            const auto* value = glGetString(name);
            return value ? reinterpret_cast<const char*>(value) : "";
        };
        const std::string version{text(GL_VERSION)}, vendor{text(GL_VENDOR)}, renderer{text(GL_RENDERER)};
        report.String("gl_version", version);
        report.String("gl_vendor", vendor);
        report.String("gl_renderer", renderer);
        Require((major > 4 || (major == 4 && minor >= 6)) &&
            (profile & GL_CONTEXT_CORE_PROFILE_BIT) && samples == 4 && buffers == 1,
            "Driver-reported GL 4.6 Core/4x MSAA attributes differ from production requirements");
#ifndef NDEBUG
        Require((flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0, "Debug production context lacks the debug flag");
#endif
        int interval{};
        GraphicsBridge::SetVsync(true);
        Require(SDL_GL_GetSwapInterval(&interval), "SDL_GL_GetSwapInterval failed after production SetVsync(true)");
        report.Number("vsync_on_actual", interval);
        Require(interval == 1, "VSync-on request did not become interval 1");
        GraphicsBridge::SetVsync(false);
        Require(SDL_GL_GetSwapInterval(&interval), "SDL_GL_GetSwapInterval failed after production SetVsync(false)");
        report.Number("vsync_off_actual", interval);
        Require(interval == 0, "VSync-off request did not become interval 0");
        Require(GraphicsBridge::GetProcedure("glSymoCraftDefinitelyMissingProcedure") == nullptr,
            "An unavailable optional GL procedure was not represented as null");
        Require(!GraphicsBridge::ExtensionSupported("GL_SYMOCRAFT_definitely_missing_extension"),
            "A nonexistent optional extension was reported as supported");
        report.Boolean("optional_gl_queries_absent", true);
        Reject([] { GraphicsBridge::GetProcedure(nullptr); }, report, "invalid_procedure_query");
        Reject([] { GraphicsBridge::ExtensionSupported(nullptr); }, report, "invalid_extension_query");
    }

#ifdef SYMOCRAFT_PLATFORM_PROBE_VULKAN
    using SymoCraft::Platform::VulkanBridge;
    void RequireVk(VkResult result, const char* operation) {
        Require(result == VK_SUCCESS, std::string(operation) + ": VkResult=" + std::to_string(result));
    }
    struct VulkanResources {
        Window& window;
        VkInstance instance{};
        VkSurfaceKHR surface{};
        PFN_vkGetInstanceProcAddr get{};
        PFN_vkDestroyInstance destroy{};
        Report* observations{};
        explicit VulkanResources(Window& owner) : window(owner) {}
        ~VulkanResources() {
            try { Close(nullptr); }
            catch (const std::exception& error) { std::cerr << "cleanup: Vulkan resources: " << error.what() << '\n'; }
        }
        void Create(Report& report) {
            observations = &report;
            get = VulkanBridge::GetInstanceProcedureAddress(window);
            Require(get != nullptr, "Production Vulkan bridge returned no SDL loader entry");
            const auto version = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(get(nullptr, "vkEnumerateInstanceVersion"));
            std::uint32_t supported = VK_API_VERSION_1_0;
            if (version) RequireVk(version(&supported), "vkEnumerateInstanceVersion");
            report.Number("vulkan_loader_api_version", supported);
            report.Number("vulkan_header_version", VK_HEADER_VERSION);
            Require(supported >= VK_API_VERSION_1_2, "The loader does not provide Vulkan 1.2");
            const auto create = reinterpret_cast<PFN_vkCreateInstance>(get(nullptr, "vkCreateInstance"));
            Require(create != nullptr, "SDL loader is missing vkCreateInstance");
            const auto extensions = VulkanBridge::GetRequiredExtensions(window);
            Require(!extensions.empty(), "Production Vulkan bridge returned no required extensions");
            std::vector<const char*> names;
            std::ostringstream joined;
            for (const auto& extension : extensions) {
                if (!names.empty()) joined << ',';
                joined << extension;
                names.push_back(extension.c_str());
            }
            report.String("vulkan_required_extensions", joined.str());
            VkApplicationInfo application{};
            application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            application.pApplicationName = "SymoCraft production SDL3 platform bridge probe";
            application.apiVersion = VK_API_VERSION_1_2;
            VkInstanceCreateInfo settings{};
            settings.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            settings.pApplicationInfo = &application;
            settings.enabledExtensionCount = static_cast<std::uint32_t>(names.size());
            settings.ppEnabledExtensionNames = names.data();
            RequireVk(create(&settings, nullptr, &instance), "vkCreateInstance");
            report.Boolean("vulkan_instance_created", true);
            destroy = reinterpret_cast<PFN_vkDestroyInstance>(get(instance, "vkDestroyInstance"));
            Require(destroy != nullptr, "SDL loader is missing vkDestroyInstance");
            surface = VulkanBridge::CreateSurface(window, instance);
            Require(surface != VK_NULL_HANDLE, "Production Vulkan bridge did not create a real surface");
            report.Boolean("vulkan_surface_created", true);
            report.String("vulkan_loader_source", "production VulkanBridge; SDL window-managed loader; no import library");
            const auto enumerate = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(get(instance, "vkEnumeratePhysicalDevices"));
            Require(enumerate != nullptr, "SDL loader is missing vkEnumeratePhysicalDevices");
            std::uint32_t devices{};
            RequireVk(enumerate(instance, &devices, nullptr), "vkEnumeratePhysicalDevices");
            Require(devices != 0, "No actual Vulkan physical device is available");
            report.Number("vulkan_physical_device_count", devices);
            Reject([&] { VulkanBridge::CreateSurface(window, VK_NULL_HANDLE); }, report, "null_vulkan_instance");
        }
        void Close(Report* report) {
            if (!report) report = observations;
            if (surface) {
                VulkanBridge::DestroySurface(window, instance, surface);
                surface = VK_NULL_HANDLE;
                if (report) report->Boolean("vulkan_surface_destroyed", true);
            }
            if (instance) {
                Require(destroy != nullptr, "Cannot release acquired instance: destroy procedure is absent");
                destroy(instance, nullptr);
                instance = VK_NULL_HANDLE;
                if (report) report->Boolean("vulkan_instance_destroyed", true);
            }
        }
    };
#endif

    void RejectNonGl(Window& window, Report& report) {
        Require(!GraphicsBridge::HasCurrentContext(), "Non-GL window unexpectedly has a current GL context");
        Reject([&] { GraphicsBridge::MakeCurrent(window); }, report, "non_gl_make_current");
        Reject([&] { GraphicsBridge::Present(window); }, report, "non_gl_present");
        Reject([] { GraphicsBridge::SetVsync(true); }, report, "non_gl_vsync");
        Reject([] { GraphicsBridge::GetProcedure("glClear"); }, report, "non_gl_procedure");
        Reject([] { GraphicsBridge::ExtensionSupported("GL_ARB_debug_output"); }, report, "non_gl_extension");
        report.Boolean("non_gl_has_no_context", true);
    }

    void WaitClose(Window& window, HWND hwnd, Report& report) {
        Require(PostMessageW(hwnd, WM_CLOSE, 0, 0) != FALSE, "PostMessageW(WM_CLOSE) failed");
        const auto start = Window::Time();
        while (!window.ShouldClose() && Window::Time() - start < 2.0) {
            window.WaitEvents(0.02);
            window.PollInt();
        }
        Require(window.ShouldClose(), "Production WaitEvents lost the native WM_CLOSE event");
        report.Boolean("wait_close_received", true);
    }

    SDL_Window* FindSdlWindow(HWND hwnd) {
        int count{};
        std::unique_ptr<SDL_Window*, decltype(&SDL_free)> windows(SDL_GetWindows(&count), SDL_free);
        Require(windows != nullptr, "SDL_GetWindows failed while inspecting the real production window");
        for (int index = 0; index < count; ++index) {
            auto* window = windows.get()[index];
            const auto properties = SDL_GetWindowProperties(window);
            if (properties && SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr) == hwnd)
                return window;
        }
        throw std::runtime_error("The production HWND was not associated with an actual SDL window");
    }

    void CheckBenchmarkWindow(HWND hwnd, Report& report) {
        const auto flags = SDL_GetWindowFlags(FindSdlWindow(hwnd));
        const auto style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        const auto extended = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        report.Number("benchmark_sdl_window_flags", flags);
        report.Number("benchmark_win32_style", static_cast<std::uint32_t>(style));
        report.Number("benchmark_win32_extended_style", static_cast<std::uint32_t>(extended));
        report.Boolean("benchmark_requested_borderless", true);
        report.Boolean("benchmark_requested_exclusive", false);
        report.Boolean("benchmark_requested_topmost", false);
        Require((flags & SDL_WINDOW_BORDERLESS) &&
            !(flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_RESIZABLE)) &&
            !(extended & WS_EX_TOPMOST), "Benchmark SDL borderless/nonexclusive/fixed/non-topmost policy differs");
        RECT window_bounds{}, client_bounds{};
        POINT client_origin{};
        Require(GetWindowRect(hwnd, &window_bounds) && GetClientRect(hwnd, &client_bounds) && ClientToScreen(hwnd, &client_origin),
            "Benchmark native window/client bounds query failed");
        const auto window_width = window_bounds.right - window_bounds.left;
        const auto window_height = window_bounds.bottom - window_bounds.top;
        const auto client_width = client_bounds.right - client_bounds.left;
        const auto client_height = client_bounds.bottom - client_bounds.top;
        report.Number("benchmark_native_window_width", window_width);
        report.Number("benchmark_native_window_height", window_height);
        report.Number("benchmark_native_client_width", client_width);
        report.Number("benchmark_native_client_height", client_height);
        report.Number("benchmark_native_client_offset_x", client_origin.x - window_bounds.left);
        report.Number("benchmark_native_client_offset_y", client_origin.y - window_bounds.top);
        // SDL retains WS_CAPTION for native windowed/taskbar behavior but removes its chrome in WM_NCCALCSIZE.
        Require(window_width == client_width && window_height == client_height &&
            client_origin.x == window_bounds.left && client_origin.y == window_bounds.top,
            "Benchmark actual native client rectangle has non-client border or title chrome");
        Require(GetPropW(hwnd, L"NonRudeHWND") != nullptr, "Production Benchmark NonRudeHWND is absent");
        report.Boolean("benchmark_actual_borderless_client", true);
        report.Boolean("benchmark_window_policy_valid", true);
    }

    void ProbeLifecycle(Report& report) {
        Window::Free();
        Window::Free();
        SDL_SetMainReady();
        Require(SDL_InitSubSystem(SDL_INIT_VIDEO), "External SDL video owner could not initialize");
        struct ExternalOwner {
            bool active{true};
            ~ExternalOwner() { if (active) SDL_QuitSubSystem(SDL_INIT_VIDEO); }
        } owner;
        for (int cycle = 0; cycle < 3; ++cycle) {
            Runtime runtime;
            Window::Init();
            std::unique_ptr<Window> window(Window::Create("Production SDL owner lifecycle probe", 320, 240,
                false, true, WindowMode::Native));
            const auto hwnd = NativeBridge::GetHandle(*window);
            Require(hwnd && IsWindow(hwnd), "Lifecycle Native window has no valid HWND");
            window->Destroy();
            window->Destroy();
            Require(!IsWindow(hwnd), "Destroy did not destroy the lifecycle HWND");
            window.reset();
            Window::Free();
            runtime.active = false;
            Window::Free();
            Require((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0,
                "Platform Free consumed another SDL video owner's reference");
        }
        report.Number("lifecycle_cycles", 3);
        report.Boolean("external_sdl_owner_preserved", true);
        report.Boolean("init_repeated", true);
        report.Boolean("free_repeated", true);
        report.Boolean("destroy_repeated", true);
        report.Boolean("window_destroyed", true);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        owner.active = false;
        Require((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0,
            "Repeated platform Init left an extra video reference after all owners released");
        report.Boolean("sdl_shutdown_completed", true);
    }

    void Probe(const Options& options, Report& report) {
        report.String("scope", "real production platform libraries; not physical input, DPI matrix, gameplay or GPU backend acceptance");
        report.Number("sdl_header_version", SDL_VERSION);
        report.Number("sdl_runtime_version", SDL_GetVersion());
        report.String("sdl_revision", SDL_GetRevision());
        report.String("fault", options.fault);
        report.Number("gl_presented_frames", 0);
        if (options.fault == "create-window" || options.fault == "create-context" || options.fault == "make-current")
            ArmFailure(options.fault);
        if (options.mode == "lifecycle") { ProbeLifecycle(report); return; }
        Runtime runtime;
        const auto mode = options.mode == "gl" ? WindowMode::OpenGL :
            options.mode == "native" ? WindowMode::Native : WindowMode::Vulkan;
        std::unique_ptr<Window> window(Window::Create("SymoCraft production SDL3 platform probe",
            options.benchmark ? 1920 : 640, options.benchmark ? 1080 : 480,
            options.benchmark, options.allow_unfocused, mode));
        Require(window != nullptr, "Window::Create published null instead of a diagnostic failure");
        const auto hwnd = NativeBridge::GetHandle(*window);
        Require(hwnd && IsWindow(hwnd), "Production NativeBridge returned no valid HWND");
        report.Boolean("native_hwnd_valid", true);
        Reject([] { Window::Free(); }, report, "free_live_window");
        Require(IsWindow(hwnd), "Rejected Free destroyed the live platform window");
        Reject([] {
            std::unique_ptr<Window> second(Window::Create("Rejected second platform window", 320, 240,
                false, true, WindowMode::Native));
        }, report, "second_platform_window");
        report.String("video_driver", SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "unknown");
        Require(window->width > 0 && window->height > 0, "Production window has no drawable pixels");
        report.Number("pixel_width", window->width);
        report.Number("pixel_height", window->height);
        if (options.benchmark) {
            Require(window->width == 1920 && window->height == 1080, "Benchmark production pixels are not 1920x1080");
            CheckBenchmarkWindow(hwnd, report);
        }
        if (mode == WindowMode::OpenGL) PrepareGl(*window, report);
        else RejectNonGl(*window, report);
        if (options.fault == "set-vsync") {
            ArmFailure(options.fault);
            GraphicsBridge::SetVsync(true);
            throw std::runtime_error("Injected VSync failure was swallowed by production");
        }
        if (options.fault == "query-extension") {
            ArmFailure(options.fault);
            GraphicsBridge::ExtensionSupported("GL_ARB_debug_output");
            throw std::runtime_error("Injected extension-query error was treated as legitimate unsupported capability");
        }
        if (options.fault == "get-procedure") {
            ArmFailure(options.fault);
            Require(gladLoadGLLoader(GlProcedure) != 0,
                "GLAD required procedure load failed via the production bridge (restricted get-procedure fixture)");
            throw std::runtime_error("Injected missing required GL procedure was swallowed by GLAD");
        }
        if (options.fault == "present" || options.fault == "surface") ArmFailure(options.fault);
#ifdef SYMOCRAFT_PLATFORM_PROBE_VULKAN
        VulkanResources vulkan(*window);
        if (mode == WindowMode::Vulkan) vulkan.Create(report);
        else {
            Reject([&] { VulkanBridge::GetInstanceProcedureAddress(*window); }, report, "non_vulkan_loader");
            Reject([&] { VulkanBridge::GetRequiredExtensions(*window); }, report, "non_vulkan_extensions");
        }
#endif
        int presented{};
        const auto start = Window::Time();
        for (int index = 0; index < options.frames && !window->ShouldClose(); ++index) {
            Require(Window::Time() - start < 10.0, "Production platform probe exceeded its frame budget");
            window->PollInt();
            window->CaptureInput();
            if (mode == WindowMode::OpenGL) {
                glViewport(0, 0, window->width, window->height);
                glClearColor(0.12f, 0.48f, 0.27f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                std::array<unsigned char, 4> pixel{};
                glReadPixels(window->width / 2, window->height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
                Require(glGetError() == GL_NO_ERROR, "Actual GL clear/readback reported an error");
                Require(pixel[0] > 0 && pixel[1] > pixel[0] && pixel[1] > pixel[2], "Actual GL readback is blank or differs from submitted color");
                report.Number("readback_red", static_cast<int>(pixel[0]));
                report.Number("readback_green", static_cast<int>(pixel[1]));
                report.Number("readback_blue", static_cast<int>(pixel[2]));
                GraphicsBridge::Present(*window);
                ++presented;
            }
            window->WaitEvents(0.005);
        }
        report.Number("gl_presented_frames", presented);
        if (mode == WindowMode::OpenGL) Require(presented > 0, "Production GL window closed before a real Present");
        report.Number("elapsed_seconds", Window::Time() - start);
        report.Boolean("focused_at_end", window->Focused());
        if (options.wait_close) WaitClose(*window, hwnd, report);
#ifdef SYMOCRAFT_PLATFORM_PROBE_VULKAN
        vulkan.Close(&report);
#endif
        if (options.fault == "destroy-context-after-release") ArmFailure(options.fault);
        window->Destroy();
        window->Destroy();
        Require(!IsWindow(hwnd), "Production Destroy left a live HWND");
        report.Boolean("window_destroyed", true);
        report.Boolean("destroy_repeated", true);
        Reject([&] { NativeBridge::GetHandle(*window); }, report, "destroyed_native_bridge");
        Reject([&] { GraphicsBridge::MakeCurrent(*window); }, report, "destroyed_make_current");
        Reject([&] { GraphicsBridge::Present(*window); }, report, "destroyed_present");
#ifdef SYMOCRAFT_PLATFORM_PROBE_VULKAN
        Reject([&] { VulkanBridge::GetInstanceProcedureAddress(*window); }, report, "destroyed_vulkan_bridge");
#endif
        Require(!GraphicsBridge::HasCurrentContext(), "Destroyed GL context remained current");
        window.reset();
        runtime.Close(report);
    }
}

int main(int argc, char** argv) {
    Report report;
    int exit_code{};
    // Keep production logger output as evidence on stderr and stdout as one JSON document.
    const int saved_stdout = _dup(_fileno(stdout));
    if (saved_stdout >= 0) _dup2(_fileno(stderr), _fileno(stdout));
    try {
        const auto options = Parse(argc, argv);
        report.String("mode", options.mode);
        Probe(options, report);
        report.String("status", "pass");
    } catch (const std::exception& error) {
        report.String("status", "fail");
        report.String("error", error.what());
        report.Boolean("video_active_after_failure", (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0);
        std::cerr << "Production SDL3 platform probe failed: " << error.what() << '\n';
        exit_code = 1;
    }
    AddFailureTrace(report);
    std::fflush(stdout);
    if (saved_stdout >= 0) {
        _dup2(saved_stdout, _fileno(stdout));
        _close(saved_stdout);
    }
    std::cout << report.Json();
    return exit_code;
}
