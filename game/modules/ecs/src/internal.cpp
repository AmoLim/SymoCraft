//
// Created by Amo on 2022/6/17.
//

#include "component_container.h"

namespace SymoCraft::ECS
    {
        EntityId null_entity = Internal::CreateEntityId(UINT32_MAX, 0);

        namespace Internal
        {
            int32 m_counter = 0;

            inline bool IsNull(EntityId entity)
            {
                return Internal::GetEntityIndex(entity) == Internal::GetEntityIndex(null_entity);
            }

            // ----------------------------------------------------------------------
            // SparseSetPool Implement
            void SparseSetPool::Init()
            {
                for (unsigned int & component_index : component_index_in_data)
                    component_index = UINT32_MAX;
                // UINT32_MAX means null index
            }

            // ----------------------------------------------------------------------
            // ComponentContainer Implement
            void ComponentContainer::Free()
            {
                if (pools != nullptr)
                {
                    AmoMemory_Free(pools);
                    pools = nullptr;
                }

                if (entities != nullptr)
                {
                    AmoMemory_Free(entities);
                    entities = nullptr;
                }

                if (data != nullptr)
                {
                    AmoMemory_Free(data);
                    data = nullptr;
                }

                max_num_components = 0;
                num_components = 0;
                num_pools = 0;
            }

            SparseSetPool* ComponentContainer::GetPool(EntityIndex index) const
            {
                for (int i = 0; i < num_pools; i++)
                {
                    SparseSetPool& pool = pools[i];
                    if (pool.start_entity_index <= index && pool.start_entity_index + kSpareSetPoolSize > index)
                    {
                        return &pool;
                    }
                }

                return nullptr;
            }

            void ComponentContainer::Add(EntityId entity)
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
                AmoBase::AmoMemory_ZeroMem(data + next_component_index * component_size, component_size);
                entities[next_component_index] = entity_index;
                num_components++;
            }

            uint8* ComponentContainer::AddOrGet(EntityId entity)
            {
                if (!IsComponentExist(entity))
                    Add(entity);
                return Get(Internal::GetEntityIndex(entity));
            }

            void ComponentContainer::Remove(EntityId entity)
            {
                const EntityIndex entity_index = GetEntityIndex(entity);
                SparseSetPool* pool = GetPool(entity_index);
                if (!pool || entity_index >= num_components)
                {
                    AmoLogger_Warning("Tried to remove an entity '%d' that did not exist for component '%d'"
                                      , entity_index, component_type);
                    return;
                }

                auto dense_array_component_index =
                        (ComponentIndex)pool->component_index_in_data[entity_index - pool->start_entity_index];
                if (dense_array_component_index < num_components - 1 && num_components > 2)
                {
                    // If the component data is not already at the end of the component array
                    // Swap it with the component at the end of the array and update all indices accordingly
                    EntityIndex last_entity = entities[ num_components - 1];
                    SparseSetPool* pool_to_swap = GetPool(last_entity);

                    AmoLogger_Assert( pool_to_swap != nullptr, "Invalid entity was somehow stored in the dense array.");

                    pool_to_swap -> component_index_in_data[ last_entity - pool_to_swap -> start_entity_index]
                        = dense_array_component_index;
                    entities[dense_array_component_index] = entities[num_components - 1];
                    // Swap Component
                    data[dense_array_component_index] = data[num_components - 1];
                }

                // Mark this entity as gone and decrease the num_components
                pool -> component_index_in_data[entity_index - pool->start_entity_index] = UINT32_MAX;
                num_components--;
            }


        }


    }

