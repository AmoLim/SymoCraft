#pragma once

#include <symocraft/platform/window.h>
#include <vulkan_bridge.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace SymoCraftVulkanExperiment {
using SymoCraft::Window;
using SymoCraft::Platform::VulkanBridge;
constexpr std::uint64_t Timeout = 5'000'000'000;
#ifdef SYMOCRAFT_VULKAN_E01_DEBUG_ABI
constexpr bool DirectionShaderInput = true;
#else
constexpr bool DirectionShaderInput = false;
#endif

inline void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
inline void Check(VkResult result, const char* operation) {
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
    throw std::runtime_error(std::string(operation) + ": unstable enumeration");
}
struct Diagnostics { std::atomic<unsigned> warnings{}, errors{}; };
inline VKAPI_ATTR VkBool32 VKAPI_CALL Message(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user) {
    auto& diagnostics = *static_cast<Diagnostics*>(user);
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ++diagnostics.warnings;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) ++diagnostics.errors;
    std::fprintf(stderr, "[validation] severity=%u type=%u id=%s message=%s\n",
        static_cast<unsigned>(severity), static_cast<unsigned>(types),
        data && data->pMessageIdName ? data->pMessageIdName : "missing",
        data && data->pMessage ? data->pMessage : "missing");
    return VK_FALSE;
}

#define VK_FUNCTION(name) Function<PFN_##name>(#name)
struct Context {
    Diagnostics diagnostics;
    std::unique_ptr<Window> window;
    VkInstance instance{};
    VkDebugUtilsMessengerEXT messenger{};
    VkSurfaceKHR surface{};
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkQueue queue{};
    std::uint32_t queue_family{};
    std::string device_name, driver_version;
    std::uint32_t api_version{};
    VkSwapchainKHR swapchain{};
    std::vector<VkSwapchainKHR> retired_swapchains;
    unsigned swapchain_create_calls{}, swapchain_destroy_calls{}, acquire_calls{}, submit_calls{}, present_calls{};
    unsigned swap_views_live{}, framebuffers_live{};
    VkExtent2D extent{};
    VkFormat format{};
    std::vector<VkImage> swap_images;
    VkPhysicalDeviceMemoryProperties memory{};
    PFN_vkGetInstanceProcAddr get{};
    PFN_vkGetDeviceProcAddr get_device{};
    PFN_vkDestroyInstance destroy_instance{};
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_messenger{};
    PFN_vkDestroyDevice destroy_device{};
    PFN_vkDestroySwapchainKHR destroy_swapchain{};
    std::vector<std::function<void()>> cleanup;
    bool pending = false;
    bool lifetime_mode = false;

