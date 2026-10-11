#pragma once

#include <symocraft/renderer/types.h>

namespace SymoCraft {
class Camera;
namespace Assets { struct Image; }

namespace AppRendering {

Rendering::CameraParameters ToRendererCamera(const Camera& camera);
Rendering::TextureArrayData ToRendererAtlas(const Assets::Image& top_left_image);
MeshData CopyRendererMesh(MeshView source);

} // namespace AppRendering
} // namespace SymoCraft
