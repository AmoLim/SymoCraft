#pragma once

#include <symocraft/ecs/entity.h>
#include <symocraft/foundation/math.h>
#include <symocraft/world/world.h>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace SymoCraft::Simulation {
    using ActionId = std::uint64_t;
    enum class NpcKind { Human, Zombie };
    enum class ActionPhase { Idle, Moving, Harvesting, Ready, Succeeded, Failed, Cancelled };
    enum class ActionFailure {
        None, InvalidTarget, Unharvestable, Blocked, Interrupted, Cancelled,
        CreditOverflow, CreditPreparation, WorldRejected
    };
    enum class HarvestCommitStage { WorldRemove, Credit };
    class HarvestCommitFailed : public std::exception {
    public:
        HarvestCommitFailed(HarvestCommitStage stage, ECS::EntityId entity, ActionId action,
                            bool world_changed, bool resource_credited) noexcept
            : stage_(stage), entity_(entity), action_(action), world_changed_(world_changed), resource_credited_(resource_credited) {}
        const char* what() const noexcept override { return "Harvest commit failed; session cannot continue"; }
        HarvestCommitStage Stage() const noexcept { return stage_; }
        ECS::EntityId Entity() const noexcept { return entity_; }
        ActionId Action() const noexcept { return action_; }
        bool WorldChanged() const noexcept { return world_changed_; }
        bool ResourceCredited() const noexcept { return resource_credited_; }
    private:
        HarvestCommitStage stage_;
        ECS::EntityId entity_;
        ActionId action_;
        bool world_changed_, resource_credited_;
    };
    struct NpcConfig {
        NpcKind kind = NpcKind::Human;
        glm::vec3 position{0.0f};
        float yaw = 0.0f;
        std::uint64_t order_key = 1;
        double efficiency = 1.0;
        float speed = 2.0f;
        float work_distance = 2.0f;
        float arrival_tolerance = 0.05f;
        double blocked_seconds = 1.0;
    };
    struct ResourceCount { std::string resource_id; std::uint64_t quantity{}; };
    struct ActionSnapshot {
        ActionId id{};
        ActionPhase phase = ActionPhase::Idle;
        ActionFailure failure = ActionFailure::None;
        World::BlockCoord target{0};
        double elapsed_seconds{};
        double duration_seconds{};
    };
    struct NpcSnapshot {
        ECS::EntityId entity;
        NpcKind kind = NpcKind::Human;
        glm::vec3 position{0.0f};
        float yaw{};
        ActionSnapshot action;
        std::vector<ResourceCount> resources;
    };
    struct NpcDisplay {
        ECS::EntityId entity;
        NpcKind kind = NpcKind::Human;
        glm::vec3 position{0.0f};
        float yaw{};
        // Owned, read-only facts. The session/advance pair identifies a sample;
        // repeated display reads never consume motion or action results.
        std::uint64_t session_id{}, advance_id{};
        double horizontal_distance{};
        ActionSnapshot action;
        std::vector<ResourceCount> resources;
    };
    const char* ActionPhaseName(ActionPhase phase) noexcept;
    const char* ActionFailureName(ActionFailure failure) noexcept;

    class SimulationSession;
    // Behavior facades borrow a session and an identity, never a component address.
    class LivingEntity {
    public:
        virtual ~LivingEntity() = default;
        ECS::EntityId Identity() const noexcept { return entity_; }
        virtual ActionId Harvest(World::BlockCoord target);
        virtual void Cancel(ActionId action);
    protected:
        LivingEntity(SimulationSession& session, ECS::EntityId entity) noexcept
            : session_(session), entity_(entity) {}
        SimulationSession& session_;
        ECS::EntityId entity_;
    };
    class Player final : public LivingEntity {
    public:
        Player(SimulationSession& session, ECS::EntityId entity) noexcept : LivingEntity(session, entity) {}
        ActionId Harvest(World::BlockCoord target) override;
    };
    class Mob final : public LivingEntity {
    public:
        Mob(SimulationSession& session, ECS::EntityId entity) noexcept : LivingEntity(session, entity) {}
    };
}
