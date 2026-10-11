#pragma once

#include <symocraft/renderer/types.h>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <optional>

namespace SymoCraft {
class Camera;
class Window;
enum class WindowMode : std::uint8_t;
struct StartupOptions;
namespace World { class VoxelWorld; }
namespace Performance { class Session; struct Memory; }

namespace Application::Detail {
struct MeshUpload {
    std::uint64_t bytes{}, calls{};
    double cpu_ms{};
};

struct GameFrame {
    Rendering::FrameResult result;
    double render_cpu_ms{}, upload_cpu_ms{}, present_ms{};
};

WindowMode SelectWindowMode(const StartupOptions& options);
void AttachRendererContext(Window& window, const StartupOptions& options);
bool CheckRendererAssets(const StartupOptions& options, std::ostream& errors);

// One gameplay loop; this adapter contains only the transitional rendering differences.
class GameRenderer final {
public:
    GameRenderer(Window& window, const StartupOptions& options, bool collect_samples);
    ~GameRenderer() noexcept;
    GameRenderer(const GameRenderer&) = delete;
    GameRenderer& operator=(const GameRenderer&) = delete;

    bool Modern() const noexcept;
    std::uint16_t LoadAtlas(const std::filesystem::path& path);
    MeshUpload AtlasUpload() const noexcept;
    MeshUpload Synchronize(const World::VoxelWorld& world);
    void ReleaseWorld(std::uint64_t identity);
    void Resize();
    void DescribeSession(Performance::Session& session) const;
    void SampleDeviceMemory(Performance::Memory& memory) const;
    void PollGpu(Performance::Session& session, std::optional<std::uint64_t> excluded_frame = {});
    GameFrame Render(std::uint64_t frame_id, const Camera& camera,
                     std::optional<glm::vec3> selection, Performance::Session* session = nullptr,
                     bool screenshot = false);
    void Capture(std::uint64_t frame_id, const Camera& camera,
                 std::optional<glm::vec3> selection, const std::filesystem::path& path,
                 Performance::Session& session);
    void Shutdown();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
}
