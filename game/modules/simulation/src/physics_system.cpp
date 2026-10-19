#include <symocraft/simulation/physics_system.h>
#include "physics_adapter.h"

namespace SymoCraft::Physics {
    void Update(ECS::Registry& registry, const World::VoxelWorld& world, float frame_delta,
                PlayerMath::FixedStepBudget& budget) {
        const auto steps = budget.Consume(frame_delta);
        std::uint64_t commits = 0;
        for (unsigned step = 0; step < steps; ++step)
            Simulation::Detail::StepBodies(registry, world, commits);
    }

    RaycastStaticResult RayCastStatic(const World::VoxelWorld& world, const glm::vec3& origin,
                                      const glm::vec3& direction, float distance, bool /*draw*/) {
        const auto hit = RayCast(origin, direction, distance, Simulation::Detail::CollisionQuery(world));
        RaycastStaticResult result{};
        if (hit.hit) {
            result.hit = true;
            result.block_center = glm::vec3(hit.cell) + glm::vec3(0.5f);
            result.block_size = glm::vec3(1.0f);
            result.hit_normal = hit.normal;
            result.point = origin + glm::normalize(direction) * hit.distance;
        }
        return result;
    }
}
