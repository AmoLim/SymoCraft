#pragma once
#include <symocraft/physics/physics.h>
#include <symocraft/ecs/registry.h>
#include <symocraft/world/world.h>
#include <cstdint>

namespace SymoCraft::Simulation::Detail {
    Physics::TerrainCollisionQuery CollisionQuery(const World::VoxelWorld& world);
    void StepBodies(ECS::Registry& registry, const World::VoxelWorld& world,
                    std::uint64_t& committed_bodies);
}
