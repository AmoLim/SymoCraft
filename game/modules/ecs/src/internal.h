//
// Created by Amo on 2022/6/17.
//

#ifndef SYMOCRAFT_ECS_INTERNAL_H
#define SYMOCRAFT_ECS_INTERNAL_H
#include "symocraft/foundation/types.h"

namespace SymoCraft::ECS
    {
        typedef uint32 EntityIndex;
        typedef uint32 EntityVersion;
        typedef uint64 EntityId;
        extern EntityId null_entity;

        namespace Internal
        {
            inline EntityId CreateEntityId(EntityIndex index, EntityVersion version)
            {
                return ((EntityId)version << 32 | (EntityId)index);
            }

            inline EntityIndex GetEntityIndex(EntityId id)
            {
                return (EntityIndex)id;
            }

            inline EntityVersion GetEntityVersion(EntityId id)
            {
                return (EntityVersion)(id >> 32);
            }

            /*
            // check if the entity is
             a null entity
            inline bool IsEntityValid(EntityId id)
            {
                return GetEntityIndex(id) != GetEntityIndex(null_entity);
            }
             */
        }

    }

#endif //SYMOCRAFT_ECS_INTERNAL_H
