//
// Created by Amo on 2022/6/16.
//

#ifndef SYMOCRAFT_COMPONENT_H
#define SYMOCRAFT_COMPONENT_H
#include <symocraft/foundation/math.h>

namespace SymoCraft
{
    struct Transform
    {
        glm::vec3 position;     // position in world
        glm::vec3 scale;
        float yaw;              // orientation .y
        float pitch;            // orientation .x

        glm::vec3 front;      // x axis
        glm::vec3 up;           // y axis
        glm::vec3 right;        // z axis
    };

    namespace Physics
    {
        struct RigidBody
        {
            glm::vec3 velocity;
            glm::vec3 acceleration;
            bool on_ground;
            bool is_sensor;
            bool use_gravity;

            void zero_forces()
            {
                acceleration = glm::vec3(0.0);
                velocity = glm::vec3(0.0);
            }
        };

        struct HitBox
        {
            glm::vec3 size;
            glm::vec3 offset;
        };
    }

    namespace Character
    {
        struct CharacterComponent
        {
            float base_speed;
            float run_speed;
            float jump_force;
            float down_jump_force;

            glm::vec3 movement_axis;
            // movement axis
            // x:front or back
            // y:up or down
            // z:left or right

            bool is_running;
            bool apply_jump_force;
            bool is_jumping;
        };

        struct PlayerComponent
        {
            float movement_sensitivity;
            glm::vec3 camera_offset;
        };
    }


}

#endif //SYMOCRAFT_COMPONENT_H
