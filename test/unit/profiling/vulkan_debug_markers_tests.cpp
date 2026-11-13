#include "debug_markers.h"
#include <stdexcept>
#include <string_view>

using namespace SymoCraft::Rendering::Detail;
namespace {
    unsigned opened{}, closed{}, named{}, loads{};
    VKAPI_ATTR void VKAPI_CALL Begin(VkCommandBuffer, const VkDebugUtilsLabelEXT* label) {
        if (std::string_view(label->pLabelName).empty()) throw std::logic_error("Empty label");
        ++opened;
    }
    VKAPI_ATTR void VKAPI_CALL End(VkCommandBuffer) { ++closed; }
    VKAPI_ATTR VkResult VKAPI_CALL Name(VkDevice, const VkDebugUtilsObjectNameInfoEXT* info) {
        if (info->objectType != VK_OBJECT_TYPE_BUFFER || info->objectHandle != 2 ||
            std::string_view(info->pObjectName) != "World.Mesh[1]") return VK_ERROR_UNKNOWN;
        ++named; return VK_SUCCESS;
    }
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Get(VkDevice, const char* name) {
        ++loads;
        if (std::string_view(name) == "vkSetDebugUtilsObjectNameEXT") return reinterpret_cast<PFN_vkVoidFunction>(Name);
        if (std::string_view(name) == "vkCmdBeginDebugUtilsLabelEXT") return reinterpret_cast<PFN_vkVoidFunction>(Begin);
        if (std::string_view(name) == "vkCmdEndDebugUtilsLabelEXT") return reinterpret_cast<PFN_vkVoidFunction>(End);
        return nullptr;
    }
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL MissingEnd(VkDevice device, const char* name) {
        return std::string_view(name) == "vkCmdEndDebugUtilsLabelEXT" ? nullptr : Get(device, name);
    }
    void Require(bool condition) { if (!condition) throw std::runtime_error("Vulkan annotation ownership/fallback failed"); }
}
int main() {
    try {
        VulkanAnnotations annotations;
        annotations.Load(Get, {}, false);
        { VulkanDebugRegion absent(annotations, {}, "absent"); }
        Require(loads == 0 && opened == 0 && closed == 0);
        annotations.Load(MissingEnd, {}, true);
        { VulkanDebugRegion absent(annotations, {}, "partial-procedures"); }
        Require(!annotations.name && !annotations.begin && !annotations.end && opened == 0);
        annotations.Load(Get, {}, true);
        annotations.Name({}, reinterpret_cast<VkBuffer>(2), VK_OBJECT_TYPE_BUFFER, std::string("World.Mesh[1]"));
        Require(named == 1);
        try {
            VulkanDebugRegion outer(annotations, {}, "world");
            VulkanDebugRegion inner(annotations, {}, "selection");
            throw 7;
        } catch (int) {}
        { VulkanDebugRegion empty(annotations, {}, "no-work", false); }
        Require(opened == 2 && closed == 2);
        return 0;
    } catch (...) { return 1; }
}
