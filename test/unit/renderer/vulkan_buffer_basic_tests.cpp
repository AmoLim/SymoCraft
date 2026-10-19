#include "backend/vulkan/buffer_policy.h"
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace {
using namespace SymoCraft::Rendering::Detail;
void Require(bool ok) { if (!ok) throw std::runtime_error("Vulkan buffer basic check failed"); }

VkPhysicalDeviceMemoryProperties NormalMemoryTypes() {
    VkPhysicalDeviceMemoryProperties memory{};
    memory.memoryTypeCount = 5;
    memory.memoryTypes[1] = {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0};
    memory.memoryTypes[2] = {VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1};
    memory.memoryTypes[3] = {VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT, 1};
    memory.memoryTypes[4] = {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0};
    return memory;
}
void MeshLocalPreference() {
    const auto memory = NormalMemoryTypes();
    Require(HostVisibleBufferMemoryType(memory, 31, true) == 4);
}
void CompatibleHostFallback() {
    const auto memory = NormalMemoryTypes();
    Require(HostVisibleBufferMemoryType(memory, 15, true) == 2);
}
void OrdinaryHostBuffers() {
    const auto memory = NormalMemoryTypes();
    Require(HostVisibleBufferMemoryType(memory, 31, false) == 2);
}
struct CpuBuffer {
    unsigned identity{};
    std::array<std::byte, 288> vertices{};
};
void IndependentSelectionSlots() {
    std::array<std::shared_ptr<CpuBuffer>, 3> slots{};
    unsigned allocations = 0, writes = 0;
    for (unsigned frame = 0; frame < 12; ++frame) {
        const auto slot = frame % slots.size();
        WriteRetainedSelectionBuffer(slots[slot], [&] {
            return std::make_shared<CpuBuffer>(CpuBuffer{++allocations});
        }, [&](CpuBuffer& buffer) {
            buffer.vertices.fill(static_cast<std::byte>(frame));
            ++writes;
        });
        Require(slots[slot]->identity == slot + 1);
        Require(slots[slot]->vertices.front() == static_cast<std::byte>(frame));
        Require(slots[slot]->vertices.back() == static_cast<std::byte>(frame));
    }
    Require(allocations == 3 && writes == 12);
    Require(slots[0] != slots[1] && slots[1] != slots[2] && slots[0] != slots[2]);
}
void SelectionHideAndResume() {
    std::shared_ptr<CpuBuffer> slot;
    unsigned allocations = 0, writes = 0;
    for (bool visible : {true, true, false, false, true}) {
        if (visible) WriteRetainedSelectionBuffer(slot, [&] {
            Require(!slot);
            return std::make_shared<CpuBuffer>(CpuBuffer{++allocations});
        }, [&](CpuBuffer& buffer) {
            if (!writes) Require(!slot);
            buffer.vertices.fill(static_cast<std::byte>(++writes));
        });
        Require(slot && slot->identity == 1);
    }
    Require(allocations == 1 && writes == 3 && slot->vertices.front() == std::byte{3});
    std::weak_ptr<CpuBuffer> retained = slot;
    slot.reset();
    Require(retained.expired());
}
}
int main() {
    try {
        MeshLocalPreference(); CompatibleHostFallback(); OrdinaryHostBuffers();
        IndependentSelectionSlots(); SelectionHideAndResume();
        std::cout << "5 normal CPU buffer-policy cases passed: local preference, compatible host fallback, ordinary host policy, independent selection slots, hide/resume; no GPU/native lifetime/boundary/regression/performance tests\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