    template<class T> T Function(const char* name) const {
        const auto value = reinterpret_cast<T>(device ? get_device(device, name) : get(instance, name));
        if (!value) throw std::runtime_error(std::string("missing Vulkan function: ") + name);
        return value;
    }
    void RequireCleanValidation(const char* operation) const {
        if (diagnostics.warnings.load() || diagnostics.errors.load())
            throw std::runtime_error(std::string(operation) + ": Vulkan validation already reported warning/error; stopping before further submission");
    }
    void Close() {
        Require(!pending, "no completion proof; retaining all native resources until process exit");
        while (!cleanup.empty()) { cleanup.back()(); cleanup.pop_back(); }
        if (swapchain || !retired_swapchains.empty()) Require(destroy_swapchain != nullptr, "missing swapchain destroy");
        while (!retired_swapchains.empty()) { destroy_swapchain(device, retired_swapchains.back(), nullptr); ++swapchain_destroy_calls; retired_swapchains.pop_back(); }
        if (swapchain) { destroy_swapchain(device, swapchain, nullptr); ++swapchain_destroy_calls; swapchain = {}; }
        if (device) { Require(destroy_device != nullptr, "missing device destroy"); destroy_device(device, nullptr); device = {}; }
        if (surface) { VulkanBridge::DestroySurface(*window, instance, surface); surface = {}; }
        if (messenger) { Require(destroy_messenger != nullptr, "missing messenger destroy"); destroy_messenger(instance, messenger, nullptr); messenger = {}; }
        if (instance) { Require(destroy_instance != nullptr, "missing instance destroy"); destroy_instance(instance, nullptr); instance = {}; }
        if (window) { window->Destroy(); window.reset(); }
        Window::Free();
    }
    ~Context() noexcept {
        try { Close(); }
        catch (const std::exception& error) {
            std::fprintf(stderr, "[terminal retention] %s\n", error.what());
            (void)window.release();
        }
    }
    std::uint32_t MemoryType(std::uint32_t bits, VkMemoryPropertyFlags required) const {
        for (std::uint32_t i = 0; i < memory.memoryTypeCount; ++i)
            if ((bits & (1u << i)) && (memory.memoryTypes[i].propertyFlags & required) == required) return i;
        throw std::runtime_error("required actual Vulkan memory type is unavailable");
    }
    void Initialize(bool lifetime = false, const char* title = nullptr) {
        lifetime_mode = lifetime;
        Window::Init();
        window.reset(Window::Create(title ? title : (lifetime ? "SymoCraft Vulkan lifetime E02" : "SymoCraft Vulkan textured E01"), 640, 480, false, true, SymoCraft::WindowMode::Vulkan));
        get = VulkanBridge::GetInstanceProcedureAddress(*window);
        std::uint32_t version{};
        Check(VK_FUNCTION(vkEnumerateInstanceVersion)(&version), "vkEnumerateInstanceVersion");
        Require(version >= VK_API_VERSION_1_2, "Vulkan loader 1.2 is required");
        const auto enum_layers = VK_FUNCTION(vkEnumerateInstanceLayerProperties);
        const auto layers = Enumerate<VkLayerProperties>(enum_layers, "vkEnumerateInstanceLayerProperties");
        constexpr const char* validation = "VK_LAYER_KHRONOS_validation";
        Require(std::any_of(layers.begin(), layers.end(), [&](const auto& layer) { return std::strcmp(layer.layerName, validation) == 0; }),
            "requested Vulkan validation is missing; no downgrade");
        auto extension_strings = VulkanBridge::GetRequiredExtensions(*window);
        extension_strings.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        extension_strings.emplace_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
        extension_strings.emplace_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
        const auto enum_extensions = VK_FUNCTION(vkEnumerateInstanceExtensionProperties);
        auto available_extensions = Enumerate<VkExtensionProperties>([&](auto* n, auto* values) {
            return enum_extensions(nullptr, n, values);
        }, "vkEnumerateInstanceExtensionProperties");
        if (lifetime) {
            const auto layer_extensions = Enumerate<VkExtensionProperties>([&](auto* n, auto* values) {
                return enum_extensions(validation, n, values);
            }, "vkEnumerateInstanceExtensionProperties(validation)");
            available_extensions.insert(available_extensions.end(), layer_extensions.begin(), layer_extensions.end());
            extension_strings.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
        }
        std::vector<const char*> extensions;
        for (const auto& name : extension_strings) {
            Require(std::any_of(available_extensions.begin(), available_extensions.end(), [&](const auto& e) {
                return name == e.extensionName;
            }), "a required local experiment instance extension is missing");
            extensions.push_back(name.c_str());
            std::cout << "enabled_instance_extension=" << name << '\n';
        }
        VkDebugUtilsMessengerCreateInfoEXT debug{};
        debug.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug.pfnUserCallback = Message;
        debug.pUserData = &diagnostics;
        VkApplicationInfo app{};
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pApplicationName = title ? title : (lifetime ? "SymoCraft Vulkan R1-E02" : "SymoCraft Vulkan R1-E01");
        app.apiVersion = VK_API_VERSION_1_2;
        const VkValidationFeatureEnableEXT synchronization = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
        VkValidationFeaturesEXT validation_features{};
        validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        validation_features.pNext = &debug;
        validation_features.enabledValidationFeatureCount = 1;
        validation_features.pEnabledValidationFeatures = &synchronization;
        VkInstanceCreateInfo settings{};
        settings.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        settings.pNext = lifetime ? static_cast<const void*>(&validation_features) : &debug;
        settings.pApplicationInfo = &app;
        settings.enabledLayerCount = 1;
        settings.ppEnabledLayerNames = &validation;
        settings.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        settings.ppEnabledExtensionNames = extensions.data();
        Check(VK_FUNCTION(vkCreateInstance)(&settings, nullptr, &instance), "vkCreateInstance");
        destroy_instance = VK_FUNCTION(vkDestroyInstance);
        destroy_messenger = VK_FUNCTION(vkDestroyDebugUtilsMessengerEXT);
        Check(VK_FUNCTION(vkCreateDebugUtilsMessengerEXT)(instance, &debug, nullptr, &messenger), "vkCreateDebugUtilsMessengerEXT");
        surface = VulkanBridge::CreateSurface(*window, instance);
        const auto enum_physical = VK_FUNCTION(vkEnumeratePhysicalDevices);
        for (const auto candidate : Enumerate<VkPhysicalDevice>([&](auto* count, auto* values) { return enum_physical(instance, count, values); }, "vkEnumeratePhysicalDevices")) {
            VkPhysicalDeviceIDProperties id{};
            id.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
            VkPhysicalDeviceDriverProperties driver{};
            driver.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
            id.pNext = &driver;
            VkPhysicalDeviceProperties2 properties{};
            properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            properties.pNext = &id;
            VK_FUNCTION(vkGetPhysicalDeviceProperties2)(candidate, &properties);
            const auto& p = properties.properties;
            if (p.vendorID != 0x10de || p.deviceID != 0x2c05 || std::string(p.deviceName).find("RTX 5070 Ti") == std::string::npos) continue;
            Require(!physical, "ambiguous RTX 5070 Ti identity");
            Require(p.apiVersion >= VK_API_VERSION_1_2 && p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, "selected device is not required actual hardware");
            physical = candidate;
            device_name = p.deviceName; driver_version = driver.driverInfo; api_version = p.apiVersion;
            std::cout << "actual_device=" << p.deviceName << "; vendor=" << p.vendorID << "; device=" << p.deviceID << "; api=" << p.apiVersion
                      << "; driver=" << driver.driverInfo << "; luid_valid=" << id.deviceLUIDValid << "; luid_bytes=";
            for (const auto byte : id.deviceLUID) { constexpr char digits[] = "0123456789abcdef"; std::cout << digits[byte >> 4] << digits[byte & 15]; }
            std::cout << '\n';
        }
        Require(physical != VK_NULL_HANDLE, "RTX 5070 Ti is unavailable; no substitute");
        VK_FUNCTION(vkGetPhysicalDeviceMemoryProperties)(physical, &memory);
        const auto get_queues = VK_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);
        std::uint32_t count{};
        get_queues(physical, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        get_queues(physical, &count, families.data());
        queue_family = std::numeric_limits<std::uint32_t>::max();
        for (std::uint32_t i = 0; i < count; ++i) {
            VkBool32 support{};
            Check(VK_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR)(physical, i, surface, &support), "vkGetPhysicalDeviceSurfaceSupportKHR");
            if (queue_family == std::numeric_limits<std::uint32_t>::max() && families[i].queueCount && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && support) queue_family = i;
        }
        Require(queue_family != std::numeric_limits<std::uint32_t>::max(), "no combined graphics/present queue for this experiment");
        destroy_device = VK_FUNCTION(vkDestroyDevice);
        get_device = VK_FUNCTION(vkGetDeviceProcAddr);
        VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT maintenance{};
        maintenance.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
        VkPhysicalDeviceTimelineSemaphoreFeatures timeline{};
        timeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
        maintenance.pNext = lifetime ? &timeline : nullptr;
        VkPhysicalDeviceFeatures2 features{};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features.pNext = &maintenance;
        VK_FUNCTION(vkGetPhysicalDeviceFeatures2)(physical, &features);
        Require(maintenance.swapchainMaintenance1, "this local experiment needs supported swapchain maintenance present fences");
        Require(!lifetime || timeline.timelineSemaphore, "this local lifetime experiment needs actual Vulkan 1.2 timeline semaphores");
        float priority = 1.0f;
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = queue_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        const std::array<const char*, 2> device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME};
        const auto enum_device_extensions = VK_FUNCTION(vkEnumerateDeviceExtensionProperties);
        const auto available_device_extensions = Enumerate<VkExtensionProperties>([&](auto* n, auto* values) {
            return enum_device_extensions(physical, nullptr, n, values);
        }, "vkEnumerateDeviceExtensionProperties");
        for (const auto name : device_extensions) {
            Require(std::any_of(available_device_extensions.begin(), available_device_extensions.end(), [&](const auto& e) {
                return std::strcmp(name, e.extensionName) == 0;
            }), "a required local experiment device extension is missing");
            std::cout << "enabled_device_extension=" << name << '\n';
        }
        VkDeviceCreateInfo device_info{};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.pNext = &maintenance;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = static_cast<std::uint32_t>(device_extensions.size());
        device_info.ppEnabledExtensionNames = device_extensions.data();
        Check(VK_FUNCTION(vkCreateDevice)(physical, &device_info, nullptr, &device), "vkCreateDevice");
        VK_FUNCTION(vkGetDeviceQueue)(device, queue_family, 0, &queue);
        Require(queue != VK_NULL_HANDLE, "device queue is missing");
        destroy_swapchain = VK_FUNCTION(vkDestroySwapchainKHR);
        RebuildSwapchain();
    }
    void RebuildSwapchain() {
        Require(!pending && swap_views_live == 0 && framebuffers_live == 0, "old presentation/frame references are not retired");
        // Instance-level queries must not be resolved with vkGetDeviceProcAddr.
        const auto surface_function = [&]<class T>(const char* name) {
            const auto value = reinterpret_cast<T>(get(instance, name));
            if (!value) throw std::runtime_error(std::string("missing surface query: ") + name);
            return value;
        };
        VkSurfaceCapabilitiesKHR caps{};
        Check(surface_function.operator()<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>("vkGetPhysicalDeviceSurfaceCapabilitiesKHR")(physical, surface, &caps), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
        const auto get_formats = surface_function.operator()<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>("vkGetPhysicalDeviceSurfaceFormatsKHR");
        const auto formats = Enumerate<VkSurfaceFormatKHR>([&](auto* n, auto* values) { return get_formats(physical, surface, n, values); }, "vkGetPhysicalDeviceSurfaceFormatsKHR");
        format = VK_FORMAT_UNDEFINED;
        for (const auto& f : formats)
            if (format == VK_FORMAT_UNDEFINED && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && (f.format == VK_FORMAT_R8G8B8A8_UNORM || f.format == VK_FORMAT_B8G8R8A8_UNORM)) format = f.format;
        Require(format != VK_FORMAT_UNDEFINED, "required UNORM surface format unavailable");
        constexpr VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        Require((caps.supportedUsageFlags & usage) == usage, "swapchain screenshot transfer source unavailable");
        extent = caps.currentExtent;
        if (extent.width == std::numeric_limits<std::uint32_t>::max()) {
            extent.width = std::clamp(static_cast<std::uint32_t>(window->width), caps.minImageExtent.width, caps.maxImageExtent.width);
            extent.height = std::clamp(static_cast<std::uint32_t>(window->height), caps.minImageExtent.height, caps.maxImageExtent.height);
        }
        Require(extent.width && extent.height, "zero drawable extent");
        VkSwapchainCreateInfoKHR swap_info{};
        swap_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swap_info.surface = surface;
        swap_info.minImageCount = caps.maxImageCount ? std::min(caps.minImageCount + 1, caps.maxImageCount) : caps.minImageCount + 1;
        swap_info.imageFormat = format;
        swap_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swap_info.imageExtent = extent;
        swap_info.imageArrayLayers = 1;
        swap_info.imageUsage = usage;
        swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swap_info.preTransform = caps.currentTransform;
        swap_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        Require((caps.supportedCompositeAlpha & swap_info.compositeAlpha) != 0, "opaque composition unsupported for experiment");
        swap_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        // Readback experiments require owned pixels even when another window occludes them.
        swap_info.clipped = VK_FALSE;
        swap_info.oldSwapchain = swapchain;
        retired_swapchains.reserve(retired_swapchains.size() + 1);
        VkSwapchainKHR next{};
        ++swapchain_create_calls;
        Check(VK_FUNCTION(vkCreateSwapchainKHR)(device, &swap_info, nullptr, &next), "vkCreateSwapchainKHR");
        if (swapchain) retired_swapchains.push_back(swapchain);
        swapchain = next;
        const auto get_images = VK_FUNCTION(vkGetSwapchainImagesKHR);
        swap_images = Enumerate<VkImage>([&](auto* n, auto* values) { return get_images(device, swapchain, n, values); }, "vkGetSwapchainImagesKHR");
        Require(!swap_images.empty(), "swapchain contains no images");
        while (!retired_swapchains.empty()) {
            destroy_swapchain(device, retired_swapchains.back(), nullptr);
            ++swapchain_destroy_calls;
            retired_swapchains.pop_back();
        }
        std::cout << "actual_extent=" << extent.width << 'x' << extent.height << "; actual_format=" << format
                  << "; actual_samples=1; actual_present=FIFO; queue_family=" << queue_family
                  << "; validation_enabled=1; window_mode=Vulkan/resizable; focus_policy=allow-unfocused\n";
    }
    struct Buffer { VkBuffer buffer{}; VkDeviceMemory memory{}; VkDeviceSize size{}; };
    Buffer MakeBuffer(VkDeviceSize bytes, VkBufferUsageFlags usage) {
        Buffer output{};
        output.size = bytes;
        const auto destroy = VK_FUNCTION(vkDestroyBuffer);
        const auto free = VK_FUNCTION(vkFreeMemory);
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = bytes;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Check(VK_FUNCTION(vkCreateBuffer)(device, &info, nullptr, &output.buffer), "vkCreateBuffer");
        cleanup.emplace_back([=, this] { destroy(device, output.buffer, nullptr); });
        VkMemoryRequirements requirements{};
        VK_FUNCTION(vkGetBufferMemoryRequirements)(device, output.buffer, &requirements);
        VkMemoryAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = MemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        Check(VK_FUNCTION(vkAllocateMemory)(device, &allocation, nullptr, &output.memory), "vkAllocateMemory(buffer)");
        // Free allocation after its bound object, not the reverse creation order.
        cleanup.insert(cleanup.end() - 1, [=, this] { free(device, output.memory, nullptr); });
        Check(VK_FUNCTION(vkBindBufferMemory)(device, output.buffer, output.memory, 0), "vkBindBufferMemory");
        return output;
    }
    void Write(const Buffer& buffer, const void* bytes) {
        void* mapped{};
        Check(VK_FUNCTION(vkMapMemory)(device, buffer.memory, 0, buffer.size, 0, &mapped), "vkMapMemory(write)");
        std::memcpy(mapped, bytes, static_cast<std::size_t>(buffer.size));
        VK_FUNCTION(vkUnmapMemory)(device, buffer.memory);
    }
    VkImage Texture(VkFormat texture_format) {
        const auto destroy = VK_FUNCTION(vkDestroyImage);
        const auto free = VK_FUNCTION(vkFreeMemory);
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = texture_format;
        info.extent = {2, 2, 1};
        info.mipLevels = 1;
        info.arrayLayers = 2;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkImage image{};
        Check(VK_FUNCTION(vkCreateImage)(device, &info, nullptr, &image), "vkCreateImage(texture)");
        cleanup.emplace_back([=, this] { destroy(device, image, nullptr); });
        VkMemoryRequirements requirements{};
        VK_FUNCTION(vkGetImageMemoryRequirements)(device, image, &requirements);
        VkMemoryAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = MemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkDeviceMemory storage{};
        Check(VK_FUNCTION(vkAllocateMemory)(device, &allocation, nullptr, &storage), "vkAllocateMemory(texture)");
        cleanup.insert(cleanup.end() - 1, [=, this] { free(device, storage, nullptr); });
        Check(VK_FUNCTION(vkBindImageMemory)(device, image, storage, 0), "vkBindImageMemory");
        return image;
    }
    VkImageView View(VkImage image, VkFormat view_format, VkImageViewType type, std::uint32_t layers) {
        const auto destroy = VK_FUNCTION(vkDestroyImageView);
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = image;
        info.viewType = type;
        info.format = view_format;
        info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers};
        VkImageView view{};
        Check(VK_FUNCTION(vkCreateImageView)(device, &info, nullptr, &view), "vkCreateImageView");
        if (type == VK_IMAGE_VIEW_TYPE_2D) ++swap_views_live;
        cleanup.emplace_back([=, this] { destroy(device, view, nullptr); if (type == VK_IMAGE_VIEW_TYPE_2D) --swap_views_live; });
        return view;
    }
};

