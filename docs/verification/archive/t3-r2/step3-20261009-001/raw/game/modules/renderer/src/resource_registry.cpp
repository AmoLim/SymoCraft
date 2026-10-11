#include "resource_registry.h"

#include "symocraft/renderer/error.h"
#include "validation.h"

#include <atomic>
#include <limits>

namespace SymoCraft::Rendering::Detail {
namespace {

std::uint64_t NextIdentity() {
    static std::atomic<std::uint64_t> next{1};
    auto candidate = next.load(std::memory_order_relaxed);
    for (;;) {
        if (candidate == std::numeric_limits<std::uint64_t>::max())
            throw ResourceRegistryError(RenderErrorCategory::AllocationFailure,
                                        "renderer resource identity exhausted");
        if (next.compare_exchange_weak(candidate, candidate + 1, std::memory_order_relaxed))
            return candidate;
    }
}

} // namespace

ResourceRegistry::ResourceRegistry() : identity_(NextIdentity()) {}

template <class Tag, class Data>
const ResourceRegistry::Slot<Data>& ResourceRegistry::Find(
    ResourceHandle<Tag> handle, const std::vector<Slot<Data>>& slots) const {
    if (handle.owner_ != identity_ || handle.slot_ >= slots.size())
        throw ResourceRegistryError(RenderErrorCategory::InvalidHandle,
                                    "invalid or foreign renderer resource handle");
    const auto& slot = slots[handle.slot_];
    if (!slot.data || handle.generation_ != slot.generation)
        throw ResourceRegistryError(RenderErrorCategory::InvalidHandle,
                                    "stale renderer resource handle");
    return slot;
}

template <class Data>
std::size_t ResourceRegistry::FreeSlot(std::vector<Slot<Data>>& slots) {
    std::size_t index = 0;
    while (index < slots.size() && (slots[index].data || slots[index].generation == 0)) ++index;
    if (index == slots.size()) slots.emplace_back();
    return index;
}

ResourceRegistry::MeshReservation ResourceRegistry::PrepareMesh(const MeshData& data) {
    ValidateMesh(data);
    auto owned = std::make_shared<const MeshData>(data);
    const auto index = FreeSlot(meshes_);
    const auto generation = meshes_[index].generation;
    return {{identity_, index, generation}, {identity_, index, generation}, std::move(owned)};
}

ResourceRegistry::TextureReservation ResourceRegistry::PrepareTexture(const TextureArrayData& data) {
    ValidateTextureArray(data);
    auto owned = std::make_shared<const TextureArrayData>(data);
    const auto index = FreeSlot(textures_);
    const auto generation = textures_[index].generation;
    return {{identity_, index, generation}, {identity_, index, generation}, std::move(owned)};
}

void ResourceRegistry::Commit(const MeshReservation& reservation) noexcept {
    meshes_[reservation.id.slot].data = reservation.data;
}

void ResourceRegistry::Commit(const TextureReservation& reservation) noexcept {
    textures_[reservation.id.slot].data = reservation.data;
}

void ResourceRegistry::ReplaceMesh(MeshHandle handle, std::shared_ptr<const MeshData> data) noexcept {
    meshes_[handle.slot_].data = std::move(data);
}

BackendResourceId ResourceRegistry::Id(MeshHandle handle) const {
    Find(handle, meshes_);
    return {identity_, handle.slot_, handle.generation_};
}

BackendResourceId ResourceRegistry::Id(TextureArrayHandle handle) const {
    Find(handle, textures_);
    return {identity_, handle.slot_, handle.generation_};
}

std::shared_ptr<const MeshData> ResourceRegistry::Mesh(MeshHandle handle) const {
    return Find(handle, meshes_).data;
}

std::shared_ptr<const TextureArrayData> ResourceRegistry::Texture(TextureArrayHandle handle) const {
    return Find(handle, textures_).data;
}

template <class Data>
void ResourceRegistry::Invalidate(Slot<Data>& slot) noexcept {
    slot.data.reset();
    slot.generation = slot.generation == std::numeric_limits<std::uint64_t>::max()
                          ? 0 : slot.generation + 1;
}

void ResourceRegistry::DestroyMesh(MeshHandle handle) noexcept {
    Invalidate(meshes_[handle.slot_]);
}

void ResourceRegistry::DestroyTexture(TextureArrayHandle handle) noexcept {
    Invalidate(textures_[handle.slot_]);
}

void ResourceRegistry::Clear() noexcept {
    for (auto& slot : meshes_) Invalidate(slot);
    for (auto& slot : textures_) Invalidate(slot);
}

} // namespace SymoCraft::Rendering::Detail
