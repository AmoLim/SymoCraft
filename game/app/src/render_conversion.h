#pragma once

#include <symocraft/renderer/types.h>

namespace SymoCraft {
class Camera;
namespace Assets { struct Image; }
namespace Simulation { struct PlayerView; }

namespace AppRendering {

Rendering::CameraParameters ToRendererCamera(const Camera& camera);
Rendering::CameraParameters ToRendererCamera(const Simulation::PlayerView& view);
Rendering::TextureArrayData ToRendererAtlas(const Assets::Image& top_left_image);
MeshData CopyRendererMesh(MeshView source);

} // namespace AppRendering
} // namespace SymoCraft
