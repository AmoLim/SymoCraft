#pragma once

#include <symocraft/foundation/math.h>
#include <symocraft/simulation/player.h>
#include <symocraft/world/world.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>

namespace SymoCraft::Simulation {
    namespace Detail { struct SessionDiagnosticsAdapter; }
    enum class SessionState { Ready, Paused, Failed };
    enum class AdvanceStage { Input, Character, Physics, Recovery, Interaction, Result };
    struct PointerInput { double mouse_dx{}, mouse_dy{}, scroll_y{}; };
    struct PlayerPose {
        glm::vec3 position{0.0f};
        float yaw = -90.0f, pitch = 0.0f;
        glm::vec3 forward{0.0f, 0.0f, -1.0f}, up{0.0f, 1.0f, 0.0f};
    };
    struct LensState {
        float fov = 45.0f, near_plane = 0.1f, far_plane = 2000.0f;
        glm::vec3 eye_offset{0.0f, 0.65f, 0.0f};
    };
    struct PlayerView {
        glm::vec3 position, forward, up;
        float fov, near_plane, far_plane;
    };
    struct SelectionHit { glm::ivec3 cell; glm::vec3 normal; float distance; };
    struct AdvanceInput {
        PlayerIntent player;
        bool allow_interaction = true, place = false, remove = false;
    };
    struct AdvanceStatistics {
        float rule_seconds{};
        unsigned physical_steps{};
        std::uint64_t body_commits{};
        bool recovered = false;
    };
    struct AdvanceResult {
        PlayerPose pose;
        LensState lens;
        std::optional<SelectionHit> selection;
        std::optional<World::EditRequest> edit;
        AdvanceStatistics statistics;
    };
    class Rejected : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class AdvanceFailed : public std::runtime_error {
    public:
        AdvanceFailed(AdvanceStage stage, unsigned steps, std::uint64_t commits);
        AdvanceStage Stage() const noexcept { return stage_; }
        unsigned CompletedSteps() const noexcept { return steps_; }
        std::uint64_t BodyCommits() const noexcept { return commits_; }
    private:
        AdvanceStage stage_;
        unsigned steps_;
        std::uint64_t commits_;
    };

    // Serial, non-reentrant borrower of caller-owned, dedicated storage. Register the
    // required component types first; discard that assembly scope on factory failure.
    class SimulationSession final {
    public:
        static std::unique_ptr<SimulationSession> Create(ECS::Registry& registry,
            const World::VoxelWorld& world, PlayerPose initial = {});
        ~SimulationSession();
        SimulationSession(const SimulationSession&) = delete;
        SimulationSession& operator=(const SimulationSession&) = delete;
        AdvanceResult Advance(double seconds, const AdvanceInput& input,
                              std::span<const PointerInput> pointer = {});
        void Pause();
        void Resume();
        SessionState State() const noexcept;
        PlayerView View() const;
    private:
        struct Impl;
        explicit SimulationSession(std::unique_ptr<Impl> implementation);
        std::unique_ptr<Impl> impl_;
        friend struct Detail::SessionDiagnosticsAdapter;
    };
    // Pure view derivation: a bad ratio rejects only this request, never the session.
    PlayerView DeriveView(const PlayerPose& pose, const LensState& lens, double aspect_ratio);
}
