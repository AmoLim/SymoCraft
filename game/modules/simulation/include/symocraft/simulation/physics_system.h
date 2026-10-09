//
// Created by Amo on 2022/6/18.
//

#ifndef SYMOCRAFT_PHYSICS_SYSTEM_H
#define SYMOCRAFT_PHYSICS_SYSTEM_H
#include <symocraft/foundation/math.h>
#include <symocraft/ecs/registry.h>
#include <symocraft/simulation/component.h>

namespace SymoCraft::World { class VoxelWorld; }

namespace SymoCraft
{
    struct RaycastStaticResult
    {
        glm::vec3 point;
        glm::vec3 block_center;
        glm::vec3 block_size;
        glm::vec3 hit_normal;
        bool hit;
    };

    namespace Physics
    {
        // TODO: Make Physic System a System object
        // Initializing physics system

        // physic update function
        void Update(ECS::Registry& registry, const World::VoxelWorld& world, float frame_delta);
        void ResetTiming();

        // Ray Casting for player on the block
        RaycastStaticResult RayCastStatic(const World::VoxelWorld& world,
                                           const glm::vec3 &origin, const glm::vec3 &normal_direction
                                           , float max_distance, bool draw = false);
    }
}


#endif //SYMOCRAFT_PHYSICS_SYSTEM_H
