#define SDL_MAIN_HANDLED
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad/glad.h>
#include <Windows.h>
#ifdef SYMOCRAFT_SDL3_PROBE_VULKAN
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#include <SDL3/SDL_vulkan.h>
#endif
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
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

    struct Options {
        std::string mode{"gl"}, output;
        int frames{8}, width{640}, height{480};
        bool relative{}, benchmark{}, allow_unfocused{}, test_wait_close{};
    };

    int Integer(const char* text, int minimum, int maximum) {
        std::size_t consumed{};
        const std::string input{text};
        const int value = std::stoi(input, &consumed);
        if (consumed != input.size() || value < minimum || value > maximum)
            throw std::runtime_error("Argument is outside the supported integer range: " + input);
        return value;
    }

    Options Parse(int argc, char** argv) {
        Options result;
        for (int index = 1; index < argc; ++index) {
            const std::string argument{argv[index]};
            if (argument == "--relative") { result.relative = true; continue; }
            if (argument == "--benchmark") { result.benchmark = true; continue; }
            if (argument == "--allow-unfocused") { result.allow_unfocused = true; continue; }
            if (argument == "--test-wait-close") { result.test_wait_close = true; continue; }
            if (index + 1 >= argc) throw std::runtime_error("Missing value for " + argument);
            const char* value = argv[++index];
            if (argument == "--mode") result.mode = value;
            else if (argument == "--output") result.output = value;
            else if (argument == "--frames") result.frames = Integer(value, 1, 120);
            else if (argument == "--width") result.width = Integer(value, 64, 3840);
            else if (argument == "--height") result.height = Integer(value, 64, 2160);
            else throw std::runtime_error("Unknown argument: " + argument);
        }
        if (result.mode != "gl" && result.mode != "native" && result.mode != "vulkan")
            throw std::runtime_error("--mode must be gl, native, or vulkan");
#ifndef SYMOCRAFT_SDL3_PROBE_VULKAN
        if (result.mode == "vulkan") throw std::runtime_error("Vulkan mode was not compiled into this probe");
#endif
        return result;
    }

    void Require(bool successful, const char* operation) {
        if (!successful) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
    }

    struct Runtime {
        SDL_Window* window{};
        SDL_GLContext context{};
        bool initialized{};
        HWND non_rude_window{};
        Runtime() {
            SDL_SetMainReady();
            if (!SDL_Init(SDL_INIT_VIDEO)) {
                const auto message = std::string("SDL_Init(SDL_INIT_VIDEO): ") + SDL_GetError();
                SDL_Quit();
                throw std::runtime_error(message);
            }
            initialized = true;
        }
        ~Runtime() {
            if (context && !SDL_GL_DestroyContext(context))
                std::cerr << "cleanup: SDL_GL_DestroyContext: " << SDL_GetError() << '\n';
            if (non_rude_window) RemovePropW(non_rude_window, L"NonRudeHWND");
            if (window) SDL_DestroyWindow(window);
            if (initialized) SDL_Quit();
        }
        void Close(Report& report) {
            if (context) {
                Require(SDL_GL_DestroyContext(context), "SDL_GL_DestroyContext");
                context = nullptr;
                report.Boolean("gl_context_destroyed", true);
            }
            if (window) {
                if (non_rude_window) {
                    if (!RemovePropW(non_rude_window, L"NonRudeHWND"))
                        throw std::runtime_error("RemovePropW(NonRudeHWND) failed");
                    non_rude_window = nullptr;
                    report.Boolean("benchmark_property_removed", true);
                }
                SDL_DestroyWindow(window);
                window = nullptr;
                report.Boolean("window_destroyed", true);
            }
            if (initialized) {
                SDL_Quit();
                initialized = false;
                report.Boolean("sdl_shutdown_completed", true);
            }
        }
        Runtime(const Runtime&) = delete;
        Runtime& operator=(const Runtime&) = delete;
    };

    void* GlProcedure(const char* name) {
        return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name));
    }

    void PrepareGl(Runtime& runtime, Report& report) {
        runtime.context = SDL_GL_CreateContext(runtime.window);
        Require(runtime.context != nullptr, "SDL_GL_CreateContext");
        Require(SDL_GL_MakeCurrent(runtime.window, runtime.context), "SDL_GL_MakeCurrent");
        if (!gladLoadGLLoader(GlProcedure)) throw std::runtime_error("GLAD: failed to load OpenGL procedures");
        if (!GLAD_GL_VERSION_4_6) throw std::runtime_error("OpenGL: actual context does not provide version 4.6");
        const auto gl_text = [](GLenum name) {
            const auto* value = glGetString(name);
            return value ? std::string(reinterpret_cast<const char*>(value)) : std::string{};
        };
        report.String("gl_vendor", gl_text(GL_VENDOR));
        report.String("gl_renderer", gl_text(GL_RENDERER));
        report.String("gl_version", gl_text(GL_VERSION));
        int profile{}, buffers{}, samples{}, flags{};
        Require(SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile), "SDL_GL_GetAttribute(profile)");
        Require(SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers), "SDL_GL_GetAttribute(sample_buffers)");
        Require(SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &samples), "SDL_GL_GetAttribute(samples)");
        Require(SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &flags), "SDL_GL_GetAttribute(context_flags)");
        report.Number("gl_profile", profile);
        report.Number("gl_samples", samples);
        report.Number("gl_sample_buffers", buffers);
        report.Number("gl_context_flags", flags);
        if (profile != SDL_GL_CONTEXT_PROFILE_CORE || buffers != 1 || samples != 4)
            throw std::runtime_error("OpenGL: actual Core/4x MSAA attributes do not match the requested specification");
        int actual_profile{}, actual_flags{}, actual_samples{};
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &actual_profile);
        glGetIntegerv(GL_CONTEXT_FLAGS, &actual_flags);
        glGetIntegerv(GL_SAMPLES, &actual_samples);
        report.Number("gl_driver_profile", actual_profile);
        report.Number("gl_driver_context_flags", actual_flags);
        report.Number("gl_driver_samples", actual_samples);
        if (!(actual_profile & GL_CONTEXT_CORE_PROFILE_BIT) || actual_samples != 4)
            throw std::runtime_error("OpenGL: driver-reported Core/4x MSAA attributes do not match the request");
