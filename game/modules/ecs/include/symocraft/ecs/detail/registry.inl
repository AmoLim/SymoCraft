#pragma once
#include <algorithm>
#include <functional>
#include <vector>

namespace SymoCraft::ECS::Detail {
    template<class... T> struct UniqueTypes : std::true_type {};
    template<class T, class... Rest> struct UniqueTypes<T, Rest...>
        : std::bool_constant<(!std::is_same_v<std::remove_const_t<T>, std::remove_const_t<Rest>> && ...)
                             && UniqueTypes<Rest...>::value> {};
    template<class Fn, class... Args> bool Visit(Fn& visitor, Args&&... args) {
        using Result = std::invoke_result_t<Fn&, Args...>;
        static_assert(std::is_same_v<Result, void> || std::is_same_v<Result, VisitControl>,
                      "SYMOCRAFT_ECS_VISITOR: return void or VisitControl");
        if constexpr (std::is_same_v<Result, void>) {
            std::invoke(visitor, std::forward<Args>(args)...);
            return false;
        } else return std::invoke(visitor, std::forward<Args>(args)...) == VisitControl::Stop;
    }
}

namespace SymoCraft::ECS {
    template<class T> void Registry::CheckBare() {
        static_assert(std::is_same_v<T, std::remove_cvref_t<T>> && std::is_object_v<T> && !std::is_array_v<T>,
                      "SYMOCRAFT_ECS_BARE: bare object component required");
        static_assert(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>
                      && std::is_nothrow_destructible_v<T>,
                      "SYMOCRAFT_ECS_LIFETIME: nonthrowing move and destruction required");
        static_assert(entt::component_traits<std::remove_cvref_t<T>>::page_size != 0,
                      "SYMOCRAFT_ECS_OBJECT: real object storage required");
    }
    template<class T> void Registry::CheckReadable() {
        static_assert(!std::is_reference_v<T> && !std::is_volatile_v<T>,
                      "SYMOCRAFT_ECS_READ: only T or const T supported");
        CheckBare<std::remove_const_t<T>>();
    }
    template<class T> void Registry::RegisterComponent(std::string_view name) {
        CheckBare<T>();
        CheckStructural();
        RegisterType(&Detail::TypeToken<T>, entt::type_hash<std::remove_cvref_t<T>>::value(), name,
                     [](entt::registry& native) { (void)native.storage<std::remove_cvref_t<T>>(); });
    }
    template<class T> void Registry::RegisterComponent(const char* name) {
        CheckBare<T>();
        CheckStructural();
        if (!name) throw RegistryError(RegistryErrorCode::InvalidName);
        RegisterComponent<T>(std::string_view(name));
    }
    template<class T> bool Registry::IsComponentRegistered() const {
        CheckBare<T>();
        CheckRead();
        return IsTypeRegistered(&Detail::TypeToken<T>);
    }
    template<class T, class... Args> T& Registry::Emplace(EntityId id, Args&&... args) {
        CheckBare<T>();
        static_assert(std::is_constructible_v<T, Args...>, "SYMOCRAFT_ECS_CTOR: invalid component constructor");
        CheckStructural();
        const auto entity = RequireEntity(id);
        RequireType(&Detail::TypeToken<T>);
        auto& pool = Native().storage<std::remove_cvref_t<T>>();
        if (pool.contains(entity)) throw RegistryError(RegistryErrorCode::DuplicateComponent);
        MutationGuard guard(*this);
        const auto before = pool.size();
        try { return pool.emplace(entity, std::forward<Args>(args)...); }
        catch (...) {
            bool inconsistent = pool.contains(entity);
            // A rolled-back in-place insertion may retain a tombstone, not a live component.
            if constexpr (!entt::component_traits<std::remove_cvref_t<T>>::in_place_delete)
                inconsistent = inconsistent || pool.size() != before;
            if (inconsistent) Fail(RegistryOperation::Emplace);
            throw;
        }
    }
    template<class T> T& Registry::GetOrAdd(EntityId id) {
        CheckBare<T>();
        static_assert(std::is_default_constructible_v<T>, "SYMOCRAFT_ECS_DEFAULT: default constructor required");
        CheckStructural();
        const auto entity = RequireEntity(id);
        RequireType(&Detail::TypeToken<T>);
        auto& pool = Native().storage<std::remove_cvref_t<T>>();
        if (pool.contains(entity)) return pool.get(entity);
        return Emplace<T>(id);
    }
    template<class T> bool Registry::RemoveComponent(EntityId id) {
        CheckBare<T>();
        CheckStructural();
        const auto entity = RequireEntity(id);
        RequireType(&Detail::TypeToken<T>);
        auto& pool = Native().storage<std::remove_cvref_t<T>>();
        if (!pool.contains(entity)) return false;
        MutationGuard guard(*this);
        try { pool.erase(entity); }
        catch (...) { Fail(RegistryOperation::Remove); throw; }
        return true;
    }
    template<class T> bool Registry::HasComponent(EntityId id) const {
        CheckBare<T>();
        CheckRead();
        const auto entity = RequireEntity(id);
        RequireType(&Detail::TypeToken<T>);
        return Native().storage<std::remove_cvref_t<T>>()->contains(entity);
    }
    template<class T> std::add_pointer_t<T> Registry::TryGetComponent(EntityId id) {
        CheckReadable<T>();
        CheckRead();
        const auto entity = RequireEntity(id);
        RequireType(&Detail::TypeToken<std::remove_const_t<T>>);
        auto& pool = Native().storage<std::remove_cvref_t<T>>();
        return pool.contains(entity) ? std::addressof(pool.get(entity)) : nullptr;
    }
    template<class T> const std::remove_cvref_t<T>* Registry::TryGetComponent(EntityId id) const {
        CheckReadable<T>();
        CheckRead();
        const auto entity = RequireEntity(id);
        RequireType(&Detail::TypeToken<std::remove_const_t<T>>);
        const auto* pool = Native().storage<std::remove_cvref_t<T>>();
        return pool->contains(entity) ? std::addressof(pool->get(entity)) : nullptr;
    }
    template<class T> T& Registry::GetComponent(EntityId id) {
        auto* component = TryGetComponent<T>(id);
        if (!component) throw RegistryError(RegistryErrorCode::MissingComponent);
        return *component;
    }
    template<class T> const std::remove_cvref_t<T>& Registry::GetComponent(EntityId id) const {
        const auto* component = TryGetComponent<T>(id);
        if (!component) throw RegistryError(RegistryErrorCode::MissingComponent);
        return *component;
    }
    template<class Self, class... T, class... X> void Registry::CheckQuery(Self& self, Exclude<X...>) {
        (CheckReadable<T>(), ...);
        (CheckBare<X>(), ...);
        static_assert(Detail::UniqueTypes<T..., X...>::value,
                      "SYMOCRAFT_ECS_QUERY_TYPES: duplicate or overlapping component types");
        (self.RequireType(&Detail::TypeToken<std::remove_const_t<T>>), ...);
        (self.RequireType(&Detail::TypeToken<X>), ...);
    }
    template<class Self, class T> decltype(auto) Registry::QueryValue(Self& self, entt::entity entity) {
        if constexpr (std::is_const_v<Self>) return self.Native().template storage<std::remove_const_t<T>>()->get(entity);
        else return static_cast<T&>(self.Native().template storage<std::remove_const_t<T>>().get(entity));
    }
    template<class Self, class... T, class... X, class Fn>
    QueryResult Registry::RunEach(Self& self, Exclude<X...> excluded, Fn& visitor) {
        QueryGuard guard(self);
        CheckQuery<Self, T...>(self, excluded);
        const auto& native = std::as_const(self.Native());
        if constexpr (sizeof...(T) == 0) {
            // No required pool exists to drive this query: include component-free entities.
            for (auto [entity] : native.template storage<entt::entity>()->each()) {
                if ((!native.template storage<X>()->contains(entity) && ...) &&
                    Detail::Visit(visitor, self.ProjectId(entity)))
                    return QueryResult::Stopped;
            }
        } else {
            const auto view = native.template view<std::remove_const_t<T>...>(entt::exclude<X...>);
            for (const auto entity : view)
                if (Detail::Visit(visitor, self.ProjectId(entity), QueryValue<Self, T>(self, entity)...))
                    return QueryResult::Stopped;
        }
        return QueryResult::Completed;
    }
    template<class Self, class... T, class... X, class Key, class Fn>
    QueryResult Registry::RunOrdered(Self& self, Exclude<X...> excluded, Key& key, Fn& visitor) {
        QueryGuard guard(self);
        CheckQuery<Self, T...>(self, excluded);
        using KeyResult = std::invoke_result_t<Key&, EntityId, const std::remove_const_t<T>&...>;
        using Value = std::remove_cvref_t<KeyResult>;
        static_assert(!std::is_reference_v<KeyResult> && !std::is_pointer_v<Value>,
                      "SYMOCRAFT_ECS_KEY: return an owned key value");
        struct Item { Value key; EntityId id; };
        std::vector<Item> items;
        const Registry& readable = self;
        auto collect = [&](EntityId id, const std::remove_const_t<T>&... components) {
            items.push_back({std::invoke(key, id, components...), id});
        };
        RunEach<const Registry, const std::remove_const_t<T>...>(readable, excluded, collect);
        const auto less = [](const Item& a, const Item& b) { return std::less<>{}(a.key, b.key); };
        std::sort(items.begin(), items.end(), less);
        for (std::size_t index = 1; index < items.size(); ++index)
            if (!less(items[index - 1], items[index]) && !less(items[index], items[index - 1]))
                throw RegistryError(RegistryErrorCode::OrderingTie);
        for (const auto& item : items) {
            if (Detail::Visit(visitor, item.id, QueryValue<Self, T>(self, self.RequireEntity(item.id))...))
                return QueryResult::Stopped;
        }
        return QueryResult::Completed;
    }
    template<class... T, class Fn> QueryResult Registry::Each(Fn&& visitor) {
        return Each<T...>(Exclude<>{}, std::forward<Fn>(visitor));
    }
    template<class... T, class Fn> QueryResult Registry::Each(Fn&& visitor) const {
        return Each<T...>(Exclude<>{}, std::forward<Fn>(visitor));
    }
    template<class... T, class... X, class Fn> QueryResult Registry::Each(Exclude<X...> excluded, Fn&& visitor) {
        return RunEach<Registry, T...>(*this, excluded, visitor);
    }
    template<class... T, class... X, class Fn> QueryResult Registry::Each(Exclude<X...> excluded, Fn&& visitor) const {
        return RunEach<const Registry, T...>(*this, excluded, visitor);
    }
    template<class... T, class Key, class Fn> QueryResult Registry::EachOrdered(Key&& key, Fn&& visitor) {
        return EachOrdered<T...>(Exclude<>{}, std::forward<Key>(key), std::forward<Fn>(visitor));
    }
    template<class... T, class Key, class Fn> QueryResult Registry::EachOrdered(Key&& key, Fn&& visitor) const {
        return EachOrdered<T...>(Exclude<>{}, std::forward<Key>(key), std::forward<Fn>(visitor));
    }
    template<class... T, class... X, class Key, class Fn>
    QueryResult Registry::EachOrdered(Exclude<X...> excluded, Key&& key, Fn&& visitor) {
        return RunOrdered<Registry, T...>(*this, excluded, key, visitor);
    }
    template<class... T, class... X, class Key, class Fn>
    QueryResult Registry::EachOrdered(Exclude<X...> excluded, Key&& key, Fn&& visitor) const {
        return RunOrdered<const Registry, T...>(*this, excluded, key, visitor);
    }
}
