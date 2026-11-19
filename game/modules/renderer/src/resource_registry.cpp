#include "resource_registry.h"

#include "symocraft/renderer/error.h"
#include "validation.h"

#include <atomic>
#include <cmath>
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

ResourceRegistry::CharacterReservation ResourceRegistry::PrepareCharacter(const CharacterMeshData& data) {
    if (data.vertices.size() % 3 != 0 || data.vertices.size() > 1024 * 1024)
        throw std::invalid_argument("character mesh must contain complete bounded triangles");
    for (const auto& vertex : data.vertices) {
        for (glm::length_t axis = 0; axis < 3; ++axis) {
            if (!std::isfinite(vertex.position[axis]) || !std::isfinite(vertex.normal[axis]) ||
                !std::isfinite(vertex.color[axis]) || vertex.color[axis] < 0 || vertex.color[axis] > 1)
                throw std::invalid_argument("invalid character vertex");
        }
        const auto length = glm::length(vertex.normal);
        if (!std::isfinite(length) || std::abs(length - 1.0f) > 1e-3f)
            throw std::invalid_argument("character normal must be unit length");
    }
    auto owned = std::make_shared<const CharacterMeshData>(data);
    const auto index = FreeSlot(characters_);
    const auto generation = characters_[index].generation;
    return {{identity_, index, generation}, {identity_, index, generation}, std::move(owned)};
}

void ResourceRegistry::Commit(const CharacterReservation& reservation) noexcept {
    characters_[reservation.id.slot].data = reservation.data;
}

BackendResourceId ResourceRegistry::Id(CharacterMeshHandle handle) const {
    Find(handle, characters_);
    return {identity_, handle.slot_, handle.generation_};
}

std::shared_ptr<const CharacterMeshData> ResourceRegistry::CharacterMesh(CharacterMeshHandle handle) const {
    return Find(handle, characters_).data;
}

void ResourceRegistry::DestroyCharacter(CharacterMeshHandle handle) noexcept {
    Invalidate(characters_[handle.slot_]);
}

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
    const auto maximum_layer = ValidateMesh(data);
    auto owned = std::make_shared<const MeshData>(data);
    const auto index = FreeSlot(meshes_);
    const auto generation = meshes_[index].generation;
    return {{identity_, index, generation}, {identity_, index, generation}, std::move(owned), maximum_layer};
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
    meshes_[reservation.id.slot].maximum_layer = reservation.maximum_layer;
}

void ResourceRegistry::Commit(const TextureReservation& reservation) noexcept {
    textures_[reservation.id.slot].data = reservation.data;
}

void ResourceRegistry::ReplaceMesh(MeshHandle handle, std::shared_ptr<const MeshData> data,
                                   std::optional<std::uint32_t> maximum_layer) noexcept {
    meshes_[handle.slot_].data = std::move(data);
    meshes_[handle.slot_].maximum_layer = maximum_layer;
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

std::optional<std::uint32_t> ResourceRegistry::MeshMaximumLayer(MeshHandle handle) const {
    return Find(handle, meshes_).maximum_layer;
}

std::shared_ptr<const TextureArrayData> ResourceRegistry::Texture(TextureArrayHandle handle) const {
    return Find(handle, textures_).data;
}

template <class Data>
void ResourceRegistry::Invalidate(Slot<Data>& slot) noexcept {
    slot.data.reset();
    slot.maximum_layer.reset();
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
    for (auto& slot : characters_) Invalidate(slot);
}

} // namespace SymoCraft::Rendering::Detail