#ifndef NDEBUG
        if (!(actual_flags & GL_CONTEXT_FLAG_DEBUG_BIT))
            throw std::runtime_error("OpenGL: the Debug probe did not receive a debug context");
#endif
        int interval{};
        Require(SDL_GL_SetSwapInterval(1), "SDL_GL_SetSwapInterval(1)");
        Require(SDL_GL_GetSwapInterval(&interval), "SDL_GL_GetSwapInterval(vsync_on)");
        report.Number("vsync_on_actual", interval);
        Require(SDL_GL_SetSwapInterval(0), "SDL_GL_SetSwapInterval(0)");
        Require(SDL_GL_GetSwapInterval(&interval), "SDL_GL_GetSwapInterval(vsync_off)");
        report.Number("vsync_off_actual", interval);
    }

#ifdef SYMOCRAFT_SDL3_PROBE_VULKAN
    void RequireVk(VkResult result, const char* operation) {
        if (result != VK_SUCCESS)
            throw std::runtime_error(std::string(operation) + ": VkResult=" + std::to_string(result));
    }

    struct VulkanResources {
        VkInstance instance{};
        VkSurfaceKHR surface{};
        PFN_vkGetInstanceProcAddr get{};
        PFN_vkDestroyInstance destroy{};
        ~VulkanResources() {
            if (surface && instance) SDL_Vulkan_DestroySurface(instance, surface, nullptr);
            if (instance && destroy) destroy(instance, nullptr);
            if (instance && !destroy) std::cerr << "cleanup: missing vkDestroyInstance for an acquired instance\n";
        }
        void Close(Report& report) {
            if (surface && instance) {
                SDL_Vulkan_DestroySurface(instance, surface, nullptr);
                surface = VK_NULL_HANDLE;
                report.Boolean("vulkan_surface_destroyed", true);
            }
            if (instance && destroy) {
                destroy(instance, nullptr);
                instance = VK_NULL_HANDLE;
                report.Boolean("vulkan_instance_destroyed", true);
            }
        }
        void Create(SDL_Window* window, Report& report) {
            get = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
            Require(get != nullptr, "SDL_Vulkan_GetVkGetInstanceProcAddr");
            const auto version_function = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(get(nullptr, "vkEnumerateInstanceVersion"));
            std::uint32_t supported_version = VK_API_VERSION_1_0;
            if (version_function) RequireVk(version_function(&supported_version), "vkEnumerateInstanceVersion");
            report.Number("vulkan_loader_api_version", supported_version);
            report.Number("vulkan_header_version", VK_HEADER_VERSION);
            if (supported_version < VK_API_VERSION_1_2)
                throw std::runtime_error("Vulkan loader does not provide the candidate Vulkan 1.2 instance baseline");
            const auto create = reinterpret_cast<PFN_vkCreateInstance>(get(nullptr, "vkCreateInstance"));
            if (!create) throw std::runtime_error("SDL Vulkan loader: missing vkCreateInstance");
            Uint32 count{};
            const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);
            Require(extensions != nullptr && count != 0, "SDL_Vulkan_GetInstanceExtensions");
            std::ostringstream names;
            for (Uint32 index = 0; index < count; ++index) {
                if (index) names << ',';
                names << extensions[index];
            }
            report.String("vulkan_required_extensions", names.str());
            VkApplicationInfo application{};
            application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            application.pApplicationName = "SymoCraft SDL3 preparation probe";
            application.apiVersion = VK_API_VERSION_1_2;
            VkInstanceCreateInfo settings{};
            settings.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            settings.pApplicationInfo = &application;
            settings.enabledExtensionCount = count;
            settings.ppEnabledExtensionNames = extensions;
            RequireVk(create(&settings, nullptr, &instance), "vkCreateInstance");
            destroy = reinterpret_cast<PFN_vkDestroyInstance>(get(instance, "vkDestroyInstance"));
            if (!destroy) throw std::runtime_error("SDL Vulkan loader: missing vkDestroyInstance");
            Require(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface), "SDL_Vulkan_CreateSurface");
            report.Boolean("vulkan_instance_created", true);
            report.Boolean("vulkan_surface_created", true);
            report.String("vulkan_loader_source", "SDL_Vulkan_GetVkGetInstanceProcAddr; window-managed load/unload");
            const auto enumerate = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(get(instance, "vkEnumeratePhysicalDevices"));
            if (!enumerate) throw std::runtime_error("SDL Vulkan loader: missing vkEnumeratePhysicalDevices");
            std::uint32_t devices{};
            RequireVk(enumerate(instance, &devices, nullptr), "vkEnumeratePhysicalDevices");
            report.Number("vulkan_physical_device_count", devices);
        }
    };
