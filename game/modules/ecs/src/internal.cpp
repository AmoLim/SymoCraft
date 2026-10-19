//
// Created by Amo on 2022/6/17.
//

#include "component_container.h"
#include <limits>
#include <new>
#include <stdexcept>

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
            namespace {
                std::size_t CheckedBytes(std::size_t size, std::size_t count) {
                    if (size == 0 || count > (std::numeric_limits<std::size_t>::max)() / size)
                        throw std::length_error("ECS component allocation size overflow");
                    return size * count;
                }
                template<class T> T* Allocate(std::size_t bytes) {
                    auto* result = static_cast<T*>(AmoMemory_Allocate(bytes));
                    if (!result) throw std::bad_alloc();
                    return result;
                }
            }

            void ComponentContainer::InitRaw(std::size_t size, EntityIndex entity_index, int type) {
                const auto bytes = CheckedBytes(size, kSpareSetPoolSize);
                ComponentContainer pending;
                pending.component_type = type;
                pending.component_size = size;
                pending.pools = Allocate<SparseSetPool>(sizeof(SparseSetPool));
                pending.pools[0].Init();
                pending.pools[0].start_entity_index = GetPoolAlignedIndex(entity_index);
                pending.num_pools = 1;
                pending.max_num_components = kSpareSetPoolSize;
                pending.data = Allocate<char>(bytes);
                pending.entities = Allocate<EntityIndex>(CheckedBytes(sizeof(EntityIndex), kSpareSetPoolSize));
                Swap(pending);
            }

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
                    if (pool.start_entity_index <= index && index - pool.start_entity_index < kSpareSetPoolSize)
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
                    if (num_pools == (std::numeric_limits<int>::max)())
                        throw std::length_error("ECS sparse pool count overflow");
                    const int new_num_pools = num_pools + 1;
                    // reallocate pools
                    SparseSetPool* new_pools = (SparseSetPool*) AmoMemory_ReAlloc(pools
                    , CheckedBytes(sizeof(SparseSetPool), new_num_pools));
                    if (!new_pools)
                    {
                        throw std::bad_alloc();
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
                    if (max_num_components == 0 || max_num_components > UINT32_MAX / 2)
                        throw std::length_error("ECS component capacity overflow");
                    const auto new_max_num_components = max_num_components * 2;
                    const auto data_bytes = CheckedBytes(component_size, new_max_num_components);
                    const auto entity_bytes = CheckedBytes(sizeof(EntityIndex), new_max_num_components);
                    // Stage both buffers; neither old pointer can be lost on the second failure.
                    ComponentContainer pending;
                    pending.data = Allocate<char>(data_bytes);
                    pending.entities = Allocate<EntityIndex>(entity_bytes);
                    AmoBase::AmoMemory_CopyMem(pending.data, data, component_size * num_components);
                    AmoBase::AmoMemory_CopyMem(pending.entities, entities, sizeof(EntityIndex) * num_components);
                    auto* new_component_memory = std::exchange(pending.data, data);
                    auto* new_entity_memory = std::exchange(pending.entities, entities);
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

