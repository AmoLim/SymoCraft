//
// Created by Amo on 2022/6/17.
//

#ifndef SYMOCRAFT_COMPONENT_CONTAINER_H
#define SYMOCRAFT_COMPONENT_CONTAINER_H
#include "symocraft/foundation/types.h"
#include "symocraft/foundation/memory.h"
#include "symocraft/foundation/diagnostics.h"
#include <type_traits>
#include <typeinfo>
#include "internal.h"

namespace SymoCraft
{
    namespace ECS
    {
        typedef uint32 EntityIndex;
        typedef uint32 EntityVersion;
        typedef uint64 EntityId;
        typedef uint32 ComponentIndex;

        namespace Internal
        {
            extern int32 m_counter;
            template<typename T>
            int32 GetComponentType()
            {
                static int32 m_component_type = Internal::m_counter++;
                return m_component_type;
            }

            const int kSpareSetPoolSize = 8;
            const int32 kMaxNumComponents = 256;

            struct SparseSetPool
            {
                EntityIndex start_entity_index;
                ComponentIndex component_index_in_data[kSpareSetPoolSize];

                void Init();
            };

            class ComponentContainer
            {
            public:

                void InitRaw(std::size_t size, EntityIndex entity_index, int type)
                {
                    component_type = type;
                    component_size = size;
                    num_pools = 1;
                    pools = (SparseSetPool*)AmoMemory_Allocate(sizeof(SparseSetPool));
                    pools[0].Init();
                    pools[0].start_entity_index = GetPoolAlignedIndex(entity_index);
                    num_components = 0;
                    max_num_components = kSpareSetPoolSize;
                    data = (char*)AmoMemory_Allocate(component_size * max_num_components);
                    entities = (EntityIndex*)AmoMemory_Allocate(sizeof(EntityIndex) * max_num_components);
                }

                template<typename T>
                static ComponentContainer DefaultComponentContainer(EntityIndex entity_index)
                {
                    ComponentContainer res{};
                    res.component_type = GetComponentType<T>();
                    res.template Init<T>(entity_index);

                    return res;
                }

                template<typename T>
                void Init(EntityIndex entity_index)
                {
                    AmoLogger_Assert(std::is_standard_layout<T>() && std::is_trivial<T>()
                    , "Component must be POD. Component %s is not POD.", typeid(T).name());
                    component_size = sizeof(T);

                    num_pools = 1;
                    pools = (SparseSetPool*) AmoMemory_Allocate(num_pools * sizeof(SparseSetPool));
                    pools[0].Init();
                    pools[0].start_entity_index = GetPoolAlignedIndex(entity_index);

                    num_components = 0;
                    max_num_components = Internal::kSpareSetPoolSize;
                    data = (char*) AmoMemory_Allocate(component_size * max_num_components);
                    entities = (EntityIndex*) AmoMemory_Allocate(sizeof(EntityIndex) * max_num_components);
                }


                void Free();

                inline uint32 GetPoolAlignedIndex(EntityIndex index) const
                {
                    // we need to make sure pools al
                    return (index / kSpareSetPoolSize) * kSpareSetPoolSize;
                }

                // Get the pool where entity in
                SparseSetPool* GetPool(EntityIndex index) const;

                // Get the component data
                // Parameters: entity index
                template<typename T>
                inline T* Get(EntityIndex index) const
                {
                    SparseSetPool* pool = GetPool(index);
                    if (!pool)
                    {
                        AmoLogger_Error("Invalid entity '%d' for component '%d'", index, component_type);
                        return nullptr;
                    }

                    auto dense_array_component_index =
                            (ComponentIndex)pool->component_index_in_data[index - pool->start_entity_index];
                    AmoLogger_Assert((dense_array_component_index < num_components && dense_array_component_index >=0)
                    , "Invalid dense array index");
                    return (T*)(data + dense_array_component_index * component_size);
                }

