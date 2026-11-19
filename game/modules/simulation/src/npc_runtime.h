#pragma once

#include <symocraft/ecs/registry.h>
#include <symocraft/simulation/npc.h>
#include <optional>

namespace SymoCraft::Simulation::Detail {
    struct NpcDiagnosticsAdapter;
    struct NpcComponent {
        NpcConfig config;
        ActionSnapshot action;
        World::HarvestTarget target{};
        World::HarvestRule rule{};
        glm::vec3 work_position{0.0f};
        double blocked_elapsed{};
        float previous_distance{};
        glm::vec3 step_origin{0.0f};
        double horizontal_distance{};
        std::vector<ResourceCount> resources;
    };
    class NpcRuntime final {
    public:
        NpcRuntime(ECS::Registry& registry, const World::VoxelWorld& world);
        ECS::EntityId Add(const NpcConfig& config);
        void Remove(ECS::EntityId entity);
        ActionId Start(ECS::EntityId entity, World::BlockCoord target);
        void Cancel(ECS::EntityId entity, ActionId action);
        NpcSnapshot Inspect(ECS::EntityId entity) const;
        std::vector<NpcDisplay> Displays() const;
        bool Empty() const noexcept { return entities_.empty(); }
        void BeforeAdvance(float seconds);
        void BeforePhysics();
        void AfterPhysics();
        void Pause();
        void Commit(World::VoxelWorld& world);
    private:
        friend struct NpcDiagnosticsAdapter;
        NpcComponent& Require(ECS::EntityId entity);
        const NpcComponent& Require(ECS::EntityId entity) const;
        bool ValidWorkPosition(ECS::EntityId entity, const NpcComponent& npc) const;
        bool WorkPointClear(const glm::vec3& position) const;
        void Stop(ECS::EntityId entity, NpcComponent& npc, ActionFailure reason);
        ECS::Registry& registry_;
        const World::VoxelWorld& world_;
        std::vector<ECS::EntityId> entities_;
        ActionId next_action_ = 1;
        bool fail_after_remove_ = false;
    };
}
