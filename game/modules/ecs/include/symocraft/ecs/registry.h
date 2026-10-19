#pragma once
#include "symocraft/foundation/types.h"
#include "symocraft/foundation/diagnostics.h"
#include <bitset>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>

struct RawMemory;

namespace SymoCraft::ECS {
    using EntityIndex = uint32;
    using EntityVersion = uint32;
    using EntityId = uint64;
    using ComponentIndex = uint32;
    inline constexpr int MaxComponents = 256;
    extern EntityId null_entity;
    namespace Detail {
        int32 NextComponentType();
        template<class T> int32 ComponentType() {
            static const int32 type = NextComponentType();
            return type;
        }
    }
    template<class... Components> class RegistryViewer;
    class Iterator;

    class Registry {
    public:
        Registry();
        ~Registry();
        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;
        void Free();
        void Clear();
        EntityId CreateEntity();
        int NumComponents(EntityId entity) const;
        bool IsEntityValid(EntityId entity) const;
        void DestroyEntity(EntityId entity);
        template<class T> void RegisterComponent(const char* debug_name) {
            static_assert(std::is_standard_layout_v<T> && std::is_trivial_v<T>,
                          "Legacy ECS components must be POD");
            RegisterType(Detail::ComponentType<T>(), sizeof(T), debug_name);
        }
        // Type IDs remain process-wide; this does not register storage or relax its order.
        template<class T> bool IsComponentRegistered() const {
            return IsTypeRegistered(Detail::ComponentType<T>());
        }
        template<class T> T& AddComponent(EntityId entity) {
            auto* component = AddOrGetComponentByType(entity, Detail::ComponentType<T>());
            if (!component)
                throw std::runtime_error("ECS AddComponent: invalid entity or unavailable component");
            return *reinterpret_cast<T*>(component);
        }
        bool HasComponentByType(EntityId entity, int32 component_type) const;
        template<class T> bool HasComponent(EntityId entity) const {
            return HasComponentByType(entity, Detail::ComponentType<T>());
        }
        uint8* GetComponentByType(EntityId entity, int32 component_type) const;
        template<class T> T& GetComponent(EntityId entity) const {
            auto* component = GetComponentByType(entity, Detail::ComponentType<T>());
            if (!component)
                throw std::runtime_error("ECS GetComponent: invalid entity or missing component");
            return *reinterpret_cast<T*>(component);
        }
        uint8* AddOrGetComponentByType(EntityId entity, int32 component_id);
        template<class T> void RemoveComponent(EntityId entity) {
            RemoveType(entity, Detail::ComponentType<T>());
        }
        void RemoveAllComponent(EntityId entity);
        template<class... Components> RegistryViewer<Components...> View() {
            return RegistryViewer<Components...>(*this);
        }
    private:
        friend class Iterator;
        template<class...> friend class RegistryViewer;
        struct Storage;
        std::unique_ptr<Storage> storage_;
        void RegisterType(int32 type, std::size_t size, const char* debug_name);
        bool IsTypeRegistered(int32 type) const;
        void RemoveType(EntityId entity, int32 type);
        std::size_t EntitySlots() const;
        EntityId EntityAt(std::size_t index) const;
        // Retained, inactive legacy serialization; not a supported persistence API.
        RawMemory Serialize();
        void Deserialize(RawMemory& memory);
    };

        class Iterator
        {
        public:
            // constructor
            // Parameters: Registry reference, entity index, components need, is searching for all the component
            Iterator( Registry& reg, EntityIndex index
                       , std::bitset<MaxComponents>com_need
                       , bool all);

            // Indirect operator = Indexing operator
            // Return the complete entity ID, including its generation.
            EntityId operator*() const;

            // identity operator
            bool operator ==(Iterator& other) const;

            bool operator !=(Iterator& other) const;

            // Increment operator
            // skip the unqualified elements
            Iterator& operator++();

        private:
            Registry& registry;
            EntityIndex entity_index;
            std::bitset<MaxComponents>components_need;
            bool _is_searching_all;

            // Is it a valid index and has the correct component;
            bool IsIndexValid();
        };

        template<typename... Components>
        class RegistryViewer
        {
            friend class Iterator;
        public:
            explicit RegistryViewer(Registry &reg)
            : registry(reg)
            {
                _is_searching_all = sizeof...(Components) == 0;
                if (!_is_searching_all)
                {
                    int component_type[] = {0 , Detail::ComponentType<Components>() ...};
                    for (int i = 1; i <= (sizeof...(Components)); i++) {
                        if (!registry.IsTypeRegistered(component_type[i]))
                            throw std::logic_error("ECS View: component type is not registered");
                        components_need.set(component_type[i]);
                    }
                }
            }

            const Iterator begin() const
            {
                EntityIndex first_index = 0;
                for (; first_index < registry.EntitySlots() &&
                        (
                                !HasRequiredComponents(registry, components_need, registry.EntityAt(first_index)) ||
                                !registry.IsEntityValid(registry.EntityAt(first_index))
                                );)
                    first_index++;
                return Iterator(registry, first_index, components_need, _is_searching_all);
            }

            const Iterator end() const
            {
                return Iterator(registry, (EntityIndex)registry.EntitySlots(), components_need, _is_searching_all);
            }

        private:
            Registry& registry;
            std::bitset<MaxComponents> components_need;
            bool _is_searching_all;

            static bool HasRequiredComponents(Registry& reg, const std::bitset<MaxComponents> &need,
                                             EntityId entity)
            {
                bool has_required_components = true;
                for (int i = 0; i < need.size(); i++)
                    if (need.test(i) && !reg.HasComponentByType(entity, i))
                    {
                        has_required_components = false;
                        break;
                    }
                return has_required_components;
            }
        };
}