                // byte wise Get
                inline uint8* Get(EntityIndex index) const
                {
                    SparseSetPool* pool = GetPool(index);
                    if (!pool)
                    {
                        AmoLogger_Error("Invalid entity '%d' for component '%d'", index, component_type);
                        return nullptr;
                    }

                    auto dense_array_component_index =
                            (ComponentIndex)pool->component_index_in_data[index - pool->start_entity_index];
                    AmoLogger_Assert((dense_array_component_index < num_components && dense_array_component_index >=0)
                    , "Invalid dense array index");
                    return (uint8*)(data + dense_array_component_index * component_size);
                }

                // Add component to entity
                // Parameters: entity id, component data
                template<typename T>
                void Add(EntityId entity, const T& component)
                {
                    EntityIndex entity_index = GetEntityIndex(entity);
                    SparseSetPool* pool = GetPool(entity_index);
                    // no satisfying pool, add one
                    if (!pool)
                    {
                        const int new_num_pools = num_pools + 1;
                        // reallocate pools
                        SparseSetPool* new_pools = (SparseSetPool*) AmoMemory_ReAlloc(pools
                        , new_num_pools * sizeof(SparseSetPool));
                        if (!new_pools)
                        {
                            AmoLogger_Error("Failed to allocate memory for new sparse set pool for component '%d'", component_type);
                            return;
                        }
                        num_pools = new_num_pools;
                        pools = new_pools;
                        pools[num_pools - 1].Init();
                        pools[num_pools - 1].start_entity_index = GetPoolAlignedIndex(entity_index);
                        pool = &pools[num_pools - 1];
                    }

                    // allocate memory for data
                    ComponentIndex next_component_index = num_components;
                    if (next_component_index >= max_num_components)
                    {
                        int new_max_num_components = max_num_components * 2;
                        char* new_component_memory = (char*) AmoMemory_ReAlloc(data
                        , component_size * new_max_num_components);
                        EntityIndex* new_entity_memory = (EntityIndex*) AmoMemory_ReAlloc(entities
                        , sizeof(EntityIndex) * new_max_num_components);
                        if (!new_component_memory || !new_entity_memory)
                        {
                            AmoMemory_Free(new_entity_memory);
                            AmoMemory_Free(new_component_memory);
                            AmoLogger_Error("Failed to allocate new memory for "
                                            "component pool or entities for component '%d'"
                            , component_type);
                            return;
                        }
                        data = new_component_memory;
                        entities = new_entity_memory;
                        max_num_components = new_max_num_components;
                    }

                    pool->component_index_in_data[entity_index - pool->start_entity_index] = next_component_index;
                    AmoBase::AmoMemory_CopyMem(data + next_component_index * component_size, (void*)&component, component_size);
                    entities[next_component_index] = entity_index;
                    num_components++;
                }

                // Add a null component to entity
                // Parameters: entity id
                void Add(EntityId entity);

                // Add or Get
                // return the data pointer
                // Parameters: entity id
                template<typename T>
                T* AddOrGet(EntityId entity)
                {
                    if (!IsComponentExist(entity))
                        Add<T>(entity, T{});
                    return Get<T>(Internal::GetEntityIndex(entity));
                }

                // byte wise Add or Get
                // return the data pointer
                // Parameters: entity id
                uint8* AddOrGet(EntityId entity);

                // Check if a component of an entity exist
                // Exists return true
                // Parameters: entity id
                inline bool IsComponentExist(EntityId entity) const
                {
                    const EntityIndex index = GetEntityIndex(entity);
                    SparseSetPool* pool = GetPool(index);
                    if (!pool) return false;

                    return pool->component_index_in_data[index - pool->start_entity_index] != UINT32_MAX;
                }

                // Remove a component of an entity
                // Parameters: entity id
                void Remove(EntityId entity);

                // Get component size
                inline size_t GetComponentSize() const
                {
                    return component_size;
                }
            private:
                int component_type;
                uint32 max_num_components;
                uint32 num_components;
                int num_pools;

                SparseSetPool* pools;
                // Numbers of entities is always equal to num_components for a sparse set
                EntityIndex* entities;
                char * data;
                size_t component_size;
            };
        }

    }
}

#endif //SYMOCRAFT_COMPONENT_CONTAINER_H
