//
// Created by Amo on 2022/6/18.
//
#include <symocraft/simulation/transform_system.h>
#include <cmath>
#include <symocraft/simulation/component.h>

namespace SymoCraft
{
    namespace TransformSystem
    {
        void Update(ECS::Registry &registry)
        {
            for (ECS::EntityId entity : registry.View<Transform>())
            {
                Transform& transform = registry.GetComponent<Transform>(entity);
                glm::vec3 front_direction;
                front_direction.x = cos(glm::radians(transform.yaw)) * cos(glm::radians(transform.pitch));
                front_direction.z = sin(glm::radians(transform.yaw)) * cos(glm::radians(transform.pitch));
                front_direction.y = sin(glm::radians(transform.pitch));

                transform.front = glm::normalize(front_direction);
                // calculate camera right
                transform.right = glm::normalize(glm::cross(transform.front, glm::vec3(0.0f, 1.0f, 0.0f)));
                transform.up = glm::normalize(glm::cross(transform.right, transform.front));
            }
        }
    }
}