#endif

    struct EventObservations {
        int total{}, keys_down{}, keys_up{}, motions{}, wheels{}, focus_losses{};
        double dx{}, dy{}, scroll{};
        bool close_requested{};
        void Consume(const SDL_Event& event, SDL_WindowID id) {
            ++total;
            if (event.type == SDL_EVENT_QUIT) { close_requested = true; return; }
            if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
                if (event.window.windowID != id) return;
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) close_requested = true;
                if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) ++focus_losses;
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                if (event.key.windowID != id) return;
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    ++keys_down;
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE) close_requested = true;
                } else ++keys_up;
            } else if (event.type == SDL_EVENT_MOUSE_MOTION && event.motion.windowID == id) {
                ++motions; dx += event.motion.xrel; dy -= event.motion.yrel;
            } else if (event.type == SDL_EVENT_MOUSE_WHEEL && event.wheel.windowID == id) {
                ++wheels;
                scroll += event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y;
            }
        }
        void AddReport(Report& report) const {
            report.Number("events_observed", total);
            report.Number("key_down_events", keys_down);
            report.Number("key_up_events", keys_up);
            report.Number("motion_events", motions);
            report.Number("wheel_events", wheels);
            report.Number("focus_loss_events", focus_losses);
            report.Number("observed_dx", dx);
            report.Number("observed_dy", dy);
            report.Number("observed_scroll_y", scroll);
            report.Boolean("close_requested", close_requested);
        }
    };

    void Probe(const Options& options, Report& report) {
        Runtime runtime;
#ifdef SYMOCRAFT_SDL3_PROBE_VULKAN
        VulkanResources vulkan;
#endif
        report.Number("sdl_header_version", SDL_VERSION);
        report.Number("sdl_runtime_version", SDL_GetVersion());
        report.String("sdl_revision", SDL_GetRevision());
        report.String("video_driver", SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "unknown");
        report.Boolean("linked_sdl_static", true);
        report.String("scope", "independent library/window probe; not production platform or three-backend game acceptance");
        SDL_WindowFlags flags = SDL_WINDOW_HIDDEN;
        flags |= options.benchmark ? SDL_WINDOW_BORDERLESS : SDL_WINDOW_RESIZABLE;
        Require(SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, options.allow_unfocused ? "0" : "1"),
            "SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN)");
        if (options.mode == "gl") {
            flags |= SDL_WINDOW_OPENGL;
            SDL_GL_ResetAttributes();
            Require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4), "SDL_GL_SetAttribute(major)");
            Require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6), "SDL_GL_SetAttribute(minor)");
            Require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE), "SDL_GL_SetAttribute(core)");
            Require(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "SDL_GL_SetAttribute(doublebuffer)");
            Require(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24), "SDL_GL_SetAttribute(depth)");
            Require(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1), "SDL_GL_SetAttribute(sample_buffers)");
            Require(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4), "SDL_GL_SetAttribute(samples)");