struct Vertex { std::int32_t position[3]; float uv_layer[3]; float direction; };
static_assert(sizeof(Vertex) == 28 && offsetof(Vertex, position) == 0 && offsetof(Vertex, uv_layer) == 12 && offsetof(Vertex, direction) == 24);
inline std::array<Vertex, 12> Vertices() {
    std::array<Vertex, 12> vertices{};
    std::size_t index = 0;
    for (std::int32_t layer = 0; layer < 2; ++layer) {
        const auto left = layer == 0 ? -3 : 1;
        const auto right = left + 2;
        for (const auto& v : std::array<Vertex, 6>{{
            {{left, 2, 0}, {0, 0, static_cast<float>(layer)}, 1},
            {{left, -2, 0}, {0, 1, static_cast<float>(layer)}, 1},
            {{right, -2, 0}, {1, 1, static_cast<float>(layer)}, 1},
            {{left, 2, 0}, {0, 0, static_cast<float>(layer)}, 1},
            {{right, -2, 0}, {1, 1, static_cast<float>(layer)}, 1},
            {{right, 2, 0}, {1, 0, static_cast<float>(layer)}, 1}}}) vertices[index++] = v;
    }
    return vertices;
}
inline std::vector<std::uint32_t> Shader(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    Require(input.good(), "shader file unavailable");
    const auto bytes = static_cast<std::streamoff>(input.tellg());
    Require(bytes > 0 && bytes % 4 == 0, "invalid SPIR-V byte size");
    std::vector<std::uint32_t> words(static_cast<std::size_t>(bytes) / 4);
    input.seekg(0);
    input.read(reinterpret_cast<char*>(words.data()), bytes);
    Require(input.good() && words.front() == 0x07230203, "SPIR-V read or magic validation failed");
    return words;
}

