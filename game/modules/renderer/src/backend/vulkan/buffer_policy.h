#pragma once

#include <vulkan/vulkan_core.h>
#include <memory>
#include <optional>
#include <utility>

namespace SymoCraft::Rendering::Detail {
inline std::optional<uint32_t> HostVisibleBufferMemoryType(
    const VkPhysicalDeviceMemoryProperties& memory, uint32_t bits, bool prefer_device_local) {
    std::optional<uint32_t> fallback;
    for (uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
        const auto flags = memory.memoryTypes[i].propertyFlags;
        if (!(bits & (1u << i)) || !(flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) continue;
        if (!fallback) fallback = i;
        if (!prefer_device_local || (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) return i;
    }
    return fallback;
}

// The caller must finish the slot and reset its commands before writing retained storage.
template<class Buffer, class Create, class Write>
void WriteRetainedSelectionBuffer(std::shared_ptr<Buffer>& buffer, Create&& create, Write&& write) {
    if (buffer) {
        write(*buffer);
    } else {
        auto candidate = create();
        write(*candidate);
        buffer = std::move(candidate);
    }
}
}
