//
// Created by Amo on 2022/6/18.
//

#ifndef SYMOCRAFT_CHARACTER_SYSTEM_H
#define SYMOCRAFT_CHARACTER_SYSTEM_H
#include <symocraft/ecs/registry.h>


namespace SymoCraft
{
    namespace ECS
    {
        class Registry;
    }

    namespace Character
    {
        namespace Player
        {
            void Update(ECS::Registry& registry);
            void SyncCamera(ECS::Registry& registry, ECS::EntityId camera_entity);
        }
    }
}

#endif //SYMOCRAFT_CHARACTER_SYSTEM_H
