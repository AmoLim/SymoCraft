#include "vulkan_backend.h"
#include "diagnostic_access.h"
#include "frame_binding_cache.h"
#include "buffer_policy.h"
#include "../../frame_diagnostics.h"
#include "vulkan_bridge.h"
#include <symocraft/renderer/error.h>
#include <Windows.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>

namespace SymoCraft::Rendering::Detail {
namespace {
using Clock = std::chrono::steady_clock;
using Deadline = Clock::time_point;
using ObservedCall = VulkanDiagnosticAccess::Call;
using BufferRole = VulkanDiagnosticAccess::BufferRole;

[[noreturn]] void Fail(RenderErrorCategory category, const char* operation, const std::string& message,
                      VkResult result = VK_SUCCESS) {
    throw RenderError(category, "Vulkan", operation, message,
                      result == VK_SUCCESS ? std::string{} : std::to_string(result));
}
void Check(VkResult result, const char* operation, bool submitted = false) {
    if (result == VK_SUCCESS) return;
    auto category = RenderErrorCategory::InitializationFailure;
    if (result == VK_ERROR_DEVICE_LOST) category = RenderErrorCategory::DeviceLost;
    else if (result == VK_TIMEOUT || result == VK_NOT_READY) category = RenderErrorCategory::Timeout;
    else if (submitted) category = RenderErrorCategory::SubmissionFailure;
    else if (result == VK_ERROR_FORMAT_NOT_SUPPORTED || result == VK_ERROR_EXTENSION_NOT_PRESENT ||
             result == VK_ERROR_FEATURE_NOT_PRESENT || result == VK_ERROR_INCOMPATIBLE_DRIVER)
        category = RenderErrorCategory::Unsupported;
    else if (result == VK_ERROR_OUT_OF_HOST_MEMORY || result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        category = RenderErrorCategory::AllocationFailure;
    Fail(category, operation, "native operation failed", result);
}
uint64_t Remaining(Deadline deadline) {
    const auto remaining = deadline - Clock::now();
    return remaining <= Clock::duration::zero() ? 0 :
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(remaining).count());
}
template<class T> T Procedure(PFN_vkGetInstanceProcAddr get, VkInstance instance, const char* name) {
    auto result = reinterpret_cast<T>(get(instance, name));
    if (!result) Fail(RenderErrorCategory::Unsupported, "LoadProcedure", std::string("missing entry: ") + name);
    return result;
}
struct IdLess {
    bool operator()(const BackendResourceId& a, const BackendResourceId& b) const noexcept {
        return std::tie(a.owner, a.slot, a.generation) < std::tie(b.owner, b.slot, b.generation);
    }
};

// All dispatch and native ownership stay behind the private backend boundary.
struct Native {
    std::shared_ptr<VulkanDiagnosticAccess> diagnostic;
    HMODULE loader{};
    PFN_vkGetInstanceProcAddr get{};
    VkInstance instance{};
    VkDebugUtilsMessengerEXT messenger{};
    VkSurfaceKHR surface{};
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkQueue graphics{}, present{};
    uint32_t graphics_family{}, present_family{}, timestamp_bits{};
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceMemoryProperties memory{};
    uint32_t reported_mesh_memory_types{};
    VkPhysicalDeviceIDProperties identity{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
    VkPhysicalDeviceDriverProperties driver{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES};
    std::atomic<unsigned> warnings{}, errors{};
    bool khr_maintenance{}, instance_khr_maintenance{}, instance_ext_maintenance{}, unsafe{}, rebuild{};
    std::chrono::milliseconds timeout{};

    template<class Work> auto Observe(ObservedCall call, Work&& work) {
        if (!diagnostic || !diagnostic->observe_performance) return work();
        const auto begin = Clock::now();
        auto& stats = diagnostic->native_calls[unsigned(call)];
        if constexpr (std::is_void_v<decltype(work())>) {
            work(); ++stats.calls; ++stats.successes;
            stats.wall_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count();
        } else {
            const auto result = work(); ++stats.calls; stats.successes += result == VK_SUCCESS;
            stats.wall_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count();
            return result;
        }
    }

    PFN_vkDestroyInstance DestroyInstance{};
    PFN_vkDestroySurfaceKHR DestroySurfaceKHR{};
    PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT{};
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR{};
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR GetPhysicalDeviceSurfaceFormatsKHR{};
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR{};
    PFN_vkGetPhysicalDeviceImageFormatProperties GetPhysicalDeviceImageFormatProperties{};
    PFN_vkGetDeviceProcAddr GetDeviceProcAddr{};

#define DEVICE_PROCEDURES(X) \
    X(DestroyDevice) X(GetDeviceQueue) X(CreateBuffer) X(DestroyBuffer) X(GetBufferMemoryRequirements) \
    X(AllocateMemory) X(FreeMemory) X(BindBufferMemory) X(MapMemory) X(UnmapMemory) \
    X(FlushMappedMemoryRanges) X(InvalidateMappedMemoryRanges) \
    X(CreateImage) X(DestroyImage) X(GetImageMemoryRequirements) X(BindImageMemory) \
    X(CreateImageView) X(DestroyImageView) X(CreateSampler) X(DestroySampler) \
    X(CreateCommandPool) X(DestroyCommandPool) X(ResetCommandPool) X(AllocateCommandBuffers) \
    X(BeginCommandBuffer) X(EndCommandBuffer) X(CreateFence) X(DestroyFence) X(ResetFences) \
    X(WaitForFences) X(GetFenceStatus) X(CreateSemaphore) X(DestroySemaphore) X(QueueSubmit) \
    X(CreateSwapchainKHR) X(DestroySwapchainKHR) X(GetSwapchainImagesKHR) X(AcquireNextImageKHR) X(QueuePresentKHR) \
    X(CreateRenderPass) X(DestroyRenderPass) X(CreateFramebuffer) X(DestroyFramebuffer) \
    X(CreateDescriptorSetLayout) X(DestroyDescriptorSetLayout) X(CreatePipelineLayout) X(DestroyPipelineLayout) \
    X(CreateShaderModule) X(DestroyShaderModule) X(CreateGraphicsPipelines) X(DestroyPipeline) \
    X(CreateDescriptorPool) X(DestroyDescriptorPool) X(AllocateDescriptorSets) X(UpdateDescriptorSets) \
    X(CreateQueryPool) X(DestroyQueryPool) X(GetQueryPoolResults) \
    X(CmdPipelineBarrier) X(CmdCopyBufferToImage) X(CmdCopyImageToBuffer) X(CmdBeginRenderPass) X(CmdEndRenderPass) \
    X(CmdSetViewport) X(CmdSetScissor) X(CmdBindPipeline) X(CmdBindVertexBuffers) X(CmdBindDescriptorSets) \
    X(CmdDraw) X(CmdResetQueryPool) X(CmdWriteTimestamp)
#define DECLARE(name) PFN_vk##name name{};
    DEVICE_PROCEDURES(DECLARE)
#undef DECLARE

    struct Buffer {
        Native* n{};
        VkBuffer buffer{};
        VkDeviceMemory memory{};
        VkDeviceSize size{};
        bool coherent{};
        VkDeviceSize allocated{};
        VulkanDiagnosticAccess::AllocationStats* observation{};
        ~Buffer() {
            if (buffer) n->Observe(ObservedCall::BufferDestroy, [&] { n->DestroyBuffer(n->device, buffer, nullptr); });
            if (memory) n->Observe(ObservedCall::BufferFree, [&] { n->FreeMemory(n->device, memory, nullptr); });
            if (observation) { ++observation->releases; observation->live_bytes -= allocated; }
            if (allocated && n->diagnostic) { ++n->diagnostic->buffers_released; n->diagnostic->live_bytes -= allocated; }
        }
        void Write(const void* bytes, VkDeviceSize count, VkDeviceSize offset = 0) {
            void* mapped{};
            Check(n->Observe(ObservedCall::MapWrite, [&] { return n->MapMemory(n->device, memory, 0, VK_WHOLE_SIZE, 0, &mapped); }), "MapBuffer");
            std::memcpy(static_cast<std::byte*>(mapped) + offset, bytes, static_cast<size_t>(count));
            VkResult result = VK_SUCCESS;
            if (!coherent) {
                VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
                range.memory = memory; range.size = VK_WHOLE_SIZE;
                result = n->Observe(ObservedCall::FlushWrite, [&] { return n->FlushMappedMemoryRanges(n->device, 1, &range); });
            }
            n->Observe(ObservedCall::UnmapWrite, [&] { n->UnmapMemory(n->device, memory); });
            Check(result, "FlushBuffer");
        }
        void Read(void* bytes, VkDeviceSize count) {
            void* mapped{};
            Check(n->MapMemory(n->device, memory, 0, VK_WHOLE_SIZE, 0, &mapped), "MapReadback", true);
            VkResult result = VK_SUCCESS;
            if (!coherent) {
                VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
                range.memory = memory; range.size = VK_WHOLE_SIZE;
                result = n->InvalidateMappedMemoryRanges(n->device, 1, &range);
            }
            if (result == VK_SUCCESS) std::memcpy(bytes, mapped, static_cast<size_t>(count));
            n->UnmapMemory(n->device, memory);
            Check(result, "InvalidateReadback", true);
        }
    };
    struct Image {
        Native* n{};
        VkImage image{};
        VkImageView view{};
        VkDeviceMemory memory{};
        VkDeviceSize allocated{};
        ~Image() {
            if (view) n->DestroyImageView(n->device, view, nullptr);
            if (image) n->Observe(ObservedCall::ImageDestroy, [&] { n->DestroyImage(n->device, image, nullptr); });
            if (memory) n->Observe(ObservedCall::ImageFree, [&] { n->FreeMemory(n->device, memory, nullptr); });
            if (allocated && n->diagnostic) { ++n->diagnostic->images_released; n->diagnostic->live_bytes -= allocated; }
        }
    };
    struct Mesh {
        std::shared_ptr<Buffer> buffer;
        uint32_t vertices{};
        std::array<double, 3> maximum{};
    };
    struct FrameBindings {
        Native* n{};
        VkDescriptorPool pool{};
        std::shared_ptr<Buffer> uniforms;
        VkDeviceSize stride{};
        std::vector<VkDescriptorSet> sets;
        std::vector<std::byte> packed;
        FrameBindingCache cache;
        VulkanDiagnosticAccess::BindingCounters* observed{};
        ~FrameBindings() {
            if (pool) {
                n->Observe(ObservedCall::PoolDestroy, [&] { n->DestroyDescriptorPool(n->device, pool, nullptr); });
                if (observed) ++observed->pools_released;
            }
        }
    };
    struct Frame {
        VkCommandPool pool{};
        VkCommandBuffer command{};
        VkFence fence{};
        VkSemaphore acquire{};
        VkQueryPool query{};
        bool pending{}, query_pending{};
        uint64_t frame_id{};
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Image>> textures;
        std::shared_ptr<Buffer> selection, readback;
        std::unique_ptr<FrameBindings> bindings, selection_binding;
    };
    struct SwapImage {
        VkImage image{};
        VkImageView view{};
        VkFramebuffer framebuffer{};
        VkSemaphore rendered{};
        VkFence presented{};
        bool pending{};
        std::shared_ptr<Image> color, depth;
    };
    std::array<Frame, 3> frames{};
    size_t next_frame{};
    uint32_t diagnostic_present_index = UINT32_MAX;
    Frame upload{};
    std::shared_ptr<Buffer> staging;
    std::shared_ptr<Image> uploading;
    VkSwapchainKHR swapchain{};
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
    VkPresentModeKHR mode{VK_PRESENT_MODE_FIFO_KHR};
    VkExtent2D extent{};
    std::vector<SwapImage> images;
    VkRenderPass render_pass{};
    VkDescriptorSetLayout set_layout{};
    VkPipelineLayout pipeline_layout{};
    VkPipeline blocks{}, selection{};
    VkSampler sampler{};
    std::map<BackendResourceId, std::shared_ptr<Mesh>, IdLess> meshes;
    std::map<BackendResourceId, std::shared_ptr<Image>, IdLess> textures;
    std::array<GpuFrameSample, 256> ready{};
    size_t ready_begin{}, ready_count{};

    Deadline Limit() const { return Clock::now() + timeout; }
    VkResult Allocate(const VkMemoryAllocateInfo& info, VkDeviceMemory& allocation, ObservedCall call) {
        if (diagnostic && diagnostic->next_allocation != VK_SUCCESS)
            return std::exchange(diagnostic->next_allocation, VK_SUCCESS);
        return Observe(call, [&] { return AllocateMemory(device, &info, nullptr, &allocation); });
    }
    void CountAllocation(VkDeviceSize bytes, bool image) {
        if (!diagnostic) return;
        if (image) ++diagnostic->images_created; else ++diagnostic->buffers_created;
        diagnostic->live_bytes += bytes;
        diagnostic->peak_bytes = std::max(diagnostic->peak_bytes, diagnostic->live_bytes);
    }
    void Wait(VkFence fence, Deadline deadline, const char* operation, ObservedCall call) {
        if (diagnostic) ++diagnostic->wait_calls;
        const auto result = Observe(call, [&] { return WaitForFences(device, 1, &fence, VK_TRUE, Remaining(deadline)); });
        if (result != VK_SUCCESS) unsafe = true;
        Check(result, operation, true);
    }
    uint32_t MemoryType(uint32_t bits, VkMemoryPropertyFlags flags) {
        for (uint32_t i = 0; i < memory.memoryTypeCount; ++i)
            if ((bits & (1u << i)) && (memory.memoryTypes[i].propertyFlags & flags) == flags) return i;
        Fail(RenderErrorCategory::Unsupported, "MemoryType", "required memory type unavailable");
    }
    std::shared_ptr<Buffer> MakeBuffer(VkDeviceSize size, VkBufferUsageFlags usage, BufferRole role) {
        auto result = std::make_shared<Buffer>(); result->n = this; result->size = size;
        VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create.size = size; create.usage = usage; create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Check(Observe(ObservedCall::BufferCreate, [&] { return CreateBuffer(device, &create, nullptr, &result->buffer); }), "CreateBuffer");
        VkMemoryRequirements requirements{}; GetBufferMemoryRequirements(device, result->buffer, &requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate.allocationSize = requirements.size;
        const auto memory_type = HostVisibleBufferMemoryType(memory, requirements.memoryTypeBits, role == BufferRole::Mesh);
        if (!memory_type) Fail(RenderErrorCategory::Unsupported, "MemoryType", "required memory type unavailable");
        allocate.memoryTypeIndex = *memory_type;
        result->coherent = (memory.memoryTypes[allocate.memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
        Check(Allocate(allocate, result->memory, ObservedCall::BufferAllocate), "AllocateBufferMemory");
        result->allocated = requirements.size; CountAllocation(requirements.size, false);
        if (diagnostic && diagnostic->observe_performance) {
            result->observation = &diagnostic->buffer_allocations[unsigned(role)][allocate.memoryTypeIndex];
            result->observation->Allocate(size, requirements.size, requirements.memoryTypeBits);
        }
        Check(Observe(ObservedCall::BufferBind, [&] { return BindBufferMemory(device, result->buffer, result->memory, 0); }), "BindBufferMemory");
        if (role == BufferRole::Mesh && !(reported_mesh_memory_types & (1u << *memory_type))) {
            const auto& selected = memory.memoryTypes[*memory_type];
            const bool local = (selected.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0;
            std::fprintf(stderr, "[renderer] Vulkan mesh memory: policy=prefer-device-local-host-visible; type=%u; flags=0x%x; heap=%u; host-visible-fallback=%s\n",
                         *memory_type, selected.propertyFlags, selected.heapIndex, local ? "false" : "true");
            reported_mesh_memory_types |= 1u << *memory_type;
        }
        if (result->observation) {
            ++result->observation->binds;
            if (role == BufferRole::Mesh) {
                ++diagnostic->mesh_bound_buffers; diagnostic->mesh_bound_bytes += size;
                if (memory.memoryTypes[allocate.memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                    ++diagnostic->mesh_device_local_buffers; diagnostic->mesh_device_local_bytes += size;
                }
            }
        }
        return result;
    }
    std::unique_ptr<FrameBindings> MakeFrameBindings(size_t capacity, VkDeviceSize stride, bool selection = false) {
        auto result = std::make_unique<FrameBindings>();
        result->n = this; result->stride = stride;
        if (diagnostic) {
            auto& slot = diagnostic->binding_slots.at(next_frame);
            result->observed = selection ? &slot.selection : &slot.mesh;
        }
        result->cache = FrameBindingCache(capacity);
        result->sets.resize(capacity);
        result->packed.resize(static_cast<size_t>(stride * capacity));
        const auto count = static_cast<uint32_t>(capacity);
        std::vector<VkDescriptorSetLayout> layouts(capacity, set_layout);
        DiagnosticPhase(FramePhase::UniformAllocate);
        result->uniforms = MakeBuffer(stride * capacity, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            selection ? BufferRole::SelectionUniform : BufferRole::MeshUniform);
        if (result->observed) ++result->observed->uniform_allocations;
        DiagnosticPhase(FramePhase::DescriptorAllocate);
        VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,count},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,count},{VK_DESCRIPTOR_TYPE_SAMPLER,count}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = count; pool.poolSizeCount = 3; pool.pPoolSizes = sizes;
        Check(Observe(ObservedCall::PoolCreate, [&] { return CreateDescriptorPool(device, &pool, nullptr, &result->pool); }), "CreateFrameDescriptorPool");
        if (result->observed) ++result->observed->pools_created;
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocate.descriptorPool = result->pool; allocate.descriptorSetCount = count; allocate.pSetLayouts = layouts.data();
        Check(Observe(ObservedCall::SetsAllocate, [&] { return AllocateDescriptorSets(device, &allocate, result->sets.data()); }), "AllocateFrameDescriptors");
        if (result->observed) {
            result->observed->sets_allocated += count;
            result->observed->capacity = capacity;
        }
        return result;
    }
    void WriteFrameDescriptors(FrameBindings& bindings, size_t index, FrameBindingCache::Writes dirty, const Image* image) {
        VkDescriptorBufferInfo buffer{bindings.uniforms->buffer, bindings.stride * index, 64};
        VkDescriptorImageInfo texture{}, sample{}; sample.sampler = sampler;
        if (image) { texture.imageView = image->view; texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; }
        std::array<VkWriteDescriptorSet,3> writes{};
        uint32_t count = 0;
        auto append = [&](uint32_t binding, VkDescriptorType type) -> VkWriteDescriptorSet& {
            auto& write = writes[count++];
            write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}; write.dstSet = bindings.sets[index];
            write.dstBinding = binding; write.descriptorCount = 1; write.descriptorType = type;
            return write;
        };
        if (dirty.uniform) append(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER).pBufferInfo = &buffer;
        if (dirty.texture) append(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE).pImageInfo = &texture;
        if (dirty.sampler) append(2, VK_DESCRIPTOR_TYPE_SAMPLER).pImageInfo = &sample;
        Observe(ObservedCall::DescriptorUpdate, [&] { UpdateDescriptorSets(device, count, writes.data(), 0, nullptr); });
        if (bindings.observed) {
            ++bindings.observed->update_calls;
            bindings.observed->uniform_descriptors += dirty.uniform;
            bindings.observed->image_descriptors += dirty.texture;
            bindings.observed->sampler_descriptors += dirty.sampler;
        }
    }
    std::shared_ptr<Image> MakeImage(uint32_t width, uint32_t height, uint32_t layers, VkFormat image_format,
                                    VkImageUsageFlags usage, VkSampleCountFlagBits count, VkImageAspectFlags aspect) {
        auto result = std::make_shared<Image>(); result->n = this;
        VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        create.imageType = VK_IMAGE_TYPE_2D; create.format = image_format;
        create.extent = {width, height, 1}; create.mipLevels = 1; create.arrayLayers = layers;
        create.samples = count; create.tiling = VK_IMAGE_TILING_OPTIMAL; create.usage = usage;
        Check(Observe(ObservedCall::ImageCreate, [&] { return CreateImage(device, &create, nullptr, &result->image); }), "CreateImage");
        VkMemoryRequirements requirements{}; GetImageMemoryRequirements(device, result->image, &requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate.allocationSize = requirements.size;
        allocate.memoryTypeIndex = MemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        Check(Allocate(allocate, result->memory, ObservedCall::ImageAllocate), "AllocateImageMemory");
        result->allocated = requirements.size; CountAllocation(requirements.size, true);
        Check(Observe(ObservedCall::ImageBind, [&] { return BindImageMemory(device, result->image, result->memory, 0); }), "BindImageMemory");
        result->view = MakeView(result->image, image_format, layers, aspect, (usage & VK_IMAGE_USAGE_SAMPLED_BIT) != 0);
        return result;
    }
    VkImageView MakeView(VkImage image, VkFormat image_format, uint32_t layers, VkImageAspectFlags aspect, bool array) {
        VkImageViewCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        create.image = image; create.format = image_format;
        create.viewType = array ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        create.subresourceRange = {aspect, 0, 1, 0, layers};
        VkImageView result{}; Check(CreateImageView(device, &create, nullptr, &result), "CreateImageView"); return result;
    }
    void CreateFrame(Frame& frame, bool queries) {
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO}; pool.queueFamilyIndex = graphics_family;
        Check(CreateCommandPool(device, &pool, nullptr, &frame.pool), "CreateCommandPool");
        VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocate.commandPool = frame.pool; allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; allocate.commandBufferCount = 1;
        Check(AllocateCommandBuffers(device, &allocate, &frame.command), "AllocateCommandBuffer");
        VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        Check(CreateFence(device, &fence, nullptr, &frame.fence), "CreateSubmitFence");
        VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        Check(CreateSemaphore(device, &semaphore, nullptr, &frame.acquire), "CreateAcquireSemaphore");
        if (queries && timestamp_bits) {
            VkQueryPoolCreateInfo query{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO}; query.queryType = VK_QUERY_TYPE_TIMESTAMP; query.queryCount = 2;
            Check(CreateQueryPool(device, &query, nullptr, &frame.query), "CreateTimestampQuery");
        }
    }
    void Begin(Frame& frame, bool reset = true) {
        if (reset) Check(ResetCommandPool(device, frame.pool, 0), "ResetCommandPool");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO}; begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        Check(BeginCommandBuffer(frame.command, &begin), "BeginCommandBuffer");
    }
    void Submit(Frame& frame, const VkSubmitInfo& submit) {
        Check(ResetFences(device, 1, &frame.fence), "ResetSubmitFence");
        // Failed submission is not evidence that the driver stopped referencing this owner.
        unsafe = true;
        if (diagnostic) ++diagnostic->submit_attempts;
        auto actual = submit;
        std::array<VkSemaphore, 2> waits{};
        std::array<VkPipelineStageFlags, 2> stages{};
        std::array<uint64_t, 2> values{};
        const uint64_t binary_signal_value = 0;
        VkTimelineSemaphoreSubmitInfo timeline{VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO};
        if (diagnostic && diagnostic->next_gate) {
            if (!diagnostic->enable_timeline_gate || submit.pNext || submit.waitSemaphoreCount > 1 || submit.signalSemaphoreCount > 1)
                Fail(RenderErrorCategory::InvalidState, "DiagnosticGate", "unexpected production submit shape");
            if (submit.waitSemaphoreCount) { waits[0] = submit.pWaitSemaphores[0]; stages[0] = submit.pWaitDstStageMask[0]; }
            waits[submit.waitSemaphoreCount] = std::exchange(diagnostic->next_gate, {});
            stages[submit.waitSemaphoreCount] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            values[submit.waitSemaphoreCount] = 1;
            actual.waitSemaphoreCount++; actual.pWaitSemaphores = waits.data(); actual.pWaitDstStageMask = stages.data();
            timeline.waitSemaphoreValueCount = actual.waitSemaphoreCount; timeline.pWaitSemaphoreValues = values.data();
            timeline.signalSemaphoreValueCount = submit.signalSemaphoreCount; timeline.pSignalSemaphoreValues = &binary_signal_value;
            actual.pNext = &timeline;
        }
        const auto injected = diagnostic ? std::exchange(diagnostic->next_submit, VK_SUCCESS) : VK_SUCCESS;
        Check(injected == VK_SUCCESS ? Observe(ObservedCall::QueueSubmit, [&] { return QueueSubmit(graphics, 1, &actual, frame.fence); }) : injected, "QueueSubmit", true);
        frame.pending = true; unsafe = false;
        if (diagnostic) { ++diagnostic->successful_submits; diagnostic->last_submit = frame.fence; }
    }
    void Ready(GpuFrameSample sample) noexcept {
        if (ready_count == ready.size()) {
            std::fprintf(stderr, "[renderer] Vulkan GPU sample discarded: frame=%llu\n",
                         static_cast<unsigned long long>(ready[ready_begin].frame_id));
            ready_begin = (ready_begin + 1) % ready.size(); --ready_count;
        }
        ready[(ready_begin + ready_count++) % ready.size()] = sample;
    }
    void Collect(Frame& frame) {
        if (!frame.query_pending) return;
        std::array<uint64_t, 2> values{};
        auto result = Observe(ObservedCall::QueryRead, [&] { return GetQueryPoolResults(device, frame.query, 0, 2, sizeof(values), values.data(),
                                               sizeof(uint64_t), VK_QUERY_RESULT_64_BIT); });
        if (diagnostic) {
            ++diagnostic->query_calls;
            if (result == VK_SUCCESS && diagnostic->next_query != VK_SUCCESS)
                result = std::exchange(diagnostic->next_query, VK_SUCCESS);
        }
        if (result == VK_NOT_READY) return;
        if (result != VK_SUCCESS) { unsafe = true; Check(result, "ReadTimestampQuery", true); }
        const uint64_t mask = timestamp_bits == 64 ? UINT64_MAX : (uint64_t{1} << timestamp_bits) - 1;
        const auto ticks = (values[1] - values[0]) & mask;
        const double ns = static_cast<double>(ticks) * properties.limits.timestampPeriod;
        if (!std::isfinite(ns) || ns > static_cast<double>(INT64_MAX))
            Fail(RenderErrorCategory::SubmissionFailure, "Timestamp", "timestamp duration is not representable");
        Ready({frame.frame_id, true, std::chrono::nanoseconds(static_cast<int64_t>(ns)), GpuSampleMissingReason::None});
        frame.query_pending = false;
    }
    void Finish(Frame& frame, Deadline deadline) {
        if (!frame.pending) return;
        Wait(frame.fence, deadline, "WaitSubmit", &frame == &upload ? ObservedCall::WaitUpload : ObservedCall::WaitSubmit);
        frame.pending = false; Collect(frame);
    }
    void Drain(Deadline deadline) {
        Finish(upload, deadline);
        for (auto& frame : frames) Finish(frame, deadline);
        for (auto& image : images) if (image.pending) {
            Wait(image.presented, deadline, "WaitPresent", ObservedCall::WaitDrainPresent); image.pending = false;
        }
    }
    void FinishDiagnosticPresent() {
        if (diagnostic_present_index == UINT32_MAX) return;
        if (diagnostic->quarantined)
            Fail(RenderErrorCategory::InvalidState, "DiagnosticPresent", "terminal owner cannot resume presentation");
        auto& image = images.at(diagnostic_present_index);
        Check(GetFenceStatus(device, diagnostic->last_submit), "DiagnosticCompletedSubmit", true);
        Check(ResetFences(device, 1, &image.presented), "DiagnosticResetPresent", true);
        VkSwapchainPresentFenceInfoKHR retirement{VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR};
        if (!khr_maintenance) retirement.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
        retirement.swapchainCount = 1; retirement.pFences = &image.presented;
        VkPresentInfoKHR present_info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR}; present_info.pNext = &retirement;
        present_info.waitSemaphoreCount = 1; present_info.pWaitSemaphores = &image.rendered;
        present_info.swapchainCount = 1; present_info.pSwapchains = &swapchain; present_info.pImageIndices = &diagnostic_present_index;
        Check(QueuePresentKHR(present, &present_info), "DiagnosticDeferredPresent", true);
        ++diagnostic->present_calls; image.pending = true;
        diagnostic_present_index = UINT32_MAX; diagnostic->deferred_present_pending = false;
        unsafe = false;
    }
    void RetireFrame(Frame& frame) noexcept {
        frame.meshes.clear(); frame.textures.clear(); frame.readback.reset();
    }
    void DropFrame(Frame& frame) noexcept {
        RetireFrame(frame); frame.selection.reset(); frame.bindings.reset(); frame.selection_binding.reset();
    }
    void DestroyFrame(Frame& frame) noexcept {
        DropFrame(frame);
        if (frame.query) DestroyQueryPool(device, frame.query, nullptr);
        if (frame.acquire) DestroySemaphore(device, frame.acquire, nullptr);
        if (frame.fence) DestroyFence(device, frame.fence, nullptr);
        if (frame.pool) DestroyCommandPool(device, frame.pool, nullptr);
        frame = {};
    }
    void DestroySwapchain() noexcept {
        for (auto& image : images) {
            if (image.framebuffer) DestroyFramebuffer(device, image.framebuffer, nullptr);
            if (image.view) DestroyImageView(device, image.view, nullptr);
            if (image.rendered) DestroySemaphore(device, image.rendered, nullptr);
            if (image.presented) DestroyFence(device, image.presented, nullptr);
            image.color.reset(); image.depth.reset();
        }
        images.clear();
        if (blocks) DestroyPipeline(device, std::exchange(blocks, {}), nullptr);
        if (selection) DestroyPipeline(device, std::exchange(selection, {}), nullptr);
        if (render_pass) DestroyRenderPass(device, std::exchange(render_pass, {}), nullptr);
        if (swapchain) DestroySwapchainKHR(device, std::exchange(swapchain, {}), nullptr);
    }
    ~Native() {
        if (diagnostic) diagnostic->finish_deferred_present = {};
        if (device) {
            DestroySwapchain();
            for (auto& frame : frames) DestroyFrame(frame);
            DestroyFrame(upload); staging.reset(); uploading.reset(); meshes.clear(); textures.clear();
            if (sampler) DestroySampler(device, sampler, nullptr);
            if (pipeline_layout) DestroyPipelineLayout(device, pipeline_layout, nullptr);
            if (set_layout) DestroyDescriptorSetLayout(device, set_layout, nullptr);
            DestroyDevice(device, nullptr);
        }
        if (surface) DestroySurfaceKHR(instance, surface, nullptr);
        if (messenger) DestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
        if (instance) DestroyInstance(instance, nullptr);
        std::fprintf(stderr, "[renderer] Vulkan shutdown: validation_warnings=%u; validation_errors=%u; native_retired=true\n",
                     warnings.load(), errors.load());
        if (loader) FreeLibrary(loader);
        if (diagnostic) {
            diagnostic->warnings = warnings.load(); diagnostic->errors = errors.load();
            diagnostic->native_destroyed = true; diagnostic->loader_released = true;
        }
    }
};

VKAPI_ATTR VkBool32 VKAPI_CALL Diagnostic(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user) {
    auto& n = *static_cast<Native*>(user);
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) ++n.errors;
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ++n.warnings;
    if (n.diagnostic) { n.diagnostic->warnings = n.warnings.load(); n.diagnostic->errors = n.errors.load(); }
    std::fprintf(stderr, "[renderer] Vulkan validation: %s\n", data && data->pMessage ? data->pMessage : "no message");
    return VK_FALSE;
}

std::string Version(uint32_t version) {
    return std::to_string(VK_API_VERSION_MAJOR(version)) + "." + std::to_string(VK_API_VERSION_MINOR(version)) + "." +
        std::to_string(VK_API_VERSION_PATCH(version));
}
template<class Property> bool Has(const std::vector<Property>& entries, const char* name) {
    return std::any_of(entries.begin(), entries.end(), [name](const Property& p) {
        if constexpr (std::is_same_v<Property, VkExtensionProperties>) return std::strcmp(p.extensionName, name) == 0;
        else return std::strcmp(p.layerName, name) == 0;
    });
}
void Initialize(Native& n, Window& window, const RendererConfig& config, RendererInfo& info) {
    n.timeout = config.wait_timeout;
    n.get = Platform::VulkanBridge::GetInstanceProcedureAddress(window);
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                           reinterpret_cast<LPCWSTR>(n.get), &n.loader))
        Fail(RenderErrorCategory::InitializationFailure, "RetainLoader", "cannot retain the actual SDL-associated loader module");
    if (n.diagnostic) n.diagnostic->loader_address = reinterpret_cast<std::uintptr_t>(n.loader);
    auto required = Platform::VulkanBridge::GetRequiredExtensions(window);
    const auto enumerate_extensions = Procedure<PFN_vkEnumerateInstanceExtensionProperties>(n.get, {}, "vkEnumerateInstanceExtensionProperties");
    const auto enumerate_layers = Procedure<PFN_vkEnumerateInstanceLayerProperties>(n.get, {}, "vkEnumerateInstanceLayerProperties");
    uint32_t count{}; Check(enumerate_extensions(nullptr, &count, nullptr), "InstanceExtensions");
    std::vector<VkExtensionProperties> extensions(count);
    Check(enumerate_extensions(nullptr, &count, extensions.data()), "InstanceExtensions");
    required.emplace_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
    n.instance_khr_maintenance = Has(extensions, VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
    n.instance_ext_maintenance = Has(extensions, VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
    if (n.instance_khr_maintenance) required.emplace_back(VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
    if (n.instance_ext_maintenance) required.emplace_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
    if (!n.instance_khr_maintenance && !n.instance_ext_maintenance)
        Fail(RenderErrorCategory::Unsupported, "InstanceExtensions", "KHR/EXT surface_maintenance1 dependency unavailable");
    std::vector<const char*> layers;
    if (config.enable_validation) {
        Check(enumerate_layers(&count, nullptr), "InstanceLayers");
        std::vector<VkLayerProperties> available(count); Check(enumerate_layers(&count, available.data()), "InstanceLayers");
        if (!Has(available, "VK_LAYER_KHRONOS_validation") || !Has(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            Fail(RenderErrorCategory::Unsupported, "Validation", "requested validation layer/debug utils unavailable");
        layers.push_back("VK_LAYER_KHRONOS_validation"); required.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        Check(enumerate_extensions(layers[0], &count, nullptr), "ValidationExtensions");
        std::vector<VkExtensionProperties> layer_extensions(count);
        Check(enumerate_extensions(layers[0], &count, layer_extensions.data()), "ValidationExtensions");
        if (!Has(layer_extensions, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME) && !Has(extensions, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME))
            Fail(RenderErrorCategory::Unsupported, "Validation", "synchronization validation configuration unavailable");
        required.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    }
    std::vector<const char*> names; for (auto& name : required) names.push_back(name.c_str());
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO}; app.pApplicationName = "SymoCraft"; app.apiVersion = VK_API_VERSION_1_2;
    VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug.pfnUserCallback = Diagnostic; debug.pUserData = &n;
    const VkValidationFeatureEnableEXT sync = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    VkValidationFeaturesEXT validation{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    validation.enabledValidationFeatureCount = 1; validation.pEnabledValidationFeatures = &sync; validation.pNext = &debug;
    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO}; create.pApplicationInfo = &app;
    create.enabledExtensionCount = static_cast<uint32_t>(names.size()); create.ppEnabledExtensionNames = names.data();
    create.enabledLayerCount = static_cast<uint32_t>(layers.size()); create.ppEnabledLayerNames = layers.data();
    create.pNext = config.enable_validation ? &validation : nullptr;
    const auto create_instance = Procedure<PFN_vkCreateInstance>(n.get, {}, "vkCreateInstance");
    Check(create_instance(&create, nullptr, &n.instance), "CreateInstance");
    n.DestroyInstance = Procedure<PFN_vkDestroyInstance>(n.get, n.instance, "vkDestroyInstance");
    n.DestroySurfaceKHR = Procedure<PFN_vkDestroySurfaceKHR>(n.get, n.instance, "vkDestroySurfaceKHR");
    if (config.enable_validation) {
        n.DestroyDebugUtilsMessengerEXT = Procedure<PFN_vkDestroyDebugUtilsMessengerEXT>(n.get, n.instance, "vkDestroyDebugUtilsMessengerEXT");
        Check(Procedure<PFN_vkCreateDebugUtilsMessengerEXT>(n.get, n.instance, "vkCreateDebugUtilsMessengerEXT")
            (n.instance, &debug, nullptr, &n.messenger), "CreateDebugMessenger");
    }
    n.surface = Platform::VulkanBridge::CreateSurface(window, n.instance);
#define INSTANCE(name) n.name = Procedure<PFN_vk##name>(n.get, n.instance, "vk" #name)
    INSTANCE(GetPhysicalDeviceSurfaceCapabilitiesKHR); INSTANCE(GetPhysicalDeviceSurfaceFormatsKHR);
    INSTANCE(GetPhysicalDeviceSurfacePresentModesKHR); INSTANCE(GetPhysicalDeviceImageFormatProperties); INSTANCE(GetDeviceProcAddr);
#undef INSTANCE
    const auto enumerate_devices = Procedure<PFN_vkEnumeratePhysicalDevices>(n.get, n.instance, "vkEnumeratePhysicalDevices");
    const auto get_properties = Procedure<PFN_vkGetPhysicalDeviceProperties2>(n.get, n.instance, "vkGetPhysicalDeviceProperties2");
    const auto get_features = Procedure<PFN_vkGetPhysicalDeviceFeatures2>(n.get, n.instance, "vkGetPhysicalDeviceFeatures2");
    const auto get_memory = Procedure<PFN_vkGetPhysicalDeviceMemoryProperties>(n.get, n.instance, "vkGetPhysicalDeviceMemoryProperties");
    const auto get_queues = Procedure<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(n.get, n.instance, "vkGetPhysicalDeviceQueueFamilyProperties");
    const auto get_present = Procedure<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(n.get, n.instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    const auto get_extensions = Procedure<PFN_vkEnumerateDeviceExtensionProperties>(n.get, n.instance, "vkEnumerateDeviceExtensionProperties");
    Check(enumerate_devices(n.instance, &count, nullptr), "EnumerateDevices");
    std::vector<VkPhysicalDevice> devices(count); Check(enumerate_devices(n.instance, &count, devices.data()), "EnumerateDevices");
    for (const auto candidate : devices) {
        VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2}; get_properties(candidate, &properties);
        if (properties.properties.apiVersion < VK_API_VERSION_1_2 ||
            (properties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
             properties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)) continue;
        Check(get_extensions(candidate, nullptr, &count, nullptr), "DeviceExtensions");
        std::vector<VkExtensionProperties> available(count); Check(get_extensions(candidate, nullptr, &count, available.data()), "DeviceExtensions");
        if (!Has(available, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;
        if (n.diagnostic && n.diagnostic->hide_maintenance_extension) {
            ++n.diagnostic->extension_rejections; continue;
        }
        const bool khr = n.instance_khr_maintenance && Has(available, VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME);
        if (!khr && !(n.instance_ext_maintenance && Has(available, VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME))) continue;
        VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR maintenance{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_KHR};
        if (!khr) maintenance.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
        VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2}; features.pNext = &maintenance; get_features(candidate, &features);
        if (n.diagnostic && n.diagnostic->hide_maintenance_feature) {
            ++n.diagnostic->feature_rejections; maintenance.swapchainMaintenance1 = VK_FALSE;
        }
        if (!maintenance.swapchainMaintenance1) continue;
        get_queues(candidate, &count, nullptr); std::vector<VkQueueFamilyProperties> queues(count); get_queues(candidate, &count, queues.data());
        uint32_t graphics = UINT32_MAX, present = UINT32_MAX;
        for (uint32_t i = 0; i < count; ++i) {
            VkBool32 supported{}; Check(get_present(candidate, i, n.surface, &supported), "SurfaceSupport");
            if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && graphics == UINT32_MAX) graphics = i;
            if (supported && present == UINT32_MAX) present = i;
            if (supported && (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) { graphics = present = i; break; }
        }
        if (graphics == UINT32_MAX || present == UINT32_MAX) continue;
        n.physical = candidate; n.graphics_family = graphics; n.present_family = present; n.khr_maintenance = khr;
        n.timestamp_bits = queues[graphics].timestampValidBits; break;
    }
    if (!n.physical) Fail(RenderErrorCategory::Unsupported, "SelectDevice", "hardware Vulkan 1.2 + surface/swapchain + KHR/EXT swapchainMaintenance1 required; no software fallback");
    VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    properties.pNext = &n.identity; n.identity.pNext = &n.driver; get_properties(n.physical, &properties); n.properties = properties.properties;
    get_memory(n.physical, &n.memory);
    if (n.diagnostic && n.diagnostic->observe_performance) {
        n.diagnostic->memory_properties = n.memory;
        n.diagnostic->graphics_family = n.graphics_family; n.diagnostic->present_family = n.present_family;
        n.diagnostic->timestamp_bits = n.timestamp_bits; n.diagnostic->timestamp_period = n.properties.limits.timestampPeriod;
    }
    if (!n.properties.limits.timestampComputeAndGraphics) n.timestamp_bits = 0;
    if (n.diagnostic && n.diagnostic->disable_timestamps) n.timestamp_bits = 0;
    const float priority = 1;
    std::array<VkDeviceQueueCreateInfo, 2> queues{};
    queues[0] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO}; queues[0].queueFamilyIndex = n.graphics_family;
    queues[0].queueCount = 1; queues[0].pQueuePriorities = &priority;
    queues[1] = queues[0]; queues[1].queueFamilyIndex = n.present_family;
    const char* device_extensions[]{VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        n.khr_maintenance ? VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME : VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME};
    VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR maintenance{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_KHR};
    if (!n.khr_maintenance) maintenance.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
    maintenance.swapchainMaintenance1 = VK_TRUE;
    VkPhysicalDeviceTimelineSemaphoreFeatures timeline{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
    if (n.diagnostic && n.diagnostic->enable_timeline_gate) {
        VkPhysicalDeviceFeatures2 query{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2}; query.pNext = &timeline; get_features(n.physical, &query);
        if (!timeline.timelineSemaphore) Fail(RenderErrorCategory::Unsupported, "DiagnosticGate", "isolated timeline gate feature unavailable");
        maintenance.pNext = &timeline;
    }
    VkDeviceCreateInfo device{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO}; device.pNext = &maintenance;
    device.queueCreateInfoCount = n.graphics_family == n.present_family ? 1 : 2; device.pQueueCreateInfos = queues.data();
    device.enabledExtensionCount = 2; device.ppEnabledExtensionNames = device_extensions;
    Check(Procedure<PFN_vkCreateDevice>(n.get, n.instance, "vkCreateDevice")(n.physical, &device, nullptr, &n.device), "CreateDevice");
    if (n.diagnostic) ++n.diagnostic->device_creations;
#define LOAD(name) n.name = reinterpret_cast<PFN_vk##name>(n.GetDeviceProcAddr(n.device, "vk" #name)); \
    if (!n.name) Fail(RenderErrorCategory::Unsupported, "DeviceProcedure", "missing vk" #name);
    DEVICE_PROCEDURES(LOAD)
#undef LOAD
#undef DEVICE_PROCEDURES
    n.GetDeviceQueue(n.device, n.graphics_family, 0, &n.graphics); n.GetDeviceQueue(n.device, n.present_family, 0, &n.present);
    if (n.diagnostic && n.diagnostic->observe_performance)
        n.diagnostic->same_graphics_present_queue = n.graphics == n.present;
    n.samples = static_cast<VkSampleCountFlagBits>(config.requested_samples);
    if (!(n.properties.limits.framebufferColorSampleCounts & n.samples) || !(n.properties.limits.framebufferDepthSampleCounts & n.samples))
        Fail(RenderErrorCategory::Unsupported, "Samples", "requested sample count unavailable");
    for (auto& frame : n.frames) n.CreateFrame(frame, true);
    n.CreateFrame(n.upload, false);
    VkDescriptorSetLayoutBinding bindings[3]{};
    bindings[0] = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    bindings[1] = {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    bindings[2] = {2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo layout{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; layout.bindingCount = 3; layout.pBindings = bindings;
    Check(n.CreateDescriptorSetLayout(n.device, &layout, nullptr, &n.set_layout), "CreateSetLayout");
    VkPipelineLayoutCreateInfo pipeline{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO}; pipeline.setLayoutCount = 1; pipeline.pSetLayouts = &n.set_layout;
    Check(n.CreatePipelineLayout(n.device, &pipeline, nullptr, &n.pipeline_layout), "CreatePipelineLayout");
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO}; sampler.magFilter = sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST; sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    Check(n.CreateSampler(n.device, &sampler, nullptr, &n.sampler), "CreateSampler");
    info.backend = Backend::Vulkan; info.device_name = n.properties.deviceName;
    std::ostringstream id; id << "vendor=" << std::hex << n.properties.vendorID << ";device=" << n.properties.deviceID << ";uuid=";
    for (auto byte : n.identity.deviceUUID) { id.width(2); id.fill('0'); id << static_cast<unsigned>(byte); }
    info.device_identity = id.str(); info.driver_version = std::string(n.driver.driverName) + ":" + n.driver.driverInfo;
    info.api_version = Version(n.properties.apiVersion); info.actual_samples = config.requested_samples;
    info.validation_enabled = config.enable_validation; info.capabilities = {true, true, true, n.timestamp_bits != 0};
}

struct Shader {
    Native& n;
    VkShaderModule module{};
    Shader(Native& owner, const std::filesystem::path& path) : n(owner) {
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        const auto end = stream.tellg();
        if (!stream || end < 20 || static_cast<uint64_t>(end) % 4 != 0 || static_cast<uint64_t>(end) > 64 * 1024 * 1024)
            Fail(RenderErrorCategory::AssetUnavailable, "LoadShader", "invalid SPIR-V length: " + path.string());
        std::vector<uint32_t> words(static_cast<size_t>(end) / 4);
        stream.seekg(0); stream.read(reinterpret_cast<char*>(words.data()), static_cast<std::streamsize>(end));
        if (!stream || words[0] != 0x07230203)
            Fail(RenderErrorCategory::AssetUnavailable, "LoadShader", "invalid SPIR-V: " + path.string());
        VkShaderModuleCreateInfo create{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        create.codeSize = words.size() * sizeof(uint32_t); create.pCode = words.data();
        const auto result = n.CreateShaderModule(n.device, &create, nullptr, &module);
        if (result != VK_SUCCESS)
            Fail(result == VK_ERROR_DEVICE_LOST ? RenderErrorCategory::DeviceLost : RenderErrorCategory::AssetUnavailable,
                 "CreateShaderModule", path.string(), result);
    }
    ~Shader() { if (module) n.DestroyShaderModule(n.device, module, nullptr); }
};

void CreatePipelines(Native& n, const std::filesystem::path& root) {
    VkAttachmentDescription attachments[3]{};
    attachments[0].format = n.format; attachments[0].samples = n.samples;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[1] = attachments[0]; attachments[1].format = VK_FORMAT_D32_SFLOAT;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[2] = attachments[0]; attachments[2].samples = VK_SAMPLE_COUNT_1_BIT; attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, depth{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkAttachmentReference resolve{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{}; subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1; subpass.pColorAttachments = &color; subpass.pDepthStencilAttachment = &depth;
    subpass.pResolveAttachments = n.samples == VK_SAMPLE_COUNT_1_BIT ? nullptr : &resolve;
    VkSubpassDependency dependency{}; dependency.srcSubpass = VK_SUBPASS_EXTERNAL; dependency.dstSubpass = 0;
    dependency.srcStageMask = dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO}; pass.attachmentCount = n.samples == VK_SAMPLE_COUNT_1_BIT ? 2 : 3;
    pass.pAttachments = attachments; pass.subpassCount = 1; pass.pSubpasses = &subpass; pass.dependencyCount = 1; pass.pDependencies = &dependency;
    Check(n.CreateRenderPass(n.device, &pass, nullptr, &n.render_pass), "CreateRenderPass");
    for (const bool lines : {false, true}) {
        const auto name = lines ? "selection" : "blocks";
        Shader vs(n, root / (std::string(name) + ".vs.spv")), ps(n, root / (std::string(name) + ".ps.spv"));
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}; stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vs.module; stages[0].pName = "VSMain";
        stages[1] = stages[0]; stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module = ps.module; stages[1].pName = "PSMain";
        const VkVertexInputBindingDescription binding{0, lines ? 12u : 28u, VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attributes[3]{{0, 0, lines ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R32G32B32_SINT, 0},
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12}, {2, 0, VK_FORMAT_R32_SFLOAT, 24}};
        VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertex.vertexBindingDescriptionCount = 1; vertex.pVertexBindingDescriptions = &binding;
        vertex.vertexAttributeDescriptionCount = lines ? 1 : 2;
#ifdef SYMOCRAFT_VULKAN_DEBUG_ABI
        if (!lines) vertex.vertexAttributeDescriptionCount = 3;
#endif
        vertex.pVertexAttributeDescriptions = attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = lines ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO}; viewport.viewportCount = viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL; raster.cullMode = lines ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
        raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo msaa{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO}; msaa.rasterizationSamples = n.samples;
        VkPipelineDepthStencilStateCreateInfo depth_state{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth_state.depthTestEnable = VK_TRUE; depth_state.depthWriteEnable = VK_TRUE;
        depth_state.depthCompareOp = VK_COMPARE_OP_LESS;
        VkPipelineColorBlendAttachmentState blend{}; blend.colorWriteMask = 0xf;
        VkPipelineColorBlendStateCreateInfo color_blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        color_blend.attachmentCount = 1; color_blend.pAttachments = &blend;
        const VkDynamicState dynamics[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO}; dynamic.dynamicStateCount = 2; dynamic.pDynamicStates = dynamics;
        VkGraphicsPipelineCreateInfo create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO}; create.stageCount = 2; create.pStages = stages;
        create.pVertexInputState = &vertex; create.pInputAssemblyState = &assembly; create.pViewportState = &viewport;
        create.pRasterizationState = &raster; create.pMultisampleState = &msaa; create.pDepthStencilState = &depth_state;
        create.pColorBlendState = &color_blend; create.pDynamicState = &dynamic; create.layout = n.pipeline_layout; create.renderPass = n.render_pass;
        const auto injected = n.diagnostic ? std::exchange(n.diagnostic->next_pipeline, VK_SUCCESS) : VK_SUCCESS;
        Check(injected == VK_SUCCESS ? n.CreateGraphicsPipelines(n.device, {}, 1, &create, nullptr, lines ? &n.selection : &n.blocks)
                                    : injected, "CreateGraphicsPipeline");
    }
}

void Rebuild(Native& n, Extent2D extent, bool vsync, const std::filesystem::path& root, RendererInfo& info) {
    VkSurfaceCapabilitiesKHR capabilities{};
    Check(n.GetPhysicalDeviceSurfaceCapabilitiesKHR(n.physical, n.surface, &capabilities), "SurfaceCapabilities");
    VkExtent2D actual{extent.width, extent.height};
    if (!extent.IsSuspended()) {
        if (capabilities.currentExtent.width != UINT32_MAX) actual = capabilities.currentExtent;
        // Keep accepted info and native ownership intact until SDL/WSI facts agree.
        if (actual.width != extent.width || actual.height != extent.height) {
            n.rebuild = true;
            throw RenderError(RenderErrorCategory::InvalidState, "Vulkan", "Resize",
                "surface extent is still changing; poll window facts before retry", {}, "surface_extent_pending");
        }
        if (extent.width < capabilities.minImageExtent.width || extent.width > capabilities.maxImageExtent.width ||
            extent.height < capabilities.minImageExtent.height || extent.height > capabilities.maxImageExtent.height)
            Fail(RenderErrorCategory::Unsupported, "Resize", "requested extent exceeds actual surface limits");
    }
    uint32_t count{};
    Check(n.GetPhysicalDeviceSurfacePresentModesKHR(n.physical, n.surface, &count, nullptr), "PresentModes");
    std::vector<VkPresentModeKHR> modes(count);
    Check(n.GetPhysicalDeviceSurfacePresentModesKHR(n.physical, n.surface, &count, modes.data()), "PresentModes");
    const auto mode = vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    if (std::find(modes.begin(), modes.end(), mode) == modes.end())
        Fail(RenderErrorCategory::Unsupported, "SetVSync", "requested FIFO/Immediate mode unavailable; no fallback");
    Check(n.GetPhysicalDeviceSurfaceFormatsKHR(n.physical, n.surface, &count, nullptr), "SurfaceFormats");
    std::vector<VkSurfaceFormatKHR> formats(count);
    Check(n.GetPhysicalDeviceSurfaceFormatsKHR(n.physical, n.surface, &count, formats.data()), "SurfaceFormats");
    VkFormat format = VK_FORMAT_UNDEFINED;
    for (const auto candidate : {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM})
        for (const auto& available : formats)
            if ((available.format == candidate || available.format == VK_FORMAT_UNDEFINED) && available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                if (format == VK_FORMAT_UNDEFINED) format = candidate;
    if (format == VK_FORMAT_UNDEFINED || !(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT))
        Fail(RenderErrorCategory::Unsupported, "SurfaceFormat", "RGBA8/BGRA8 UNORM with screenshot transfer-source support required");
    for (const auto attachment : {format, VK_FORMAT_D32_SFLOAT}) {
        VkImageFormatProperties properties{};
        Check(n.GetPhysicalDeviceImageFormatProperties(n.physical, attachment, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
            attachment == format ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 0, &properties), "AttachmentFormat");
        if (!(properties.sampleCounts & n.samples)) Fail(RenderErrorCategory::Unsupported, "AttachmentSamples", "requested samples unsupported by actual color/depth format");
    }
    n.Drain(n.Limit());
    for (auto& frame : n.frames) { Check(n.ResetCommandPool(n.device, frame.pool, 0), "ResetRetiredCommands"); n.DropFrame(frame); }
    n.DestroySwapchain();
    n.format = format; n.mode = mode; n.extent = actual; n.rebuild = false;
    info.framebuffer_extent = extent;
    info.actual_present_mode = vsync ? PresentMode::Fifo : PresentMode::Immediate;
    info.framebuffer_format = format == VK_FORMAT_B8G8R8A8_UNORM ? "BGRA8_UNORM" : "RGBA8_UNORM";
    if (extent.IsSuspended()) return;
    try {
        VkSwapchainCreateInfoKHR create{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR}; create.surface = n.surface;
        create.minImageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount) create.minImageCount = std::min(create.minImageCount, capabilities.maxImageCount);
        create.imageFormat = format; create.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; create.imageExtent = actual;
        create.imageArrayLayers = 1; create.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        const uint32_t families[]{n.graphics_family, n.present_family};
        create.imageSharingMode = n.graphics_family == n.present_family ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
        if (create.imageSharingMode == VK_SHARING_MODE_CONCURRENT) { create.queueFamilyIndexCount = 2; create.pQueueFamilyIndices = families; }
        create.preTransform = capabilities.currentTransform; create.presentMode = mode; create.clipped = VK_FALSE;
        for (auto alpha : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                           VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
            if (capabilities.supportedCompositeAlpha & alpha) { create.compositeAlpha = alpha; break; }
        Check(n.CreateSwapchainKHR(n.device, &create, nullptr, &n.swapchain), "CreateSwapchain");
        if (n.diagnostic) ++n.diagnostic->swapchain_creations;
        Check(n.GetSwapchainImagesKHR(n.device, n.swapchain, &count, nullptr), "SwapchainImages");
        std::vector<VkImage> images(count); Check(n.GetSwapchainImagesKHR(n.device, n.swapchain, &count, images.data()), "SwapchainImages");
        n.images.resize(count); CreatePipelines(n, root);
        if (n.diagnostic && n.diagnostic->observe_performance) n.diagnostic->swapchain_images = count;
        for (size_t i = 0; i < n.images.size(); ++i) {
            auto& image = n.images[i]; image.image = images[i];
            image.view = n.MakeView(image.image, format, 1, VK_IMAGE_ASPECT_COLOR_BIT, false);
            image.depth = n.MakeImage(extent.width, extent.height, 1, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                      n.samples, VK_IMAGE_ASPECT_DEPTH_BIT);
            if (n.samples != VK_SAMPLE_COUNT_1_BIT)
                image.color = n.MakeImage(extent.width, extent.height, 1, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, n.samples, VK_IMAGE_ASPECT_COLOR_BIT);
            const VkImageView views[]{image.color ? image.color->view : image.view, image.depth->view, image.view};
            VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO}; framebuffer.renderPass = n.render_pass;
            framebuffer.attachmentCount = image.color ? 3 : 2; framebuffer.pAttachments = views;
            framebuffer.width = extent.width; framebuffer.height = extent.height; framebuffer.layers = 1;
            Check(n.CreateFramebuffer(n.device, &framebuffer, nullptr, &image.framebuffer), "CreateFramebuffer");
            VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            Check(n.CreateFence(n.device, &fence, nullptr, &image.presented), "CreatePresentFence");
            VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            Check(n.CreateSemaphore(n.device, &semaphore, nullptr, &image.rendered), "CreatePresentSemaphore");
        }
    } catch (const RenderError& error) {
        if (error.Category() == RenderErrorCategory::DeviceLost) throw;
        throw RenderError(RenderErrorCategory::InitializationFailure, "Vulkan", "Rebuild", error.what(), error.NativeCode());
    } catch (...) {
        Fail(RenderErrorCategory::InitializationFailure, "Rebuild", "failed after retiring the old swapchain");
    }
}

void Barrier(Native& n, VkCommandBuffer command, VkImage image, uint32_t layers, VkImageLayout from, VkImageLayout to,
             VkAccessFlags source, VkAccessFlags destination, VkPipelineStageFlags source_stage, VkPipelineStageFlags destination_stage) {
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER}; barrier.oldLayout = from; barrier.newLayout = to;
    barrier.srcAccessMask = source; barrier.dstAccessMask = destination;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image; barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers};
    n.CmdPipelineBarrier(command, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

float GpuFloat(double value) {
    if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max())
        Fail(RenderErrorCategory::Unsupported, "GpuCoordinates", "input cannot be represented by GPU float arithmetic");
    return static_cast<float>(value);
}
glm::mat4 GpuMatrix(const glm::dmat4& matrix, const std::array<double, 3>& maximum) {
    glm::mat4 result;
    for (int r = 0; r < 4; ++r) {
        double bound = std::abs(matrix[3][r]);
        for (int c = 0; c < 3; ++c) bound += std::abs(matrix[c][r]) * maximum[c];
        if (!std::isfinite(bound) || bound > static_cast<double>(std::numeric_limits<float>::max()) * .999999)
            Fail(RenderErrorCategory::Unsupported, "GpuCoordinates", "clip arithmetic may overflow GPU float range");
        for (int c = 0; c < 4; ++c) result[c][r] = GpuFloat(matrix[c][r]);
    }
    return result;
}
glm::dmat4 CameraMatrix(const CameraParameters& camera, Extent2D extent) {
    const auto forward = glm::normalize(glm::dvec3(camera.forward));
    const auto side = glm::normalize(glm::cross(forward, glm::normalize(glm::dvec3(camera.up))));
    const auto up = glm::cross(side, forward); const glm::dvec3 eye(camera.position);
    glm::dmat4 view(1);
    for (int i = 0; i < 3; ++i) { view[i][0] = side[i]; view[i][1] = up[i]; view[i][2] = -forward[i]; }
    view[3][0] = -glm::dot(side, eye); view[3][1] = -glm::dot(up, eye); view[3][2] = glm::dot(forward, eye);
    return glm::perspectiveRH_ZO(static_cast<double>(camera.vertical_fov_radians), static_cast<double>(extent.width) / extent.height,
                                static_cast<double>(camera.near_plane), static_cast<double>(camera.far_plane)) * view;
}

class VulkanBackend final : public RenderBackend {
    Window& window_;
    RendererConfig config_;
    RendererInfo info_;
    std::unique_ptr<Native> native_;

    template<class F> decltype(auto) Operation(const char* name, F&& body) {
        try { return std::forward<F>(body)(); }
        catch (const RenderError&) { throw; }
        catch (const std::bad_alloc&) { Fail(RenderErrorCategory::AllocationFailure, name, "host allocation failed"); }
        catch (const std::exception& error) { Fail(RenderErrorCategory::InitializationFailure, name, error.what()); }
    }
    std::shared_ptr<Native::Mesh> Mesh(const MeshData& data) {
        auto& n = *native_;
        if (n.diagnostic && std::exchange(n.diagnostic->next_host_allocation, false)) throw std::bad_alloc();
        if (data.vertices.size() > UINT32_MAX) Fail(RenderErrorCategory::Unsupported, "Mesh", "vertex count exceeds Vulkan draw range");
        auto mesh = std::make_shared<Native::Mesh>(); mesh->vertices = static_cast<uint32_t>(data.vertices.size());
        static_assert(sizeof(BlockVertex3D) == 28 && offsetof(BlockVertex3D, tex_coord) == 12 && offsetof(BlockVertex3D, normal) == 24);
        static_assert(sizeof(LineVertex3D) == 12 && offsetof(LineVertex3D, pos_coord) == 0);
        static_assert(sizeof(glm::mat4) == 64 && offsetof(BlockVertex3D, pos_coord) == 0);
        if (!data.vertices.empty()) {
            mesh->buffer = n.MakeBuffer(data.vertices.size() * 28, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, BufferRole::Mesh);
            mesh->buffer->Write(data.vertices.data(), data.vertices.size() * 28);
            for (const auto& vertex : data.vertices) for (int i = 0; i < 3; ++i)
                mesh->maximum[i] = std::max(mesh->maximum[i], std::abs(static_cast<double>(vertex.pos_coord[i])));
        }
        if (n.diagnostic) n.diagnostic->last_mesh = mesh;
        return mesh;
    }
public:
    VulkanBackend(Window& window, const RendererConfig& config, std::shared_ptr<VulkanDiagnosticAccess> diagnostic = {})
        : window_(window), config_(config), native_(std::make_unique<Native>()) {
        native_->diagnostic = std::move(diagnostic);
        try {
            Operation("Initialize", [&] {
                Initialize(*native_, window_, config_, info_);
                Rebuild(*native_, {static_cast<uint32_t>(window.Width()), static_cast<uint32_t>(window.Height())}, config_.vsync, config_.shader_asset_root, info_);
                if (auto& d = native_->diagnostic) {
                    d->instance = native_->instance; d->physical = native_->physical; d->device = native_->device;
                    d->graphics = native_->graphics; d->get_device_proc = native_->GetDeviceProcAddr;
                    d->finish_deferred_present = [n = native_.get()] { n->FinishDiagnosticPresent(); };
                    if (d->on_initialized) d->on_initialized(*d);
                }
            });
        } catch (...) { ShutdownNoexcept(); throw; }
    }
    ~VulkanBackend() override { ShutdownNoexcept(); }
    RendererInfo GetInfo() const override { return info_; }
    void CreateMesh(BackendResourceId id, std::shared_ptr<const MeshData> data) override {
        Operation("CreateMesh", [&] {
            auto [slot, inserted] = native_->meshes.try_emplace(id);
            if (!inserted) Fail(RenderErrorCategory::InvalidHandle, "CreateMesh", "duplicate native identity");
            try { slot->second = Mesh(*data); } catch (...) { native_->meshes.erase(slot); throw; }
        });
    }
    void UpdateMesh(BackendResourceId id, std::shared_ptr<const MeshData> data) override {
        Operation("UpdateMesh", [&] { auto candidate = Mesh(*data); native_->meshes.at(id).swap(candidate); });
    }
    void DestroyMesh(BackendResourceId id) override { native_->meshes.erase(id); }
    void CreateTextureArray(BackendResourceId id, std::shared_ptr<const TextureArrayData> data) override {
        Operation("CreateTextureArray", [&] {
            auto& n = *native_;
            if (data->width > n.properties.limits.maxImageDimension2D || data->height > n.properties.limits.maxImageDimension2D ||
                data->layers > n.properties.limits.maxImageArrayLayers)
                Fail(RenderErrorCategory::Unsupported, "TextureArray", "texture dimensions exceed device limits");
            auto [slot, inserted] = n.textures.try_emplace(id);
            if (!inserted) Fail(RenderErrorCategory::InvalidHandle, "CreateTextureArray", "duplicate native identity");
            try {
                n.staging = n.MakeBuffer(data->pixels.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, BufferRole::Staging);
                n.staging->Write(data->pixels.data(), data->pixels.size());
                const auto format = data->color_space == ColorSpace::SRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
                n.uploading = n.MakeImage(data->width, data->height, data->layers, format,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
                if (n.diagnostic) n.diagnostic->last_texture = n.uploading;
                n.Begin(n.upload);
                Barrier(n, n.upload.command, n.uploading->image, data->layers, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
                VkBufferImageCopy copy{}; copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, data->layers}; copy.imageExtent = {data->width, data->height, 1};
                n.CmdCopyBufferToImage(n.upload.command, n.staging->buffer, n.uploading->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
                Barrier(n, n.upload.command, n.uploading->image, data->layers, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                Check(n.EndCommandBuffer(n.upload.command), "EndTextureUpload");
                VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.commandBufferCount = 1; submit.pCommandBuffers = &n.upload.command;
                n.Submit(n.upload, submit); n.Finish(n.upload, n.Limit());
                slot->second = std::move(n.uploading); n.staging.reset();
            } catch (...) {
                n.textures.erase(slot);
                if (!n.upload.pending && !n.unsafe) { n.staging.reset(); n.uploading.reset(); }
                throw;
            }
        });
    }
    void DestroyTextureArray(BackendResourceId id) override { native_->textures.erase(id); }
    FrameResult Render(const PreparedFrame& frame) override;
    void Resize(Extent2D extent) override {
        if (extent.width == info_.framebuffer_extent.width && extent.height == info_.framebuffer_extent.height) return;
        Operation("Resize", [&] { Rebuild(*native_, extent, config_.vsync, config_.shader_asset_root, info_); });
    }
    void SetVSync(bool enabled) override {
        if (enabled == config_.vsync) return;
        Operation("SetVSync", [&] { Rebuild(*native_, info_.framebuffer_extent, enabled, config_.shader_asset_root, info_); config_.vsync = enabled; });
    }
    std::vector<GpuFrameSample> PollGpuSamples() override {
        return Operation("PollGpuSamples", [&] {
            auto& n = *native_;
            for (auto& frame : n.frames) if (frame.pending) {
                const auto result = n.Observe(ObservedCall::FencePoll, [&] { return n.GetFenceStatus(n.device, frame.fence); });
                if (result == VK_SUCCESS) { frame.pending = false; n.Collect(frame); }
                else if (result != VK_NOT_READY) { n.unsafe = true; Check(result, "PollSubmitFence", true); }
            }
            std::vector<GpuFrameSample> result; result.reserve(n.ready_count);
            while (n.ready_count) { result.push_back(n.ready[n.ready_begin]); n.ready_begin = (n.ready_begin + 1) % n.ready.size(); --n.ready_count; }
            return result;
        });
    }
    void Shutdown() override {
        if (!native_) return;
        if (native_->unsafe) Fail(RenderErrorCategory::Timeout, "Shutdown", "native completion remains unproven; terminal retention required");
        if (native_->device) native_->Drain(native_->Limit());
        native_.reset();
    }
    void ShutdownNoexcept() noexcept override {
        if (!native_) return;
        try { Shutdown(); }
        catch (...) {
            // Intentionally never destroyed: every dependent object and actual loader reference remain together.
            std::fprintf(stderr, "[renderer] Vulkan terminal retention: native completion unproven; not a successful Shutdown\n");
            if (native_->diagnostic) {
                native_->diagnostic->quarantined = true;
                native_->diagnostic->warnings = native_->warnings.load(); native_->diagnostic->errors = native_->errors.load();
            }
            (void)native_.release();
        }
    }
};

FrameResult VulkanBackend::Render(const PreparedFrame& input) {
    return Operation("Render", [&] {
        auto& n = *native_;
        const auto start = Clock::now(); const auto deadline = n.Limit();
        FrameResult result; result.frame_id = input.frame_id;
        if (window_.Minimized() || !window_.Width() || !window_.Height()) {
            info_.framebuffer_extent = {}; result.skip_reason = FrameSkipReason::ZeroExtent;
            n.Ready({input.frame_id,false,{},GpuSampleMissingReason::Discarded}); return result;
        }
        if (static_cast<uint32_t>(window_.Width()) != input.extent.width || static_cast<uint32_t>(window_.Height()) != input.extent.height)
            Fail(RenderErrorCategory::InvalidState, "Render", "window extent changed; Resize is required before rendering");
        if (n.rebuild) {
            VkSurfaceCapabilitiesKHR current{};
            Check(n.GetPhysicalDeviceSurfaceCapabilitiesKHR(n.physical,n.surface,&current),"RebuildSurfaceCapabilities");
            if (!current.currentExtent.width || !current.currentExtent.height) {
                info_.framebuffer_extent = {}; result.skip_reason = FrameSkipReason::ZeroExtent;
                n.Ready({input.frame_id,false,{},GpuSampleMissingReason::Discarded}); return result;
            }
            if (current.currentExtent.width != UINT32_MAX &&
                (current.currentExtent.width != input.extent.width || current.currentExtent.height != input.extent.height)) {
                result.skip_reason = FrameSkipReason::Occluded;
                n.Ready({input.frame_id,false,{},GpuSampleMissingReason::Discarded}); return result;
            }
            try { Rebuild(n, input.extent, config_.vsync, config_.shader_asset_root, info_); }
            catch (const RenderError& error) {
                if (error.Category() != RenderErrorCategory::InvalidState || error.Diagnostic() != "surface_extent_pending") throw;
                result.skip_reason = FrameSkipReason::Occluded;
                n.Ready({input.frame_id,false,{},GpuSampleMissingReason::Discarded}); return result;
            }
        }
        DiagnosticPhase(FramePhase::SlotWaitCollect);
        auto& frame = n.frames[n.next_frame]; n.Finish(frame, deadline); n.Collect(frame);
        if (n.diagnostic) {
            n.diagnostic->last_binding_slot = static_cast<unsigned>(n.next_frame);
            n.diagnostic->last_binding_frame_id = input.frame_id;
        }
        DiagnosticPhase(FramePhase::CommandBegin);
        Check(n.ResetCommandPool(n.device, frame.pool, 0), "ResetRetiredCommands");
        DiagnosticPhase(FramePhase::ResourceRetire);
        n.RetireFrame(frame);
        DiagnosticPhase(FramePhase::CpuPack);
        frame.meshes.reserve(input.draws.size()); frame.textures.reserve(input.draws.size());
        const auto camera = CameraMatrix(input.camera, input.extent);
        const size_t draw_count = input.draws.size() + (input.selection ? 1 : 0);
        if (draw_count > UINT32_MAX / 3) Fail(RenderErrorCategory::Unsupported, "Descriptors", "frame descriptor count exceeds Vulkan range");
        const auto alignment = std::max<VkDeviceSize>(64, n.properties.limits.minUniformBufferOffsetAlignment);
        const auto stride = (64 + alignment - 1) / alignment * alignment;
        std::vector<glm::mat4> constants; constants.reserve(draw_count);
        for (const auto& draw : input.draws) {
            auto mesh = n.meshes.at(draw.mesh); auto texture = n.textures.at(draw.texture);
            constants.push_back(GpuMatrix(camera * glm::dmat4(draw.object_transform), mesh->maximum));
            frame.meshes.push_back(std::move(mesh)); frame.textures.push_back(std::move(texture));
        }
        if (input.selection) {
            const glm::dvec3 center = (glm::dvec3(input.selection->minimum) + glm::dvec3(input.selection->maximum)) * .5;
            const glm::dvec3 size = glm::dvec3(input.selection->maximum) - glm::dvec3(input.selection->minimum);
            const glm::dvec3 corners[]{{0,0,0},{0,0,1},{1,0,1},{1,0,0},{0,1,0},{0,1,1},{1,1,1},{1,1,0}};
            const int edges[]{7,6,6,2,2,3,3,7,7,4,6,5,2,1,3,0,5,4,4,0,0,1,1,5};
            std::array<LineVertex3D,24> vertices; std::array<double,3> maximum{};
            for (size_t i = 0; i < vertices.size(); ++i) for (int c = 0; c < 3; ++c) {
                const auto coordinate = center[c] + (corners[edges[i]][c] - .5) * size[c] * 1.004;
                vertices[i].pos_coord[c] = GpuFloat(coordinate); maximum[c] = std::max(maximum[c], std::abs(coordinate));
            }
            constants.push_back(GpuMatrix(camera, maximum));
            DiagnosticPhase(FramePhase::SelectionAllocate);
            WriteRetainedSelectionBuffer(frame.selection, [&] {
                return n.MakeBuffer(sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, BufferRole::SelectionVertex);
            }, [&](Native::Buffer& buffer) {
                DiagnosticPhase(FramePhase::SelectionWrite);
                buffer.Write(vertices.data(), sizeof(vertices));
            });
            result.statistics.upload_bytes += sizeof(vertices);
        }
        DiagnosticPhase(FramePhase::CpuPack);
        DiagnosticPhase(FramePhase::UniformAllocate);
        if (draw_count) {
            const auto limit = static_cast<size_t>(std::min<uint64_t>(UINT32_MAX / 3,
                std::min<uint64_t>(SIZE_MAX, std::numeric_limits<VkDeviceSize>::max()) / stride));
            if (input.draws.size() > limit)
                Fail(RenderErrorCategory::Unsupported, "Uniforms", "frame uniform storage exceeds addressable range");
            std::unique_ptr<Native::FrameBindings> replacement, selection_replacement;
            if (!input.draws.empty() && (!frame.bindings || frame.bindings->cache.Capacity() < input.draws.size())) {
                const auto capacity = FrameBindingCache::CapacityFor(frame.bindings ? frame.bindings->cache.Capacity() : 0,
                    input.draws.size(), limit);
                replacement = n.MakeFrameBindings(capacity, stride);
            }
            if (input.selection && !frame.selection_binding) selection_replacement = n.MakeFrameBindings(1, stride, true);
            // Publish both bundles only after every candidate allocation succeeds.
            if (replacement) frame.bindings.swap(replacement);
            if (selection_replacement) frame.selection_binding.swap(selection_replacement);
            replacement.reset(); selection_replacement.reset();
            DiagnosticPhase(FramePhase::UniformPackWrite);
            if (!input.draws.empty()) {
                auto& bindings = *frame.bindings;
                const auto bytes = stride * input.draws.size();
                for (size_t i = 0; i < input.draws.size(); ++i)
                    std::memcpy(bindings.packed.data() + stride * i, &constants[i], 64);
                bindings.uniforms->Write(bindings.packed.data(), bytes);
                if (bindings.observed) { ++bindings.observed->uniform_writes; bindings.observed->uniform_bytes += bytes; }
                result.statistics.upload_bytes += bytes;
            }
            if (input.selection) {
                auto& bindings = *frame.selection_binding;
                std::memcpy(bindings.packed.data(), &constants.back(), 64);
                bindings.uniforms->Write(bindings.packed.data(), stride);
                if (bindings.observed) { ++bindings.observed->uniform_writes; bindings.observed->uniform_bytes += stride; }
                result.statistics.upload_bytes += stride;
            }
            DiagnosticPhase(FramePhase::DescriptorAllocate);
            // On cache hits this phase is a logical boundary, not a native allocation.
            DiagnosticPhase(FramePhase::DescriptorUpdate);
            for (size_t i = 0; i < input.draws.size(); ++i) {
                auto& bindings = *frame.bindings;
                bindings.cache.WriteMesh(i, input.draws[i].texture, [&](FrameBindingCache::Writes dirty) {
                    n.WriteFrameDescriptors(bindings, i, dirty, frame.textures[i].get());
                });
            }
            if (input.selection) {
                auto& bindings = *frame.selection_binding;
                bindings.cache.WriteUniform(0, [&](FrameBindingCache::Writes dirty) {
                    n.WriteFrameDescriptors(bindings, 0, dirty, nullptr);
                });
            }
        }
        DiagnosticPhase(FramePhase::CommandBegin);
        if (input.request_screenshot) {
            const auto bytes = static_cast<uint64_t>(input.extent.width) * input.extent.height * 4;
            result.screenshot.emplace(Screenshot{input.extent, std::vector<uint8_t>(static_cast<size_t>(bytes))});
            frame.readback = n.MakeBuffer(bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, BufferRole::Readback);
        }
        n.Begin(frame, false);
        DiagnosticPhase(FramePhase::AcquireImage);
        uint32_t index{};
        VkResult acquire;
        const auto injected_acquire = n.diagnostic ? std::exchange(n.diagnostic->next_acquire, VK_SUCCESS) : VK_SUCCESS;
        if (injected_acquire == VK_ERROR_OUT_OF_DATE_KHR) {
            // Do not acquire/signify a semaphore and then pretend no image was acquired.
            ++n.diagnostic->acquire_out_of_date_injections; acquire = injected_acquire;
        } else {
            acquire = n.Observe(ObservedCall::Acquire, [&] { return n.AcquireNextImageKHR(n.device, n.swapchain, Remaining(deadline), frame.acquire, {}, &index); });
            if (n.diagnostic) {
                ++n.diagnostic->acquire_calls; n.diagnostic->last_actual_acquire = acquire;
                if (acquire == VK_SUCCESS && injected_acquire == VK_SUBOPTIMAL_KHR) acquire = injected_acquire;
            }
        }
        if (n.diagnostic) n.diagnostic->last_effective_acquire = acquire;
        if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
            n.rebuild = true; result.screenshot.reset(); result.skip_reason = FrameSkipReason::Occluded;
            n.Ready({input.frame_id, false, {}, GpuSampleMissingReason::Discarded}); return result;
        }
        Check(acquire == VK_SUBOPTIMAL_KHR ? VK_SUCCESS : acquire, "AcquireImage", true);
        DiagnosticPhase(FramePhase::PresentFenceWait);
        n.unsafe = true; // Until the acquired semaphore has a tracked submitted consumer.
        auto& image = n.images.at(index);
        if (image.pending) { n.Wait(image.presented, deadline, "WaitReacquiredPresent", ObservedCall::WaitReacquiredPresent); image.pending = false; }
        DiagnosticPhase(FramePhase::CommandRecord);
        const VkClearValue clear[2]{{{{.529f,.808f,.922f,1.f}}}, {.depthStencil={1,0}}};
        const VkRect2D scissor{{0,0},n.extent};
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO}; render.renderPass = n.render_pass;
        render.framebuffer = image.framebuffer; render.renderArea = scissor; render.clearValueCount = 2; render.pClearValues = clear;
        if (frame.query) n.CmdResetQueryPool(frame.command, frame.query, 0, 2);
        n.CmdBeginRenderPass(frame.command, &render, VK_SUBPASS_CONTENTS_INLINE);
        const VkViewport viewport{0,static_cast<float>(n.extent.height),static_cast<float>(n.extent.width),-static_cast<float>(n.extent.height),0,1};
        n.CmdSetViewport(frame.command, 0, 1, &viewport); n.CmdSetScissor(frame.command, 0, 1, &scissor);
        if (frame.query) n.CmdWriteTimestamp(frame.command, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, frame.query, 0);
        n.CmdBindPipeline(frame.command, VK_PIPELINE_BIND_POINT_GRAPHICS, n.blocks);
        const VkDeviceSize offset = 0;
        for (size_t i = 0; i < frame.meshes.size(); ++i) if (frame.meshes[i]->vertices) {
            const auto& mesh = frame.meshes[i];
            n.CmdBindVertexBuffers(frame.command, 0, 1, &mesh->buffer->buffer, &offset);
            n.CmdBindDescriptorSets(frame.command, VK_PIPELINE_BIND_POINT_GRAPHICS, n.pipeline_layout, 0, 1, &frame.bindings->sets[i], 0, nullptr);
            n.CmdDraw(frame.command, mesh->vertices, 1, 0, 0); ++result.statistics.draw_calls; result.statistics.drawn_vertices += mesh->vertices;
        }
        if (input.selection) {
            n.CmdBindPipeline(frame.command, VK_PIPELINE_BIND_POINT_GRAPHICS, n.selection);
            n.CmdBindVertexBuffers(frame.command, 0, 1, &frame.selection->buffer, &offset);
            n.CmdBindDescriptorSets(frame.command, VK_PIPELINE_BIND_POINT_GRAPHICS, n.pipeline_layout, 0, 1, &frame.selection_binding->sets[0], 0, nullptr);
            n.CmdDraw(frame.command, 24, 1, 0, 0); ++result.statistics.draw_calls; result.statistics.drawn_vertices += 24;
        }
        if (frame.query) n.CmdWriteTimestamp(frame.command, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, frame.query, 1);
        n.CmdEndRenderPass(frame.command);
        if (input.request_screenshot) {
            Barrier(n, frame.command, image.image, 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            VkBufferImageCopy copy{}; copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT,0,0,1}; copy.imageExtent = {n.extent.width,n.extent.height,1};
            n.CmdCopyImageToBuffer(frame.command, image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, frame.readback->buffer, 1, &copy);
            VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
            host.srcQueueFamilyIndex = host.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            host.buffer = frame.readback->buffer; host.size = VK_WHOLE_SIZE;
            n.CmdPipelineBarrier(frame.command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0,
                                 0, nullptr, 1, &host, 0, nullptr);
        }
        Barrier(n, frame.command, image.image, 1,
                input.request_screenshot ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                input.request_screenshot ? VK_ACCESS_TRANSFER_READ_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
                input.request_screenshot ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        Check(n.EndCommandBuffer(frame.command), "EndFrame", true);
        result.statistics.cpu_prepare = Clock::now() - start;
        const auto submit_start = Clock::now();
        DiagnosticPhase(FramePhase::QueueSubmit);
        const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.waitSemaphoreCount = 1; submit.pWaitSemaphores = &frame.acquire; submit.pWaitDstStageMask = &wait_stage;
        submit.commandBufferCount = 1; submit.pCommandBuffers = &frame.command; submit.signalSemaphoreCount = 1; submit.pSignalSemaphores = &image.rendered;
        const bool gated = n.diagnostic && n.diagnostic->next_gate;
        n.Submit(frame, submit); frame.frame_id = input.frame_id; frame.query_pending = frame.query != VK_NULL_HANDLE;
        if (n.diagnostic && !frame.meshes.empty()) {
            n.diagnostic->frame_mesh = frame.meshes.front(); n.diagnostic->frame_texture = frame.textures.front();
        }
        if (!frame.query) n.Ready({input.frame_id,false,{},GpuSampleMissingReason::Unsupported});
        result.statistics.cpu_submit = Clock::now() - submit_start;
        DiagnosticPhase(FramePhase::BackendTail);
        if (gated) {
            // WSI cannot wait on an unresolved host timeline dependency. Only the
            // isolated diagnostic defers Present until the host has released it.
            n.diagnostic_present_index = index; n.diagnostic->deferred_present_pending = true; n.unsafe = true;
            n.next_frame = (n.next_frame + 1) % n.frames.size();
            if (input.request_screenshot) n.Finish(frame, n.Limit());
            result.screenshot.reset(); result.skip_reason = FrameSkipReason::Occluded;
            return result;
        }
        Check(n.ResetFences(n.device, 1, &image.presented), "ResetPresentFence", true);
        VkSwapchainPresentFenceInfoKHR retirement{VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR};
        if (!n.khr_maintenance) retirement.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
        retirement.swapchainCount = 1; retirement.pFences = &image.presented;
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR}; present.pNext = &retirement;
        present.waitSemaphoreCount = 1; present.pWaitSemaphores = &image.rendered; present.swapchainCount = 1;
        present.pSwapchains = &n.swapchain; present.pImageIndices = &index;
        if (n.diagnostic && n.diagnostic->observe_performance) {
            auto& observed = *n.diagnostic;
            observed.queue_observation_frame_id = input.frame_id; observed.present_image = index;
            observed.tracked_submit_pending = static_cast<uint32_t>(std::count_if(n.frames.begin(), n.frames.end(), [](const auto& f) { return f.pending; }));
            observed.tracked_present_pending = static_cast<uint32_t>(std::count_if(n.images.begin(), n.images.end(), [](const auto& i) { return i.pending; }));
            // Nonblocking diagnostic snapshots are not a queue occupancy or completion wait.
            observed.submit_before_present = n.Observe(ObservedCall::FencePoll, [&] { return n.GetFenceStatus(n.device, frame.fence); });
        }
        const auto present_start = Clock::now(); n.unsafe = true;
        DiagnosticPhase(FramePhase::QueuePresent);
        auto presented = n.Observe(ObservedCall::QueuePresent, [&] { return n.QueuePresentKHR(n.present, &present); });
        if (n.diagnostic) {
            ++n.diagnostic->present_calls;
            n.diagnostic->last_actual_present = presented;
            const auto injected = std::exchange(n.diagnostic->next_present, VK_SUCCESS);
            if (injected != VK_SUCCESS && presented == VK_SUCCESS) presented = injected;
            n.diagnostic->last_effective_present = presented;
        }
        result.statistics.present_wait = Clock::now() - present_start;
        if (n.diagnostic && n.diagnostic->observe_performance)
            n.diagnostic->submit_after_present = n.Observe(ObservedCall::FencePoll, [&] { return n.GetFenceStatus(n.device, frame.fence); });
        DiagnosticPhase(FramePhase::BackendTail);
        if (presented == VK_SUCCESS || presented == VK_SUBOPTIMAL_KHR || presented == VK_ERROR_OUT_OF_DATE_KHR) {
            image.pending = true; n.unsafe = false;
        } else Check(presented, "QueuePresent", true);
        n.rebuild = acquire == VK_SUBOPTIMAL_KHR || presented == VK_SUBOPTIMAL_KHR || presented == VK_ERROR_OUT_OF_DATE_KHR;
        n.next_frame = (n.next_frame + 1) % n.frames.size();
        if (input.request_screenshot) {
            n.Finish(frame, n.Limit()); frame.readback->Read(result.screenshot->pixels.data(), result.screenshot->pixels.size());
            if (n.format == VK_FORMAT_B8G8R8A8_UNORM)
                for (size_t i = 0; i < result.screenshot->pixels.size(); i += 4) std::swap(result.screenshot->pixels[i], result.screenshot->pixels[i+2]);
        }
        if (presented == VK_ERROR_OUT_OF_DATE_KHR) { result.screenshot.reset(); result.skip_reason = FrameSkipReason::Occluded; }
        else result.status = FrameStatus::Presented;
        if (n.errors.load()) Fail(RenderErrorCategory::SubmissionFailure, "Validation", "native validation errors reported; stop submission and inspect diagnostics");
        return result;
    });
}
} // namespace

namespace {
thread_local bool diagnostic_binding_active = false;
thread_local std::shared_ptr<VulkanDiagnosticAccess> diagnostic_binding;
}
ScopedVulkanDiagnostics::ScopedVulkanDiagnostics(std::shared_ptr<VulkanDiagnosticAccess> control) {
    if (diagnostic_binding_active || !control || control->device || control->loader_address || control->native_destroyed || control->quarantined)
        Fail(RenderErrorCategory::InvalidInput, "DiagnosticBinding", "fresh exclusive creating-thread binding required");
    diagnostic_binding = std::move(control); diagnostic_binding_active = true;
}
ScopedVulkanDiagnostics::~ScopedVulkanDiagnostics() { diagnostic_binding.reset(); diagnostic_binding_active = false; }
std::unique_ptr<RenderBackend> CreateVulkanBackend(Window& window, const RendererConfig& config) {
    if (diagnostic_binding_active) {
        auto control = std::exchange(diagnostic_binding, {});
        if (!control) Fail(RenderErrorCategory::InvalidState, "DiagnosticBinding", "scoped backend already created");
        auto diagnostic_config = config; diagnostic_config.wait_timeout = control->wait_timeout;
        return std::make_unique<VulkanBackend>(window, diagnostic_config, std::move(control));
    }
    return std::make_unique<VulkanBackend>(window, config);
}
} // namespace SymoCraft::Rendering::Detail
