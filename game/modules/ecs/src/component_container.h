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
#include <utility>
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

                ComponentContainer() = default;
                ~ComponentContainer() { Free(); }
                ComponentContainer(const ComponentContainer&) = delete;
                ComponentContainer& operator=(const ComponentContainer&) = delete;
                ComponentContainer(ComponentContainer&& other) noexcept { Swap(other); }
                ComponentContainer& operator=(ComponentContainer&& other) noexcept {
                    if (this != &other) { Free(); Swap(other); }
                    return *this;
                }
                void InitRaw(std::size_t size, EntityIndex entity_index, int type);

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
                    InitRaw(sizeof(T), entity_index, component_type);
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
                    return reinterpret_cast<T*>(Get(index));
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
                    if (dense_array_component_index >= num_components)
                        return nullptr;
                    return (uint8*)(data + dense_array_component_index * component_size);
                }

                // Add component to entity
                // Parameters: entity id, component data
                template<typename T>
                void Add(EntityId entity, const T& component)
                {
                    Add(entity);
                    AmoBase::AmoMemory_CopyMem(Get(GetEntityIndex(entity)), (void*)&component, component_size);
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
                void Swap(ComponentContainer& other) noexcept {
                    using std::swap;
                    swap(component_type, other.component_type);
                    swap(max_num_components, other.max_num_components);
                    swap(num_components, other.num_components);
                    swap(num_pools, other.num_pools);
                    swap(pools, other.pools);
                    swap(entities, other.entities);
                    swap(data, other.data);
                    swap(component_size, other.component_size);
                }
                int component_type = 0;
                uint32 max_num_components = 0;
                uint32 num_components = 0;
                int num_pools = 0;

                SparseSetPool* pools = nullptr;
                // Numbers of entities is always equal to num_components for a sparse set
                EntityIndex* entities = nullptr;
                char* data = nullptr;
                size_t component_size = 0;
            };
        }

    }
}

#endif //SYMOCRAFT_COMPONENT_CONTAINER_H
