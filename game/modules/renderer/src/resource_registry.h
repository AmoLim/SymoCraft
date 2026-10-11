#pragma once

#include "backend/render_backend.h"
#include "symocraft/renderer/error.h"

namespace SymoCraft::Rendering::Detail {

class ResourceRegistryError final : public std::runtime_error {
public:
    ResourceRegistryError(RenderErrorCategory category, const char* message)
        : std::runtime_error(message), category(category) {}
    const RenderErrorCategory category;
};

class ResourceRegistry final {
public:
    ResourceRegistry();
    ResourceRegistry(const ResourceRegistry&) = delete;
    ResourceRegistry& operator=(const ResourceRegistry&) = delete;

    struct MeshReservation {
        MeshHandle handle;
        BackendResourceId id;
        std::shared_ptr<const MeshData> data;
    };
    struct TextureReservation {
        TextureArrayHandle handle;
        BackendResourceId id;
        std::shared_ptr<const TextureArrayData> data;
    };

    MeshReservation PrepareMesh(const MeshData& data);
    TextureReservation PrepareTexture(const TextureArrayData& data);
    void Commit(const MeshReservation& reservation) noexcept;
    void Commit(const TextureReservation& reservation) noexcept;
    void ReplaceMesh(MeshHandle handle, std::shared_ptr<const MeshData> data) noexcept;
    BackendResourceId Id(MeshHandle handle) const;
    BackendResourceId Id(TextureArrayHandle handle) const;
    std::shared_ptr<const MeshData> Mesh(MeshHandle handle) const;
    std::shared_ptr<const TextureArrayData> Texture(TextureArrayHandle handle) const;
    void DestroyMesh(MeshHandle handle) noexcept;
    void DestroyTexture(TextureArrayHandle handle) noexcept;
    void Clear() noexcept;

private:
    template <class Data>
    struct Slot {
        std::uint64_t generation = 1;
        std::shared_ptr<const Data> data;
    };

    template <class Tag, class Data>
    const Slot<Data>& Find(ResourceHandle<Tag> handle, const std::vector<Slot<Data>>& slots) const;
    template <class Data>
    static std::size_t FreeSlot(std::vector<Slot<Data>>& slots);
    template <class Data>
    static void Invalidate(Slot<Data>& slot) noexcept;

    const std::uint64_t identity_;
    std::vector<Slot<MeshData>> meshes_;
    std::vector<Slot<TextureArrayData>> textures_;
};

} // namespace SymoCraft::Rendering::Detail
