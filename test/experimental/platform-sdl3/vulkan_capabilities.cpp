#include <symocraft/platform/window.h>
#include <vulkan_bridge.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using SymoCraft::Window;
using SymoCraft::WindowMode;
using SymoCraft::Platform::VulkanBridge;

void Require(bool condition, const char* operation) {
    if (!condition) throw std::runtime_error(operation);
}

void Check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + ": VkResult=" + std::to_string(result));
}

template<class T, class Function>
std::vector<T> Enumerate(Function function, const char* operation) {
    for (unsigned attempt = 0; attempt < 3; ++attempt) {
        std::uint32_t count{};
        Check(function(&count, static_cast<T*>(nullptr)), operation);
        std::vector<T> values(count);
        const auto result = function(&count, values.data());
        if (result == VK_INCOMPLETE) continue;
        Check(result, operation);
        values.resize(count);
        return values;
    }
    throw std::runtime_error(std::string(operation) + ": enumeration kept changing");
}

void HexBytes(const char* label, const std::uint8_t* bytes, std::size_t count) {
    std::cout << label << '=' << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < count; ++i) std::cout << std::setw(2) << static_cast<unsigned>(bytes[i]);
    std::cout << std::dec << std::setfill(' ') << '\n';
}

struct Diagnostics {
    std::atomic<unsigned> warnings{}, errors{};
};

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user) {
    auto& diagnostics = *static_cast<Diagnostics*>(user);
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ++diagnostics.warnings;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) ++diagnostics.errors;
    std::fprintf(stderr, "[vulkan validation] severity=0x%x types=0x%x id=%s message=%s\n",
        static_cast<unsigned>(severity), static_cast<unsigned>(types),
        data && data->pMessageIdName ? data->pMessageIdName : "missing",
        data && data->pMessage ? data->pMessage : "missing");
    return VK_FALSE;
}

struct Resources {
    Diagnostics diagnostics;
    std::unique_ptr<Window> window;
    VkInstance instance{};
    VkDebugUtilsMessengerEXT messenger{};
    VkSurfaceKHR surface{};
    VkDevice device{};
    VkSwapchainKHR swapchain{};
    PFN_vkGetInstanceProcAddr get{};
    PFN_vkDestroyInstance destroy_instance{};
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_messenger{};
    PFN_vkDestroyDevice destroy_device{};
    PFN_vkDestroySwapchainKHR destroy_swapchain{};

    template<class T> T InstanceFunction(const char* name) const {
        const auto value = reinterpret_cast<T>(get(instance, name));
        if (!value) throw std::runtime_error(std::string("missing Vulkan procedure: ") + name);
        return value;
    }

    void Close() {
        // No work is submitted: destroying these unused objects requires no device drain.
        if (swapchain) {
            Require(destroy_swapchain != nullptr, "cannot clean swapchain without its destroy procedure");
            destroy_swapchain(device, swapchain, nullptr); swapchain = {};
        }
        if (device) {
            Require(destroy_device != nullptr, "cannot clean device without its destroy procedure");
            destroy_device(device, nullptr); device = {};
        }
        if (surface) { VulkanBridge::DestroySurface(*window, instance, surface); surface = {}; }
        if (messenger) {
            Require(destroy_messenger != nullptr, "cannot clean messenger without its destroy procedure");
            destroy_messenger(instance, messenger, nullptr); messenger = {};
        }
        if (instance) {
            Require(destroy_instance != nullptr, "cannot clean instance without its destroy procedure");
            destroy_instance(instance, nullptr); instance = {};
        }
        if (window) { window->Destroy(); window.reset(); }
        Window::Free();
    }

    ~Resources() noexcept {
        try { Close(); }
        catch (const std::exception& error) {
            std::fprintf(stderr, "probe cleanup: %s; retaining the window and SDL loader until process exit\n", error.what());
            (void)window.release();
        }
    }
};

