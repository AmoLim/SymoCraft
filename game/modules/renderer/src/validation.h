#pragma once

#include "symocraft/renderer/types.h"
#include <optional>

namespace SymoCraft::Rendering::Detail {

void ValidateConfig(const RendererConfig& config);
// The maximum is computed during validation; no vertices yields no maximum.
std::optional<std::uint32_t> ValidateMesh(const MeshData& mesh);
void ValidateTextureArray(const TextureArrayData& texture);
void ValidateMeshTextureLayers(std::optional<std::uint32_t> maximum_layer, std::uint32_t layers);
void ValidateCamera(const CameraParameters& camera);
void ValidateTransform(const glm::mat4& transform);
void ValidateSelection(const SelectionBox& selection);

} // namespace SymoCraft::Rendering::Detail
