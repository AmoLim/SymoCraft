#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

namespace SymoCraft::Rendering::Detail {
// Private, one-shot binding; only isolated native diagnostics install it.
struct VulkanDiagnosticAccess {
    enum class Call : unsigned {
        BufferCreate, BufferAllocate, BufferBind, BufferDestroy, BufferFree,
        ImageCreate, ImageAllocate, ImageBind, ImageDestroy, ImageFree,
        MapWrite, UnmapWrite, FlushWrite, PoolCreate, SetsAllocate, DescriptorUpdate, PoolDestroy,
        QueryRead, FencePoll, WaitSubmit, WaitUpload, WaitReacquiredPresent, WaitDrainPresent,
        Acquire, QueueSubmit, QueuePresent, Count
    };
    inline static constexpr std::array<const char*, unsigned(Call::Count)> CallNames{
        "buffer_create", "buffer_allocate", "buffer_bind", "buffer_destroy", "buffer_free",
        "image_create", "image_allocate", "image_bind", "image_destroy", "image_free",
        "map_write", "unmap_write", "flush_write", "pool_create", "sets_allocate", "descriptor_update", "pool_destroy",
        "query_read", "fence_poll", "wait_submit", "wait_upload", "wait_reacquired_present", "wait_drain_present",
        "acquire", "queue_submit", "queue_present"
    };
    struct CallStats { std::uint64_t calls{}, successes{}, wall_ns{}; };
    enum class BufferRole : unsigned { Mesh, MeshUniform, SelectionUniform, SelectionVertex, Staging, Readback, Count };
    inline static constexpr std::array<const char*, unsigned(BufferRole::Count)> BufferRoleNames{
        "mesh", "mesh_uniform", "selection_uniform", "selection_vertex", "staging", "readback"
    };
    struct AllocationStats {
        std::uint64_t allocations{}, binds{}, releases{}, requested_bytes{}, allocated_bytes{}, live_bytes{};
        std::uint32_t requirements_bits_or{}, requirements_bits_and{};
        void Allocate(std::uint64_t requested, std::uint64_t allocated, std::uint32_t bits) noexcept {
            requirements_bits_and = allocations ? requirements_bits_and & bits : bits;
            requirements_bits_or |= bits; ++allocations;
            requested_bytes += requested; allocated_bytes += allocated; live_bytes += allocated;
        }
    };
    bool observe_performance{};
    std::array<CallStats, unsigned(Call::Count)> native_calls{};
    std::array<std::array<AllocationStats, VK_MAX_MEMORY_TYPES>, unsigned(BufferRole::Count)> buffer_allocations{};
    VkPhysicalDeviceMemoryProperties memory_properties{};
    std::uint32_t graphics_family{}, present_family{}, swapchain_images{}, timestamp_bits{};
    float timestamp_period{};
    bool same_graphics_present_queue{};
    std::uint64_t queue_observation_frame_id{};
    std::uint32_t present_image = UINT32_MAX, tracked_submit_pending{}, tracked_present_pending{};
    VkResult submit_before_present = VK_NOT_READY, submit_after_present = VK_NOT_READY;
    std::uint64_t mesh_bound_buffers{}, mesh_bound_bytes{}, mesh_device_local_buffers{}, mesh_device_local_bytes{};
    struct BindingCounters {
        std::uint64_t uniform_allocations{}, pools_created{}, pools_released{}, sets_allocated{};
        std::uint64_t update_calls{}, uniform_descriptors{}, image_descriptors{}, sampler_descriptors{};
        std::uint64_t uniform_writes{}, uniform_bytes{}, capacity{};
        bool operator==(const BindingCounters&) const = default;
    };
    struct BindingSlot {
        BindingCounters mesh, selection;
        bool operator==(const BindingSlot&) const = default;
    };
    std::array<BindingSlot, 3> binding_slots{};
    unsigned last_binding_slot = UINT32_MAX;
    std::uint64_t last_binding_frame_id{};
    VkInstance instance{};
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkQueue graphics{};
    PFN_vkGetDeviceProcAddr get_device_proc{};
    VkFence last_submit{};
    VkSemaphore next_gate{};
    bool enable_timeline_gate{};
    bool deferred_present_pending{};
    std::function<void()> finish_deferred_present;
    VkResult next_allocation = VK_SUCCESS, next_submit = VK_SUCCESS, next_present = VK_SUCCESS;
    VkResult next_pipeline = VK_SUCCESS;
    VkResult next_query = VK_SUCCESS;
    bool disable_timestamps{};
    bool hide_maintenance_extension{}, hide_maintenance_feature{};
    unsigned extension_rejections{}, feature_rejections{}, device_creations{};
    unsigned acquire_calls{}, acquire_out_of_date_injections{}, swapchain_creations{};
    VkResult next_acquire = VK_SUCCESS, last_actual_acquire = VK_SUCCESS;
    VkResult last_effective_acquire = VK_SUCCESS, last_effective_present = VK_SUCCESS;
    unsigned wait_calls{}, query_calls{};
    VkResult last_actual_present = VK_SUCCESS;
    std::uintptr_t loader_address{};
    bool next_host_allocation{}, quarantined{}, native_destroyed{}, loader_released{};
    std::chrono::milliseconds wait_timeout{300};
    std::function<void(VulkanDiagnosticAccess&)> on_initialized;
    std::weak_ptr<void> last_mesh, last_texture;
    std::weak_ptr<void> frame_mesh, frame_texture;
    unsigned buffers_created{}, buffers_released{}, images_created{}, images_released{};
    unsigned submit_attempts{}, successful_submits{}, present_calls{}, warnings{}, errors{};
    std::uint64_t live_bytes{}, peak_bytes{};
};
class ScopedVulkanDiagnostics {
public:
    explicit ScopedVulkanDiagnostics(std::shared_ptr<VulkanDiagnosticAccess>);
    ~ScopedVulkanDiagnostics();
    ScopedVulkanDiagnostics(const ScopedVulkanDiagnostics&) = delete;
    ScopedVulkanDiagnostics& operator=(const ScopedVulkanDiagnostics&) = delete;
};
}