#ifndef NDEBUG
            Require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG), "SDL_GL_SetAttribute(debug)");
#endif
        } else if (options.mode == "vulkan") flags |= SDL_WINDOW_VULKAN;
        runtime.window = SDL_CreateWindow("SymoCraft SDL3 preparation probe", options.width, options.height, flags);
        Require(runtime.window != nullptr, "SDL_CreateWindow");
        const SDL_DisplayID primary = SDL_GetPrimaryDisplay();
        Require(primary != 0, "SDL_GetPrimaryDisplay");
        Require(SDL_SetWindowPosition(runtime.window, static_cast<int>(SDL_WINDOWPOS_CENTERED_DISPLAY(primary)),
            static_cast<int>(SDL_WINDOWPOS_CENTERED_DISPLAY(primary))), "SDL_SetWindowPosition(centered)");
        if (options.mode == "gl") PrepareGl(runtime, report);
#ifdef SYMOCRAFT_SDL3_PROBE_VULKAN
        if (options.mode == "vulkan") vulkan.Create(runtime.window, report);
#endif
        const auto properties = SDL_GetWindowProperties(runtime.window);
        Require(properties != 0, "SDL_GetWindowProperties");
        const auto hwnd = static_cast<HWND>(SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
        if (!hwnd || !IsWindow(hwnd)) throw std::runtime_error("Native HWND: missing or invalid Win32 window");
        report.Boolean("native_hwnd_valid", true);
        if (options.benchmark) {
            if (!SetPropW(hwnd, L"NonRudeHWND", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(1))))
                throw std::runtime_error("SetPropW(NonRudeHWND): Win32 error=" + std::to_string(GetLastError()));
            runtime.non_rude_window = hwnd;
            report.Boolean("benchmark_non_rude_hwnd", GetPropW(hwnd, L"NonRudeHWND") != nullptr);
            const auto actual_flags = SDL_GetWindowFlags(runtime.window);
            const bool valid = (actual_flags & SDL_WINDOW_BORDERLESS) &&
                !(actual_flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_RESIZABLE));
            report.Boolean("benchmark_window_flags_valid", valid);
            if (!valid) throw std::runtime_error("Benchmark window flags do not match borderless/nonexclusive/non-topmost/fixed policy");
        }
        Require(SDL_ShowWindow(runtime.window), "SDL_ShowWindow");
        if (options.relative) {
            Require(SDL_SetWindowRelativeMouseMode(runtime.window, true), "SDL_SetWindowRelativeMouseMode(true)");
            report.Boolean("relative_mode_requested", SDL_GetWindowRelativeMouseMode(runtime.window));
            report.String("relative_system_scale_hint", SDL_GetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE) ?
                SDL_GetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE) : "default-unscaled");
        }
        int width{}, height{}, pixel_width{}, pixel_height{};
        Require(SDL_GetWindowSize(runtime.window, &width, &height), "SDL_GetWindowSize");
        Require(SDL_GetWindowSizeInPixels(runtime.window, &pixel_width, &pixel_height), "SDL_GetWindowSizeInPixels");
        report.Number("window_width", width); report.Number("window_height", height);
        report.Number("pixel_width", pixel_width); report.Number("pixel_height", pixel_height);
        report.Number("window_pixel_density", SDL_GetWindowPixelDensity(runtime.window));
        report.Number("window_display_scale", SDL_GetWindowDisplayScale(runtime.window));
        if (pixel_width <= 0 || pixel_height <= 0) throw std::runtime_error("Window has no drawable pixels");
        if (options.benchmark && (pixel_width != options.width || pixel_height != options.height))
            throw std::runtime_error("Benchmark drawable pixels do not match the requested workload dimensions");
        report.Boolean("allow_unfocused_requested", options.allow_unfocused);
        const auto window_id = SDL_GetWindowID(runtime.window);
        EventObservations events;
        int rendered{};
        const Uint64 start = SDL_GetTicksNS();
        for (int index = 0; index < options.frames && !events.close_requested; ++index) {
            if (SDL_GetTicksNS() - start > 10000000000ull)
                throw std::runtime_error("Probe exceeded its ten-second event/frame budget");
            SDL_Event event{};
            if (SDL_WaitEventTimeout(&event, 10)) events.Consume(event, window_id);
            int consumed{};
            while (SDL_PollEvent(&event)) {
                if (++consumed > 8192) throw std::runtime_error("Event queue exceeded the bounded per-frame drain budget");
                events.Consume(event, window_id);
            }
            if (events.close_requested) break;
            if (options.mode == "gl") {
                glViewport(0, 0, pixel_width, pixel_height);
                glClearColor(0.12f, 0.48f, 0.27f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                std::array<unsigned char, 4> pixel{};
                glReadPixels(pixel_width / 2, pixel_height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
                if (glGetError() != GL_NO_ERROR) throw std::runtime_error("OpenGL clear/readback reported a GL error");
                if (!pixel[0] && !pixel[1] && !pixel[2]) throw std::runtime_error("OpenGL clear/readback returned a blank pixel");
                report.Number("readback_red", static_cast<int>(pixel[0]));
                report.Number("readback_green", static_cast<int>(pixel[1]));
                report.Number("readback_blue", static_cast<int>(pixel[2]));
                Require(SDL_GL_SwapWindow(runtime.window), "SDL_GL_SwapWindow");
                ++rendered;
            }
            SDL_Delay(8);
        }
        report.Number("elapsed_ms", static_cast<double>(SDL_GetTicksNS() - start) / 1000000.0);
        report.Number("gl_presented_frames", rendered);
        if (options.test_wait_close) {
            if (!PostMessageW(hwnd, WM_CLOSE, 0, 0)) throw std::runtime_error("PostMessageW(WM_CLOSE) failed");
            const auto deadline = SDL_GetTicksNS() + 2000000000ull;
            while (!events.close_requested && SDL_GetTicksNS() < deadline) {
                SDL_Event event{};
                if (SDL_WaitEventTimeout(&event, 20)) events.Consume(event, window_id);
                while (SDL_PollEvent(&event)) events.Consume(event, window_id);
            }
            report.Boolean("wait_close_received", events.close_requested);
            if (!events.close_requested) throw std::runtime_error("WM_CLOSE was not preserved by SDL wait/poll event consumption");
        }
        report.Boolean("focused_at_end", (SDL_GetWindowFlags(runtime.window) & SDL_WINDOW_INPUT_FOCUS) != 0);
        events.AddReport(report);
        if (options.mode == "gl" && rendered == 0) throw std::runtime_error("OpenGL probe closed before presenting a frame");
        if (options.relative)
            Require(SDL_SetWindowRelativeMouseMode(runtime.window, false), "SDL_SetWindowRelativeMouseMode(false)");
#ifdef SYMOCRAFT_SDL3_PROBE_VULKAN
        vulkan.Close(report);
#endif
        runtime.Close(report);
    }
}

int main(int argc, char** argv) {
    Report report;
    Options options;
    int exit_code{};
    try {
        options = Parse(argc, argv);
        report.String("mode", options.mode);
        report.Number("requested_frames", options.frames);
        Probe(options, report);
        report.String("status", "pass");
    } catch (const std::exception& error) {
        report.String("status", "fail");
        report.String("error", error.what());
        std::cerr << "SDL3 preparation probe failed: " << error.what() << '\n';
        exit_code = 1;
    }
    const auto json = report.Json();
    std::cout << json;
    if (!options.output.empty()) {
        std::ofstream output(std::filesystem::path(std::u8string(options.output.begin(), options.output.end())), std::ios::binary);
        output << json;
        if (!output) {
            std::cerr << "Cannot write JSON report: " << options.output << '\n';
            exit_code = 1;
        }
    }
    return exit_code;
}
