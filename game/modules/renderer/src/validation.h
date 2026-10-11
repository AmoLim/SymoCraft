#pragma once

#include "symocraft/renderer/types.h"

namespace SymoCraft::Rendering::Detail {

void ValidateConfig(const RendererConfig& config);
void ValidateMesh(const MeshData& mesh);
void ValidateTextureArray(const TextureArrayData& texture);
void ValidateMeshTextureLayers(const MeshData& mesh, std::uint32_t layers);
void ValidateCamera(const CameraParameters& camera);
void ValidateTransform(const glm::mat4& transform);
void ValidateSelection(const SelectionBox& selection);

} // namespace SymoCraft::Rendering::Detail