inline void Barrier(Context& c, VkCommandBuffer command, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
    VkAccessFlags source, VkAccessFlags destination, VkPipelineStageFlags source_stage, VkPipelineStageFlags destination_stage, std::uint32_t layers = 1) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = source;
    barrier.dstAccessMask = destination;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers};
    c.VK_FUNCTION(vkCmdPipelineBarrier)(command, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

struct DrawResources {
    Context::Buffer vertex, staging;
    VkImage texture{};
    VkImageView texture_view{};
    std::uint32_t vertex_count{};
    bool upload_texture = false;
    std::string filename;
    std::function<void(VkFence, VkSemaphore)> while_pending;
    std::array<float, 16> matrix{.25f, 0, 0, 0, 0, .35f, 0, 0, 0, 0, 1, 0, .08f, -.06f, .4f, 1};
};

inline void Draw(Context& c, const std::vector<std::uint32_t>& vs, const std::vector<std::uint32_t>& ps,
    bool srgb, const std::filesystem::path& output, const DrawResources* resources = nullptr) {
    const auto vertices = Vertices();
    const auto matrix = resources ? resources->matrix : DrawResources{}.matrix;
    // Ascending layers, with top-left RGBA8 rows within each asymmetric 2x2 layer.
    const std::array<std::uint8_t, 32> texels{
        255, 32, 64, 255, 16, 240, 80, 255, 48, 96, 224, 255, 192, 128, 24, 255,
        24, 160, 208, 255, 232, 48, 176, 255, 112, 224, 40, 255, 64, 80, 144, 255};
    const auto vertex = resources ? resources->vertex : c.MakeBuffer(sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    const auto uniform = c.MakeBuffer(sizeof(matrix), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    const auto staging = resources ? resources->staging : c.MakeBuffer(sizeof(texels), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    const auto readback = c.MakeBuffer(static_cast<VkDeviceSize>(c.extent.width) * c.extent.height * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    if (!resources) { c.Write(vertex, vertices.data()); c.Write(staging, texels.data()); }
    c.Write(uniform, matrix.data());
    const auto texture_format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    const auto texture = resources ? resources->texture : c.Texture(texture_format);
    const auto texture_view = resources ? resources->texture_view : c.View(texture, texture_format, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 2);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = sampler_info.minFilter = VK_FILTER_NEAREST;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = sampler_info.addressModeV = sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSampler sampler{};
    const auto destroy_sampler = c.VK_FUNCTION(vkDestroySampler);
    Check(c.VK_FUNCTION(vkCreateSampler)(c.device, &sampler_info, nullptr, &sampler), "vkCreateSampler");
    c.cleanup.emplace_back([=, &c] { destroy_sampler(c.device, sampler, nullptr); });
    const std::array<VkDescriptorSetLayoutBinding, 3> bindings{{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}}};
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();
    VkDescriptorSetLayout layout{};
    const auto destroy_layout = c.VK_FUNCTION(vkDestroyDescriptorSetLayout);
    Check(c.VK_FUNCTION(vkCreateDescriptorSetLayout)(c.device, &layout_info, nullptr, &layout), "vkCreateDescriptorSetLayout");
    c.cleanup.emplace_back([=, &c] { destroy_layout(c.device, layout, nullptr); });
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &layout;
    VkPipelineLayout pipeline_layout{};
    const auto destroy_pipeline_layout = c.VK_FUNCTION(vkDestroyPipelineLayout);
    Check(c.VK_FUNCTION(vkCreatePipelineLayout)(c.device, &pipeline_layout_info, nullptr, &pipeline_layout), "vkCreatePipelineLayout");
    c.cleanup.emplace_back([=, &c] { destroy_pipeline_layout(c.device, pipeline_layout, nullptr); });
    const std::array<VkDescriptorPoolSize, 3> sizes{{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}, {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1}, {VK_DESCRIPTOR_TYPE_SAMPLER, 1}}};
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
    pool_info.pPoolSizes = sizes.data();
    VkDescriptorPool pool{};
    const auto destroy_descriptor_pool = c.VK_FUNCTION(vkDestroyDescriptorPool);
    Check(c.VK_FUNCTION(vkCreateDescriptorPool)(c.device, &pool_info, nullptr, &pool), "vkCreateDescriptorPool");
    c.cleanup.emplace_back([=, &c] { destroy_descriptor_pool(c.device, pool, nullptr); });
    VkDescriptorSetAllocateInfo set_info{};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = pool;
    set_info.descriptorSetCount = 1;
    set_info.pSetLayouts = &layout;
    VkDescriptorSet set{};
    Check(c.VK_FUNCTION(vkAllocateDescriptorSets)(c.device, &set_info, &set), "vkAllocateDescriptorSets");
    VkDescriptorBufferInfo buffer_info{uniform.buffer, 0, sizeof(matrix)};
    VkDescriptorImageInfo image_info{VK_NULL_HANDLE, texture_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo sampler_descriptor{sampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    std::array<VkWriteDescriptorSet, 3> writes{};
    for (std::size_t i = 0; i < writes.size(); ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = set;
        writes[i].dstBinding = static_cast<std::uint32_t>(i);
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = bindings[i].descriptorType;
    }
    writes[0].pBufferInfo = &buffer_info; writes[1].pImageInfo = &image_info; writes[2].pImageInfo = &sampler_descriptor;
    c.VK_FUNCTION(vkUpdateDescriptorSets)(c.device, static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);

    VkAttachmentDescription attachment{};
    attachment.format = c.format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &reference;
    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0] = {VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0};
    dependencies[1] = {0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, 0};
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
    render_pass_info.pDependencies = dependencies.data();
    VkRenderPass render_pass{};
    const auto destroy_render_pass = c.VK_FUNCTION(vkDestroyRenderPass);
    Check(c.VK_FUNCTION(vkCreateRenderPass)(c.device, &render_pass_info, nullptr, &render_pass), "vkCreateRenderPass");
    c.cleanup.emplace_back([=, &c] { destroy_render_pass(c.device, render_pass, nullptr); });
    const auto module = [&](const std::vector<std::uint32_t>& code) {
        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = code.size() * 4;
        info.pCode = code.data();
        VkShaderModule result{};
        const auto destroy = c.VK_FUNCTION(vkDestroyShaderModule);
        Check(c.VK_FUNCTION(vkCreateShaderModule)(c.device, &info, nullptr, &result), "vkCreateShaderModule");
        c.cleanup.emplace_back([=, &c] { destroy(c.device, result, nullptr); });
        return result;
    };
    const auto vertex_module = module(vs), fragment_module = module(ps);
    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT; stages[0].module = vertex_module; stages[0].pName = "VSMain";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module = fragment_module; stages[1].pName = "PSMain";
    VkVertexInputBindingDescription vertex_binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    // The unoptimized same-source Debug shader retains DIRECTION; Release removes it.
    const std::array<VkVertexInputAttributeDescription, 3> attributes{{
        {0, 0, VK_FORMAT_R32G32B32_SINT, 0}, {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12}, {2, 0, VK_FORMAT_R32_SFLOAT, 24}}};
    VkPipelineVertexInputStateCreateInfo vertex_info{};
    vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_info.vertexBindingDescriptionCount = 1;
    vertex_info.pVertexBindingDescriptions = &vertex_binding;
    vertex_info.vertexAttributeDescriptionCount = DirectionShaderInput ? 3u : 2u;
    vertex_info.pVertexAttributeDescriptions = attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport viewport{0, static_cast<float>(c.extent.height), static_cast<float>(c.extent.width), -static_cast<float>(c.extent.height), 0, 1};
    VkRect2D scissor{{0, 0}, c.extent};
    VkPipelineViewportStateCreateInfo viewport_info{};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1; viewport_info.pViewports = &viewport;
    viewport_info.scissorCount = 1; viewport_info.pScissors = &scissor;
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1;
    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState blend{};
    blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blending{};
    blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blending.attachmentCount = 1; blending.pAttachments = &blend;
    VkGraphicsPipelineCreateInfo graphics{};
    graphics.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics.stageCount = static_cast<std::uint32_t>(stages.size()); graphics.pStages = stages.data();
    graphics.pVertexInputState = &vertex_info; graphics.pInputAssemblyState = &assembly;
    graphics.pViewportState = &viewport_info; graphics.pRasterizationState = &raster;
    graphics.pMultisampleState = &msaa; graphics.pColorBlendState = &blending;
    graphics.layout = pipeline_layout; graphics.renderPass = render_pass;
    VkPipeline pipeline{};
    const auto destroy_pipeline = c.VK_FUNCTION(vkDestroyPipeline);
    Check(c.VK_FUNCTION(vkCreateGraphicsPipelines)(c.device, VK_NULL_HANDLE, 1, &graphics, nullptr, &pipeline), "vkCreateGraphicsPipelines");
    c.cleanup.emplace_back([=, &c] { destroy_pipeline(c.device, pipeline, nullptr); });
    std::vector<VkFramebuffer> framebuffers;
    for (const auto image : c.swap_images) {
        const auto view = c.View(image, c.format, VK_IMAGE_VIEW_TYPE_2D, 1);
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = render_pass;
        info.attachmentCount = 1; info.pAttachments = &view;
        info.width = c.extent.width; info.height = c.extent.height; info.layers = 1;
        VkFramebuffer framebuffer{};
        const auto destroy = c.VK_FUNCTION(vkDestroyFramebuffer);
        Check(c.VK_FUNCTION(vkCreateFramebuffer)(c.device, &info, nullptr, &framebuffer), "vkCreateFramebuffer");
        ++c.framebuffers_live;
        c.cleanup.emplace_back([=, &c] { destroy(c.device, framebuffer, nullptr); --c.framebuffers_live; });
        framebuffers.push_back(framebuffer);
    }
    VkCommandPoolCreateInfo commands_info{};
    commands_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commands_info.queueFamilyIndex = c.queue_family;
    VkCommandPool commands{};
    const auto destroy_command_pool = c.VK_FUNCTION(vkDestroyCommandPool);
    Check(c.VK_FUNCTION(vkCreateCommandPool)(c.device, &commands_info, nullptr, &commands), "vkCreateCommandPool");
    c.cleanup.emplace_back([=, &c] { destroy_command_pool(c.device, commands, nullptr); });
    VkCommandBufferAllocateInfo command_info{};
    command_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_info.commandPool = commands;
    command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_info.commandBufferCount = 1;
    VkCommandBuffer command{};
    Check(c.VK_FUNCTION(vkAllocateCommandBuffers)(c.device, &command_info, &command), "vkAllocateCommandBuffers");
    const auto semaphore = [&] {
        VkSemaphoreCreateInfo info{}; info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkSemaphore value{};
        const auto destroy = c.VK_FUNCTION(vkDestroySemaphore);
        Check(c.VK_FUNCTION(vkCreateSemaphore)(c.device, &info, nullptr, &value), "vkCreateSemaphore");
        c.cleanup.emplace_back([=, &c] { destroy(c.device, value, nullptr); });
        return value;
    };
    const auto acquire = semaphore(), rendered = semaphore();
    VkSemaphore gate{};
    if (resources && resources->while_pending) {
        Require(c.lifetime_mode, "lifetime draws require the explicitly enabled native timeline feature");
        VkSemaphoreTypeCreateInfo type{};
        type.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.pNext = &type;
        const auto destroy = c.VK_FUNCTION(vkDestroySemaphore);
        Check(c.VK_FUNCTION(vkCreateSemaphore)(c.device, &info, nullptr, &gate), "vkCreateSemaphore(timeline gate)");
        c.cleanup.emplace_back([=, &c] { destroy(c.device, gate, nullptr); });
    }
    const auto fence = [&] {
        VkFenceCreateInfo info{}; info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence value{};
        const auto destroy = c.VK_FUNCTION(vkDestroyFence);
        Check(c.VK_FUNCTION(vkCreateFence)(c.device, &info, nullptr, &value), "vkCreateFence");
        c.cleanup.emplace_back([=, &c] { destroy(c.device, value, nullptr); });
        return value;
    };
    const auto submitted = fence(), retired = fence();
    std::uint32_t index{};
    c.RequireCleanValidation("before acquire");
    c.pending = true;
    ++c.acquire_calls;
    Check(c.VK_FUNCTION(vkAcquireNextImageKHR)(c.device, c.swapchain, Timeout, acquire, VK_NULL_HANDLE, &index), "vkAcquireNextImageKHR");
    Require(index < framebuffers.size(), "acquired image index outside framebuffer list");
    VkCommandBufferBeginInfo begin{}; begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    Check(c.VK_FUNCTION(vkBeginCommandBuffer)(command, &begin), "vkBeginCommandBuffer");
    if (!resources || resources->upload_texture) {
        Barrier(c, command, texture, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 2);
        VkBufferImageCopy upload{};
        upload.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 2}; upload.imageExtent = {2, 2, 1};
        c.VK_FUNCTION(vkCmdCopyBufferToImage)(command, staging.buffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &upload);
        Barrier(c, command, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 2);
    }
    VkClearValue clear{}; clear.color = {{8.f / 255, 16.f / 255, 24.f / 255, 1}};
    VkRenderPassBeginInfo render{}; render.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render.renderPass = render_pass; render.framebuffer = framebuffers[index]; render.renderArea = scissor;
    render.clearValueCount = 1; render.pClearValues = &clear;
    c.VK_FUNCTION(vkCmdBeginRenderPass)(command, &render, VK_SUBPASS_CONTENTS_INLINE);
    c.VK_FUNCTION(vkCmdBindPipeline)(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    c.VK_FUNCTION(vkCmdBindDescriptorSets)(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &set, 0, nullptr);
    const VkDeviceSize offset = 0;
    const auto vertex_count = resources ? resources->vertex_count : static_cast<std::uint32_t>(vertices.size());
    if (vertex_count) {
        c.VK_FUNCTION(vkCmdBindVertexBuffers)(command, 0, 1, &vertex.buffer, &offset);
        c.VK_FUNCTION(vkCmdDraw)(command, vertex_count, 1, 0, 0);
    }
    c.VK_FUNCTION(vkCmdEndRenderPass)(command);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}; copy.imageExtent = {c.extent.width, c.extent.height, 1};
    c.VK_FUNCTION(vkCmdCopyImageToBuffer)(command, c.swap_images[index], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1, &copy);
    VkBufferMemoryBarrier host{};
    host.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    host.srcQueueFamilyIndex = host.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    host.buffer = readback.buffer; host.offset = 0; host.size = readback.size;
    c.VK_FUNCTION(vkCmdPipelineBarrier)(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &host, 0, nullptr);
    Barrier(c, command, c.swap_images[index], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_READ_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    Check(c.VK_FUNCTION(vkEndCommandBuffer)(command), "vkEndCommandBuffer");
    c.RequireCleanValidation("before queue submit");
    const std::array<VkSemaphore, 2> waits{acquire, gate};
    const std::array<VkPipelineStageFlags, 2> wait_stages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};
    const std::array<std::uint64_t, 2> wait_values{0, 1};
    const std::uint64_t signal_value = 0;
    VkTimelineSemaphoreSubmitInfo timeline_submit{};
    timeline_submit.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline_submit.waitSemaphoreValueCount = 2;
    timeline_submit.pWaitSemaphoreValues = wait_values.data();
    timeline_submit.signalSemaphoreValueCount = 1;
    timeline_submit.pSignalSemaphoreValues = &signal_value;
    VkSubmitInfo submit{}; submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = gate ? &timeline_submit : nullptr;
    submit.waitSemaphoreCount = gate ? 2u : 1u;
    submit.pWaitSemaphores = waits.data(); submit.pWaitDstStageMask = wait_stages.data();
    submit.commandBufferCount = 1; submit.pCommandBuffers = &command;
    submit.signalSemaphoreCount = 1; submit.pSignalSemaphores = &rendered;
    ++c.submit_calls;
    Check(c.VK_FUNCTION(vkQueueSubmit)(c.queue, 1, &submit, submitted), "vkQueueSubmit(draw)");
    if (gate) {
        Require(c.VK_FUNCTION(vkGetFenceStatus)(c.device, submitted) == VK_NOT_READY, "native gated submission was unexpectedly completed");
        resources->while_pending(submitted, gate);
        c.RequireCleanValidation("before timeline release");
        VkSemaphoreSignalInfo signal{};
        signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
        signal.semaphore = gate; signal.value = 1;
        Check(c.VK_FUNCTION(vkSignalSemaphore)(c.device, &signal), "vkSignalSemaphore(host release)");
    }
    VkSwapchainPresentFenceInfoEXT presentation_fence{};
    presentation_fence.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
    presentation_fence.swapchainCount = 1;
    presentation_fence.pFences = &retired;
    VkPresentInfoKHR present{}; present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = &presentation_fence;
    present.waitSemaphoreCount = 1; present.pWaitSemaphores = &rendered;
    present.swapchainCount = 1; present.pSwapchains = &c.swapchain; present.pImageIndices = &index;
    ++c.present_calls;
    Check(c.VK_FUNCTION(vkQueuePresentKHR)(c.queue, &present), "vkQueuePresentKHR");
    Check(c.VK_FUNCTION(vkWaitForFences)(c.device, 1, &retired, VK_TRUE, Timeout), "vkWaitForFences(maintenance presentation fence)");
    Check(c.VK_FUNCTION(vkWaitForFences)(c.device, 1, &submitted, VK_TRUE, Timeout), "vkWaitForFences(draw readback)");
    c.pending = false;
    void* mapped{};
    Check(c.VK_FUNCTION(vkMapMemory)(c.device, readback.memory, 0, readback.size, 0, &mapped), "vkMapMemory(readback)");
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(readback.size));
    std::memcpy(rgba.data(), mapped, rgba.size());
    c.VK_FUNCTION(vkUnmapMemory)(c.device, readback.memory);
    if (c.format == VK_FORMAT_B8G8R8A8_UNORM)
        for (std::size_t i = 0; i < rgba.size(); i += 4) std::swap(rgba[i], rgba[i + 2]);
    const auto path = output / (resources ? resources->filename : (srgb ? "srgb.rgba" : "linear.rgba"));
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(rgba.data()), static_cast<std::streamsize>(rgba.size()));
    file.flush();
    Require(file.good(), "RGBA screenshot output failed");
    auto metadata_path = path;
    metadata_path.replace_extension(".json");
    std::ofstream metadata(metadata_path);
    metadata << "{\"schema\":\"symocraft.vulkan.capture.v1\",\"width\":" << c.extent.width << ",\"height\":" << c.extent.height
             << ",\"format\":\"RGBA8\",\"origin\":\"top-left\"}";
    Require(metadata.good(), "RGBA screenshot metadata output failed");
    std::cout << "capture=" << path.string() << "; width=" << c.extent.width << "; height=" << c.extent.height
              << "; format=RGBA8; origin=top-left; texture=" << (srgb ? "SRGB-native-decode" : "Linear-preserve")
              << "; vertices=12; triangles=4; layers=2; acquire_submit_present=completed; maintenance_present_fence=completed\n";
}

inline std::string JsonString(const std::string& value) {
    std::string result = "\"";
    constexpr char digits[] = "0123456789abcdef";
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte == '\\' || byte == '"') { result += '\\'; result += character; }
        else if (byte < 32) { result += "\\u00"; result += digits[byte >> 4]; result += digits[byte & 15]; }
        else result += character;
    }
    return result + '"';
}