VkImageFormatProperties ImageProperties(PFN_vkGetPhysicalDeviceImageFormatProperties query,
    VkPhysicalDevice physical, VkFormat format, VkImageUsageFlags usage, const char* label) {
    VkImageFormatProperties properties{};
    Check(query(physical, format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &properties), label);
    std::cout << label << ": format=" << format << "; samples=" << properties.sampleCounts
              << "; max_extent=" << properties.maxExtent.width << 'x' << properties.maxExtent.height
              << "; max_layers=" << properties.maxArrayLayers << '\n';
    return properties;
}

void Probe(Resources& state) {
    Window::Init();
    state.window.reset(Window::Create("SymoCraft Vulkan capability probe", 640, 480, false, true, WindowMode::Vulkan));
    state.get = VulkanBridge::GetInstanceProcedureAddress(*state.window);
    const auto version = state.InstanceFunction<PFN_vkEnumerateInstanceVersion>("vkEnumerateInstanceVersion");
    std::uint32_t loader_version{};
    Check(version(&loader_version), "vkEnumerateInstanceVersion");
    std::cout << "loader_api_version=" << loader_version << '\n';
    Require(loader_version >= VK_API_VERSION_1_2, "Vulkan loader must support 1.2");
    const auto enum_extensions = state.InstanceFunction<PFN_vkEnumerateInstanceExtensionProperties>("vkEnumerateInstanceExtensionProperties");
    const auto extensions = Enumerate<VkExtensionProperties>([&](auto* count, auto* values) {
        return enum_extensions(nullptr, count, values);
    }, "vkEnumerateInstanceExtensionProperties");
    const auto enum_layers = state.InstanceFunction<PFN_vkEnumerateInstanceLayerProperties>("vkEnumerateInstanceLayerProperties");
    const auto layers = Enumerate<VkLayerProperties>(enum_layers, "vkEnumerateInstanceLayerProperties");
    constexpr const char* validation = "VK_LAYER_KHRONOS_validation";
    Require(std::any_of(layers.begin(), layers.end(), [&](const auto& layer) {
        return std::strcmp(layer.layerName, validation) == 0;
    }), "VK_LAYER_KHRONOS_validation is unavailable; validation is not disabled");
    auto required_extensions = VulkanBridge::GetRequiredExtensions(*state.window);
    required_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    std::vector<const char*> names;
    for (const auto& name : required_extensions) {
        Require(std::any_of(extensions.begin(), extensions.end(), [&](const auto& available) {
            return name == available.extensionName;
        }), "a required surface/debug-utils instance extension is missing");
        names.push_back(name.c_str());
        std::cout << "enabled_instance_extension=" << name << '\n';
    }
    VkDebugUtilsMessengerCreateInfoEXT debug{};
    debug.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug.pfnUserCallback = DebugMessage;
    debug.pUserData = &state.diagnostics;
    VkApplicationInfo application{};
    application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.pApplicationName = "SymoCraft Vulkan capabilities";
    application.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = &debug;
    instance_info.pApplicationInfo = &application;
    instance_info.enabledLayerCount = 1;
    instance_info.ppEnabledLayerNames = &validation;
    instance_info.enabledExtensionCount = static_cast<std::uint32_t>(names.size());
    instance_info.ppEnabledExtensionNames = names.data();
    Check(state.InstanceFunction<PFN_vkCreateInstance>("vkCreateInstance")(&instance_info, nullptr, &state.instance), "vkCreateInstance");
    state.destroy_instance = state.InstanceFunction<PFN_vkDestroyInstance>("vkDestroyInstance");
    state.destroy_messenger = state.InstanceFunction<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT");
    Check(state.InstanceFunction<PFN_vkCreateDebugUtilsMessengerEXT>("vkCreateDebugUtilsMessengerEXT")(
        state.instance, &debug, nullptr, &state.messenger), "vkCreateDebugUtilsMessengerEXT");
    state.surface = VulkanBridge::CreateSurface(*state.window, state.instance);

    const auto enumerate_devices = state.InstanceFunction<PFN_vkEnumeratePhysicalDevices>("vkEnumeratePhysicalDevices");
    const auto physicals = Enumerate<VkPhysicalDevice>([&](auto* count, auto* values) {
        return enumerate_devices(state.instance, count, values);
    }, "vkEnumeratePhysicalDevices");
    const auto get_properties = state.InstanceFunction<PFN_vkGetPhysicalDeviceProperties2>("vkGetPhysicalDeviceProperties2");
    VkPhysicalDevice physical{};
    VkPhysicalDeviceProperties properties{};
    for (const auto candidate : physicals) {
        VkPhysicalDeviceIDProperties identity{};
        identity.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        VkPhysicalDeviceDriverProperties driver{};
        driver.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
        identity.pNext = &driver;
        VkPhysicalDeviceProperties2 combined{};
        combined.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        combined.pNext = &identity;
        get_properties(candidate, &combined);
        const auto& value = combined.properties;
        std::cout << "physical_device=" << value.deviceName << "; vendor=" << value.vendorID
                  << "; device=" << value.deviceID << "; api=" << value.apiVersion << '\n';
        if (value.vendorID != 0x10de || value.deviceID != 0x2c05 ||
            std::string(value.deviceName).find("RTX 5070 Ti") == std::string::npos) continue;
        Require(!physical, "multiple matching RTX 5070 Ti devices require an explicit identity choice");
        Require(value.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, "selected adapter is not a discrete hardware GPU");
        Require(value.apiVersion >= VK_API_VERSION_1_2, "selected physical device does not support Vulkan 1.2");
        physical = candidate;
        properties = value;
        std::cout << "selected_driver_version_raw=" << value.driverVersion << "; driver_id=" << driver.driverID
                  << "; driver_name=" << driver.driverName << "; driver_info=" << driver.driverInfo << '\n';
        std::cout << "device_luid_valid=" << identity.deviceLUIDValid << "; node_mask=" << identity.deviceNodeMask << '\n';
        HexBytes("device_uuid", identity.deviceUUID, VK_UUID_SIZE);
        HexBytes("driver_uuid", identity.driverUUID, VK_UUID_SIZE);
        HexBytes("device_luid_bytes", identity.deviceLUID, VK_LUID_SIZE);
    }
    Require(physical != VK_NULL_HANDLE, "the required NVIDIA RTX 5070 Ti hardware adapter was not found");
    const auto enum_device_extensions = state.InstanceFunction<PFN_vkEnumerateDeviceExtensionProperties>("vkEnumerateDeviceExtensionProperties");
    const auto device_extensions = Enumerate<VkExtensionProperties>([&](auto* count, auto* values) {
        return enum_device_extensions(physical, nullptr, count, values);
    }, "vkEnumerateDeviceExtensionProperties");
    Require(std::any_of(device_extensions.begin(), device_extensions.end(), [](const auto& extension) {
        return std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
    }), "VK_KHR_swapchain is unavailable");
    const auto get_queues = state.InstanceFunction<PFN_vkGetPhysicalDeviceQueueFamilyProperties>("vkGetPhysicalDeviceQueueFamilyProperties");
    std::uint32_t queue_count{};
    get_queues(physical, &queue_count, nullptr);
    std::vector<VkQueueFamilyProperties> queues(queue_count);
    get_queues(physical, &queue_count, queues.data());
    queues.resize(queue_count);
    const auto get_support = state.InstanceFunction<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>("vkGetPhysicalDeviceSurfaceSupportKHR");
    std::uint32_t queue_index = std::numeric_limits<std::uint32_t>::max();
    for (std::uint32_t i = 0; i < queue_count; ++i) {
        VkBool32 present{};
        Check(get_support(physical, i, state.surface, &present), "vkGetPhysicalDeviceSurfaceSupportKHR");
        std::cout << "queue_family=" << i << "; flags=" << queues[i].queueFlags << "; count=" << queues[i].queueCount
                  << "; surface_present=" << present << "; timestamp_valid_bits=" << queues[i].timestampValidBits << '\n';
        if (queue_index == std::numeric_limits<std::uint32_t>::max() && queues[i].queueCount &&
            (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) queue_index = i;
    }
    Require(queue_index != std::numeric_limits<std::uint32_t>::max(), "probe requires one graphics queue family supporting this surface");
    std::cout << "selected_queue_family=" << queue_index << "; timestamp_period_ns=" << properties.limits.timestampPeriod
              << "; timestamp_compute_and_graphics=" << properties.limits.timestampComputeAndGraphics
              << "; timestamp_capability_only=" << (queues[queue_index].timestampValidBits != 0) << '\n';

    VkSurfaceCapabilitiesKHR caps{};
    Check(state.InstanceFunction<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>("vkGetPhysicalDeviceSurfaceCapabilitiesKHR")(
        physical, state.surface, &caps), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    std::cout << "surface_caps: min_images=" << caps.minImageCount << "; max_images=" << caps.maxImageCount
              << "; extent=" << caps.currentExtent.width << 'x' << caps.currentExtent.height
              << "; min_extent=" << caps.minImageExtent.width << 'x' << caps.minImageExtent.height
              << "; max_extent=" << caps.maxImageExtent.width << 'x' << caps.maxImageExtent.height
              << "; usage=" << caps.supportedUsageFlags << "; transforms=" << caps.supportedTransforms
              << "; current_transform=" << caps.currentTransform << "; composite_alpha=" << caps.supportedCompositeAlpha << '\n';
    const auto get_formats = state.InstanceFunction<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>("vkGetPhysicalDeviceSurfaceFormatsKHR");
    const auto formats = Enumerate<VkSurfaceFormatKHR>([&](auto* count, auto* values) {
        return get_formats(physical, state.surface, count, values);
    }, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    VkSurfaceFormatKHR chosen{};
    bool found_format = false;
    for (const auto& format : formats) {
        std::cout << "surface_format=" << format.format << "; color_space=" << format.colorSpace << '\n';
        if (!found_format && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
            (format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM ||
             format.format == VK_FORMAT_UNDEFINED)) {
            chosen = format;
            if (chosen.format == VK_FORMAT_UNDEFINED) chosen.format = VK_FORMAT_R8G8B8A8_UNORM;
            found_format = true;
        }
    }
    Require(found_format, "no RGBA8/BGRA8 UNORM surface format with SRGB_NONLINEAR color space");
    const auto get_modes = state.InstanceFunction<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>("vkGetPhysicalDeviceSurfacePresentModesKHR");
    const auto modes = Enumerate<VkPresentModeKHR>([&](auto* count, auto* values) {
        return get_modes(physical, state.surface, count, values);
    }, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    for (const auto mode : modes) std::cout << "surface_present_mode=" << mode << '\n';
    Require(std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_FIFO_KHR) != modes.end(), "FIFO present mode missing");
    const auto image_properties = state.InstanceFunction<PFN_vkGetPhysicalDeviceImageFormatProperties>("vkGetPhysicalDeviceImageFormatProperties");
    for (const auto format : {VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SRGB}) {
        const auto texture = ImageProperties(image_properties, physical, format,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, "sampled_texture_array");
        Require(texture.maxExtent.width >= 64 && texture.maxExtent.height >= 64 && texture.maxArrayLayers >= 64 &&
            (texture.sampleCounts & VK_SAMPLE_COUNT_1_BIT), "texture array does not support 64x64x64 single-sample storage");
    }
    const auto color = ImageProperties(image_properties, physical, chosen.format,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, "msaa_color_and_screenshot");
    const auto depth = ImageProperties(image_properties, physical, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, "msaa_depth");
    Require((color.sampleCounts & VK_SAMPLE_COUNT_4_BIT) && (depth.sampleCounts & VK_SAMPLE_COUNT_4_BIT),
        "the chosen color/depth formats do not both support 4x MSAA");
    constexpr VkImageUsageFlags swap_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    Require((caps.supportedUsageFlags & swap_usage) == swap_usage, "surface images cannot support color attachment plus screenshot transfer source");

    state.destroy_device = state.InstanceFunction<PFN_vkDestroyDevice>("vkDestroyDevice");
    const auto get_device = state.InstanceFunction<PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr");
    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = queue_index;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    constexpr const char* swap_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = &swap_extension;
    Check(state.InstanceFunction<PFN_vkCreateDevice>("vkCreateDevice")(physical, &device_info, nullptr, &state.device), "vkCreateDevice");
    const auto device_function = [&]<class T>(const char* name) {
        const auto value = reinterpret_cast<T>(get_device(state.device, name));
        if (!value) throw std::runtime_error(std::string("missing device procedure: ") + name);
        return value;
    };
    VkQueue queue{};
    device_function.operator()<PFN_vkGetDeviceQueue>("vkGetDeviceQueue")(state.device, queue_index, 0, &queue);
    Require(queue != VK_NULL_HANDLE, "vkGetDeviceQueue returned no actual queue");
    state.destroy_swapchain = device_function.operator()<PFN_vkDestroySwapchainKHR>("vkDestroySwapchainKHR");
    VkExtent2D extent = caps.currentExtent;
    if (extent.width == std::numeric_limits<std::uint32_t>::max()) {
        extent.width = std::clamp(static_cast<std::uint32_t>(state.window->width), caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(static_cast<std::uint32_t>(state.window->height), caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    Require(extent.width && extent.height, "surface extent is zero");
    Require(color.maxExtent.width >= extent.width && color.maxExtent.height >= extent.height &&
            depth.maxExtent.width >= extent.width && depth.maxExtent.height >= extent.height, "image format limits do not cover the surface extent");
    const auto image_count = caps.maxImageCount ? std::min(caps.minImageCount + 1, caps.maxImageCount) : caps.minImageCount + 1;
    VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    bool found_alpha = false;
    for (const auto value : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR}) {
        if (!found_alpha && (caps.supportedCompositeAlpha & value)) { alpha = value; found_alpha = true; }
    }
    Require(found_alpha, "surface supports no composite alpha mode");
    VkSwapchainCreateInfoKHR swap_info{};
    swap_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_info.surface = state.surface;
    swap_info.minImageCount = image_count;
    swap_info.imageFormat = chosen.format;
    swap_info.imageColorSpace = chosen.colorSpace;
    swap_info.imageExtent = extent;
    swap_info.imageArrayLayers = 1;
    swap_info.imageUsage = swap_usage;
    swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_info.preTransform = caps.currentTransform;
    swap_info.compositeAlpha = alpha;
    swap_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swap_info.clipped = VK_TRUE;
    Check(device_function.operator()<PFN_vkCreateSwapchainKHR>("vkCreateSwapchainKHR")(
        state.device, &swap_info, nullptr, &state.swapchain), "vkCreateSwapchainKHR");
    const auto get_images = device_function.operator()<PFN_vkGetSwapchainImagesKHR>("vkGetSwapchainImagesKHR");
    const auto images = Enumerate<VkImage>([&](auto* count, auto* values) {
        return get_images(state.device, state.swapchain, count, values);
    }, "vkGetSwapchainImagesKHR");
    Require(images.size() >= image_count, "swapchain returned fewer images than requested");
    std::cout << "logical_device_queue_created=1\nswapchain_created=1; actual_images=" << images.size()
              << "; extent=" << extent.width << 'x' << extent.height << "; format=" << chosen.format
              << "; color_space=" << chosen.colorSpace << "; present_mode=" << swap_info.presentMode
              << "; image_usage=" << swap_info.imageUsage << '\n';
    std::cout << "acquire_submit_present_draw=not-run; texture_pipeline_msaa_images_timestamps_screenshot=not-run\n";
}
} // namespace

int main() {
    std::cout << std::unitbuf;
    Resources resources;
    int result = 0;
    try {
        Probe(resources);
        resources.Close();
        std::cout << "cleanup_order=device_surface_instance_window_loader; completed=1\n";
    } catch (const std::exception& error) {
        std::cerr << "[vulkan capabilities probe] " << error.what() << '\n';
        result = 1;
        try { resources.Close(); }
        catch (const std::exception& cleanup) { std::cerr << "[probe cleanup] " << cleanup.what() << '\n'; }
    }
    std::cout << "validation_warnings=" << resources.diagnostics.warnings.load()
              << "; validation_errors=" << resources.diagnostics.errors.load() << '\n';
    if (resources.diagnostics.warnings.load() || resources.diagnostics.errors.load()) result = 1;
    std::cout << "capability_probe_exit_code=" << result << '\n';
    return result;
}
