#pragma once
#include <symocraft/platform/window.h>
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan_core.h>
#include <string>
#include <vector>

namespace SymoCraft::Platform {
    struct VulkanBridge {
        // Instances and surfaces are caller-owned and must end before their window.
        static PFN_vkGetInstanceProcAddr GetInstanceProcedureAddress(Window& window);
        static std::vector<std::string> GetRequiredExtensions(Window& window);
        static VkSurfaceKHR CreateSurface(Window& window, VkInstance instance,
                                          const VkAllocationCallbacks* allocator = nullptr);
        static void DestroySurface(Window& window, VkInstance instance, VkSurfaceKHR surface,
                                   const VkAllocationCallbacks* allocator = nullptr);
    };
}
