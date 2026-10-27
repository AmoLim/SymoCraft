#include "physics_adapter.h"
#include <symocraft/simulation/component.h>
#include <symocraft/world/constants.h>

namespace SymoCraft::Simulation::Detail {
    namespace {
        bool Solid(const void* context, const glm::ivec3& cell) {
            const auto& world = *static_cast<const World::VoxelWorld*>(context);
            const auto query = world.QueryBlock(cell);
            if (query.status == World::BlockQueryStatus::OutsideWorld || query.block == BlockConstants::AIR_BLOCK) return false;
            const auto rule = world.DescribeBlock(query.block.block_id);
            if (!rule) throw std::logic_error("CollisionQuery: world contains an undefined block");
            return rule->m_is_solid;
        }
    }
    Physics::TerrainCollisionQuery CollisionQuery(const World::VoxelWorld& world) { return {&world, Solid}; }
    void StepBodies(ECS::Registry& registry, const World::VoxelWorld& world, std::uint64_t& committed_bodies) {
        const ECS::Registry& readable = registry;
        registry.EachOrdered<Transform, Physics::RigidBody, const Physics::HitBox>(
        [&](ECS::EntityId entity, const Transform&, const Physics::RigidBody&, const Physics::HitBox&) {
            // Do not filter on the key component: a missing key must reject, not skip a body.
            return readable.GetComponent<PhysicsStepOrder>(entity).value;
        }, [&](ECS::EntityId, Transform& transform, Physics::RigidBody& body, const Physics::HitBox& shape) {
            const Physics::BodyState current{transform.position, body.velocity, body.acceleration,
                body.use_gravity, body.is_sensor, body.on_ground};
            const auto result = Physics::StepBody(current, {shape.size, shape.offset}, CollisionQuery(world));
            // No storage/query work after solving and before this body's commit.
            transform.position = result.body.position;
            body.velocity = result.body.velocity;
            body.acceleration = result.body.acceleration;
            body.on_ground = result.body.on_ground;
            ++committed_bodies;
        });
    }
}
