#pragma once

#include <symocraft/renderer/types.h>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include "shutdown_report.h"

namespace SymoCraft {
class Camera;
class Window;
enum class WindowMode : std::uint8_t;
struct StartupOptions;
namespace World { class VoxelWorld; }
namespace Simulation { struct PlayerView; }

namespace Application::Detail {
struct MeshUpload {
    std::uint64_t bytes{}, calls{};
};

struct GameFrame {
    Rendering::FrameResult result;
};

WindowMode SelectWindowMode(const StartupOptions& options);
void AttachRendererContext(Window& window, const StartupOptions& options);
bool CheckRendererAssets(const StartupOptions& options, std::ostream& errors);

// One gameplay loop; this adapter contains only the transitional rendering differences.
class GameRenderer final {
public:
    GameRenderer(Window& window, const StartupOptions& options);
    ~GameRenderer() noexcept;
    GameRenderer(const GameRenderer&) = delete;
    GameRenderer& operator=(const GameRenderer&) = delete;

    bool Modern() const noexcept;
    Rendering::Backend Backend() const noexcept;
    std::string ProfilingIdentity() const;
    std::uint16_t LoadAtlas(const std::filesystem::path& path);
    MeshUpload Synchronize(const World::VoxelWorld& world);
    void ReleaseWorld(std::uint64_t identity);
    void Resize();
    GameFrame Render(std::uint64_t frame_id, const Simulation::PlayerView& camera,
                     std::optional<glm::vec3> selection, bool screenshot = false);
    void Capture(std::uint64_t frame_id, const Simulation::PlayerView& camera,
                 std::optional<glm::vec3> selection, const std::filesystem::path& path);
    ShutdownReport Shutdown();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
}
