#pragma once
#include "symocraft/ecs/detail/entt_config.h"
#include "symocraft/ecs/entity.h"
#include <cstddef>
#include <exception>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

namespace SymoCraft::ECS {
    enum class RegistryErrorCode {
        None, Failed, InvalidContext, InvalidEntity, UnregisteredType, InvalidName,
        TypeCollision, DuplicateComponent, MissingComponent, IdentityExhausted,
        EntityCapacityExhausted, GenerationExhausted, OrderingTie, StorageInconsistent
    };
    class RegistryError : public std::exception {
    public:
        explicit RegistryError(RegistryErrorCode code) noexcept : code_(code) {}
        RegistryErrorCode Code() const noexcept { return code_; }
        const char* what() const noexcept override;
    private:
        RegistryErrorCode code_;
    };
    enum class RegistryHealth { Healthy, Failed };
    enum class RegistryOperation { None, Create, Destroy, Clear, Emplace, Remove };
    struct RegistryStatus {
        RegistryHealth health = RegistryHealth::Healthy;
        RegistryOperation operation = RegistryOperation::None;
        RegistryErrorCode category = RegistryErrorCode::None;
        bool entity_facts_known = true;
    };
    enum class VisitControl { Continue, Stop };
    enum class QueryResult { Completed, Stopped };
    template<class... T> struct Exclude {};
    namespace Detail {
        template<class T> inline const unsigned char TypeToken{};
        struct RegistryTestAccess;
    }

    // External serial access is required, including across different Registry instances.
    class Registry {
    public:
        Registry();
        ~Registry() noexcept;
        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;
        Registry(Registry&&) = delete;
        Registry& operator=(Registry&&) = delete;
        EntityId CreateEntity();
        void DestroyEntity(EntityId entity);
        void Clear();
        bool IsEntityValid(EntityId entity) const;
        std::size_t NumComponents(EntityId entity) const;
        RegistryStatus GetStatus() const noexcept;
        template<class T> void RegisterComponent(std::string_view name);
        template<class T> void RegisterComponent(const char* name);
        template<class T> bool IsComponentRegistered() const;
        template<class T, class... Args> T& Emplace(EntityId entity, Args&&... args);
        template<class T> T& GetOrAdd(EntityId entity);
        template<class T> bool RemoveComponent(EntityId entity);
        template<class T> bool HasComponent(EntityId entity) const;
        // Borrow only until structural storage work begins, even if that work fails.
        // Constructor arguments must own their inputs before a structural call.
        template<class T> T& GetComponent(EntityId entity);
        template<class T> const std::remove_cvref_t<T>& GetComponent(EntityId entity) const;
        template<class T> std::add_pointer_t<T> TryGetComponent(EntityId entity);
        template<class T> const std::remove_cvref_t<T>* TryGetComponent(EntityId entity) const;
        // Callback references are valid only for that callback. Structural entries are forbidden.
        template<class... T, class Fn> QueryResult Each(Fn&& visitor);
        template<class... T, class Fn> QueryResult Each(Fn&& visitor) const;
        template<class... T, class... X, class Fn> QueryResult Each(Exclude<X...>, Fn&& visitor);
        template<class... T, class... X, class Fn> QueryResult Each(Exclude<X...>, Fn&& visitor) const;
        // Keys must own their data and satisfy strict weak ordering; equal keys are rejected.
        template<class... T, class Key, class Fn> QueryResult EachOrdered(Key&& key, Fn&& visitor);
        template<class... T, class Key, class Fn> QueryResult EachOrdered(Key&& key, Fn&& visitor) const;
        template<class... T, class... X, class Key, class Fn>
        QueryResult EachOrdered(Exclude<X...>, Key&& key, Fn&& visitor);
        template<class... T, class... X, class Key, class Fn>
        QueryResult EachOrdered(Exclude<X...>, Key&& key, Fn&& visitor) const;
    private:
        friend struct Detail::RegistryTestAccess;
        struct Storage;
        std::unique_ptr<Storage> storage_;
        class MutationGuard {
        public:
            explicit MutationGuard(Registry&) noexcept;
            ~MutationGuard();
            MutationGuard(const MutationGuard&) = delete;
        private:
            Registry& owner_;
        };
        class QueryGuard {
        public:
            explicit QueryGuard(const Registry&);
            ~QueryGuard();
            QueryGuard(const QueryGuard&) = delete;
        private:
            const Registry& owner_;
        };
        void CheckRead() const;
        void CheckStructural() const;
        void Fail(RegistryOperation) noexcept;
        bool IsTypeRegistered(const void*) const;
        void RequireType(const void*) const;
        void RegisterType(const void*, entt::id_type, std::string_view, void (*)(entt::registry&));
        entt::registry& Native() noexcept;
        const entt::registry& Native() const noexcept;
        entt::entity RequireEntity(EntityId) const;
        EntityId ProjectId(entt::entity) const noexcept;
        template<class T> static void CheckBare();
        template<class T> static void CheckReadable();
        template<class Self, class... T, class... X, class Fn>
        static QueryResult RunEach(Self&, Exclude<X...>, Fn&);
        template<class Self, class... T, class... X, class Key, class Fn>
        static QueryResult RunOrdered(Self&, Exclude<X...>, Key&, Fn&);
        template<class Self, class... T, class... X>
        static void CheckQuery(Self&, Exclude<X...>);
        template<class Self, class T> static decltype(auto) QueryValue(Self&, entt::entity);
    };
}

#include "symocraft/ecs/detail/registry.inl"