inline void NativeReport(const Context& c, const std::filesystem::path& output) {
    std::ofstream report(output / "native.json");
    report << "{\n  \"schema\":\"symocraft.vulkan-r1-e01.native.v1\",\n"
           << "  \"device\":{\"device_name\":" << JsonString(c.device_name) << ",\"vendor_id\":4318,\"device_id\":11269,\"api_version\":" << c.api_version
           << ",\"driver_version\":" << JsonString(c.driver_version) << ",\"queue_family\":" << c.queue_family << "},\n"
           << "  \"extent\":{\"width\":" << c.extent.width << ",\"height\":" << c.extent.height << "},\n"
           << "  \"samples\":1,\"present_mode\":\"Fifo\",\"swapchain_format\":"
           << JsonString(c.format == VK_FORMAT_B8G8R8A8_UNORM ? "B8G8R8A8_UNORM" : "R8G8B8A8_UNORM")
           << ",\"swapchain_color_space\":\"SRGB_NONLINEAR\",\n"
           << "  \"validation\":{\"enabled\":true,\"warnings\":" << c.diagnostics.warnings.load() << ",\"errors\":" << c.diagnostics.errors.load() << "},\n"
           << "  \"layout\":{\"vertex_stride\":28,\"position_offset\":0,\"uv_layer_offset\":12,\"direction_offset\":24,\"direction_shader_input\":"
           << (DirectionShaderInput ? "true" : "false")
           << ",\"descriptor_set\":0,\"constant_binding\":0,\"texture_binding\":1,\"sampler_binding\":2,\"matrix\":[0.25,0,0,0,0,0.35,0,0,0,0,1,0,0.08,-0.06,0.4,1],\"matrix_storage\":\"column-major\",\"viewport\":\"negative-height-only\"},\n"
           << "  \"clear_rgba\":[8,16,24,255],\"cleanup_completed\":true,\"present_retirement\":\"EXT_swapchain_maintenance1_present_fence\",\n  \"images\":[\n";
    for (unsigned i = 0; i < 2; ++i) {
        if (i) report << ",\n";
        report << "    {\"space\":\"" << (i ? "SRGB" : "Linear") << "\",\"file\":\"" << (i ? "srgb.rgba" : "linear.rgba")
               << "\",\"width\":" << c.extent.width << ",\"height\":" << c.extent.height
               << ",\"format\":\"RGBA8\",\"origin\":\"top-left\",\"submitted\":true,\"retired\":true,\"present_fence_completed\":true}";
    }
    report << "\n  ],\"E02_E03_full_game_MSAA_GPU_timing_alpha_depth\":\"not-run\"\n}\n";
    Require(report.good(), "native identity report output failed");
}
} // namespace SymoCraftVulkanExperiment
