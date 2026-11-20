#pragma once

#include <symocraft/scene/character_mesh.h>
#include <filesystem>

namespace SymoCraft::Assets {
    // Imports self-contained opaque GLB triangles and optional rigid body parts.
    // Parsing buffers and cgltf objects are released before this returns.
    CharacterMeshData LoadCharacter(const std::filesystem::path& path);
}
