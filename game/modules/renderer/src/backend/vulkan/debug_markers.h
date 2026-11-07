#pragma once

#if defined(SYMOCRAFT_GPU_MARKERS_ENABLED)
#include <vulkan/vulkan.h>
#include <cstdio>
#include <string>
#include <type_traits>

namespace SymoCraft::Rendering::Detail {
    struct VulkanAnnotations {
        PFN_vkSetDebugUtilsObjectNameEXT name{};
        PFN_vkCmdBeginDebugUtilsLabelEXT begin{};
        PFN_vkCmdEndDebugUtilsLabelEXT end{};
        void Load(PFN_vkGetDeviceProcAddr get, VkDevice device, bool enabled) noexcept {
            if (enabled) {
                name = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(get(device, "vkSetDebugUtilsObjectNameEXT"));
                begin = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(get(device, "vkCmdBeginDebugUtilsLabelEXT"));
                end = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(get(device, "vkCmdEndDebugUtilsLabelEXT"));
            }
            if (!name || !begin || !end) {
                name = nullptr; begin = nullptr; end = nullptr;
                std::fputs("[renderer] Vulkan annotations disabled: optional VK_EXT_debug_utils or annotation procedures unavailable\n", stderr);
            }
        }
        template<class Handle> void Name(VkDevice device, Handle handle, VkObjectType type, const std::string& label) const noexcept {
            if (!name || !handle) return;
            VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
            info.objectType = type;
            if constexpr (std::is_pointer_v<Handle>) info.objectHandle = reinterpret_cast<uint64_t>(handle);
            else info.objectHandle = static_cast<uint64_t>(handle);
            info.pObjectName = label.c_str();
            if (name(device, &info) != VK_SUCCESS) std::fputs("[renderer] Vulkan resource annotation unavailable\n", stderr);
        }
    };
    class VulkanDebugRegion final {
        const VulkanAnnotations* annotations_{};
        VkCommandBuffer command_{};
    public:
        VulkanDebugRegion(const VulkanAnnotations& annotations, VkCommandBuffer command, const char* name, bool active = true) noexcept {
            if (!active || !annotations.begin || !annotations.end) return;
            VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT}; label.pLabelName = name;
            label.color[0] = 0.3f; label.color[1] = 0.6f; label.color[2] = 0.9f; label.color[3] = 1;
            annotations.begin(command, &label); annotations_ = &annotations; command_ = command;
        }
        ~VulkanDebugRegion() { if (annotations_) annotations_->end(command_); }
        VulkanDebugRegion(const VulkanDebugRegion&) = delete;
        VulkanDebugRegion& operator=(const VulkanDebugRegion&) = delete;
    };
}
#define SYMOCRAFT_VK_NAME(n, object, type, label) (n).annotations.Name((n).device, object, type, label)
#define SYMOCRAFT_VK_REGION(n, command, label) ::SymoCraft::Rendering::Detail::VulkanDebugRegion symocraft_gpu_region((n).annotations, command, label)
#define SYMOCRAFT_VK_REGION_IF(n, command, label, active) ::SymoCraft::Rendering::Detail::VulkanDebugRegion symocraft_gpu_region((n).annotations, command, label, active)
#else
#define SYMOCRAFT_VK_NAME(n, object, type, label) ((void)0)
#define SYMOCRAFT_VK_REGION(n, command, label) ((void)0)
#define SYMOCRAFT_VK_REGION_IF(n, command, label, active) ((void)0)
#endif
