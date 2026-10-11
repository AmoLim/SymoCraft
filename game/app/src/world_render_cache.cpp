#include "world_render_cache.h"

#include <limits>
#include <stdexcept>

namespace SymoCraft::AppRendering::Detail {

MeshCacheStatistics AfterMeshAcceptance(MeshCacheStatistics previous,
                                        std::size_t vertex_count, bool creation) {
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    if (vertex_count > maximum / sizeof(BlockVertex3D))
        throw std::overflow_error("mesh upload byte count exceeds uint64");
    const auto bytes = static_cast<std::uint64_t>(vertex_count) * sizeof(BlockVertex3D);
    auto& operation_count = creation ? previous.created_meshes : previous.updated_meshes;
    if (operation_count == maximum || bytes > maximum - previous.upload_bytes)
        throw std::overflow_error("mesh cache statistics exceed uint64");
    ++operation_count;
    previous.upload_bytes += bytes;
    return previous;
}

} // namespace SymoCraft::AppRendering::Detail
