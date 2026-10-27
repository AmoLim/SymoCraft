#include "symocraft/ecs/registry.h"
#include "diagnostics/registry_test_access.h"
#include <limits>
#include <string>
#include <vector>

namespace SymoCraft::ECS {
    namespace {
        std::uint64_t next_batch = 1;
        bool batches_exhausted = false;
        std::uint64_t TakeBatch() {
            if (batches_exhausted) throw RegistryError(RegistryErrorCode::IdentityExhausted);
            const auto batch = next_batch;
            if (batch == (std::numeric_limits<std::uint64_t>::max)()) batches_exhausted = true;
            else ++next_batch;
            return batch;
        }
    }
    struct Registry::Storage {
        struct Registration { const void* token; entt::id_type key; std::string name; };
        static_assert(std::is_nothrow_move_constructible_v<Registration>);
        std::uint64_t batch;
        entt::registry native;
        std::vector<Registration> registrations;
        std::uint32_t next_fresh{};
        std::size_t queries{};
        bool mutating{};
        RegistryStatus status{};
        explicit Storage(std::uint64_t value) : batch(value) {}
    };
    const char* RegistryError::what() const noexcept {
        switch (code_) {
        case RegistryErrorCode::Failed: return "ECS: Registry is Failed";
        case RegistryErrorCode::InvalidContext: return "ECS: query conflict or component lifecycle reentry";
        case RegistryErrorCode::InvalidEntity: return "ECS: invalid entity";
        case RegistryErrorCode::UnregisteredType: return "ECS: component is not registered";
        case RegistryErrorCode::InvalidName: return "ECS: component name must not be null or empty";
        case RegistryErrorCode::TypeCollision: return "ECS: component type key collision";
        case RegistryErrorCode::DuplicateComponent: return "ECS: component already exists";
        case RegistryErrorCode::MissingComponent: return "ECS: missing component";
        case RegistryErrorCode::IdentityExhausted: return "ECS: identity batches exhausted";
        case RegistryErrorCode::EntityCapacityExhausted: return "ECS: entity capacity exhausted";
        case RegistryErrorCode::GenerationExhausted: return "ECS: entity generation exhausted";
        case RegistryErrorCode::OrderingTie: return "ECS: equivalent ordered-query keys";
        case RegistryErrorCode::StorageInconsistent: return "ECS: storage consistency could not be confirmed";
        default: return "ECS: contract rejection";
        }
    }
    Registry::Registry() : storage_(std::make_unique<Storage>(TakeBatch())) {}
    Registry::~Registry() noexcept {
        storage_->mutating = true;
    }
    entt::registry& Registry::Native() noexcept { return storage_->native; }
    const entt::registry& Registry::Native() const noexcept { return storage_->native; }
    RegistryStatus Registry::GetStatus() const noexcept { return storage_->status; }
    void Registry::CheckRead() const {
        if (storage_->status.health == RegistryHealth::Failed) throw RegistryError(RegistryErrorCode::Failed);
        if (storage_->mutating) throw RegistryError(RegistryErrorCode::InvalidContext);
    }
    void Registry::CheckStructural() const {
        CheckRead();
        if (storage_->queries) throw RegistryError(RegistryErrorCode::InvalidContext);
    }
    Registry::MutationGuard::MutationGuard(Registry& owner) noexcept : owner_(owner) { owner_.storage_->mutating = true; }
    Registry::MutationGuard::~MutationGuard() { owner_.storage_->mutating = false; }
    Registry::QueryGuard::QueryGuard(const Registry& owner) : owner_(owner) {
        owner_.CheckRead();
        if (owner_.storage_->queries == (std::numeric_limits<std::size_t>::max)())
            throw RegistryError(RegistryErrorCode::InvalidContext);
        ++owner_.storage_->queries;
    }
    Registry::QueryGuard::~QueryGuard() { --owner_.storage_->queries; }
    void Registry::Fail(RegistryOperation operation) noexcept {
        storage_->status = {RegistryHealth::Failed, operation, RegistryErrorCode::StorageInconsistent, false};
    }
    bool Registry::IsEntityValid(EntityId id) const {
        CheckRead();
        return id.batch_ == storage_->batch && Native().valid(static_cast<entt::entity>(id.value_));
    }
    entt::entity Registry::RequireEntity(EntityId id) const {
        if (!IsEntityValid(id)) throw RegistryError(RegistryErrorCode::InvalidEntity);
        return static_cast<entt::entity>(id.value_);
    }
    EntityId Registry::ProjectId(entt::entity entity) const noexcept {
        return EntityId(storage_->batch, entt::to_integral(entity));
    }
    bool Registry::IsTypeRegistered(const void* token) const {
        for (const auto& entry : storage_->registrations) if (entry.token == token) return true;
        return false;
    }
    void Registry::RequireType(const void* token) const {
        if (!IsTypeRegistered(token)) throw RegistryError(RegistryErrorCode::UnregisteredType);
    }
    void Registry::RegisterType(const void* token, entt::id_type key, std::string_view name,
                                void (*prepare)(entt::registry&)) {
        if (name.empty()) throw RegistryError(RegistryErrorCode::InvalidName);
        if (IsTypeRegistered(token)) return;
        for (const auto& entry : storage_->registrations)
            if (entry.key == key) throw RegistryError(RegistryErrorCode::TypeCollision);
        MutationGuard guard(*this);
        Storage::Registration pending{token, key, std::string(name)};
        storage_->registrations.reserve(storage_->registrations.size() + 1);
        prepare(Native());
        // All capacity is ready; final publication cannot allocate under DBG01.
        storage_->registrations.push_back(std::move(pending));
    }
    EntityId Registry::CreateEntity() {
        CheckStructural();
        using Traits = entt::entt_traits<entt::entity>;
        auto& entities = Native().storage<entt::entity>();
        const bool fresh = entities.free_list() == entities.size();
        if (fresh && storage_->next_fresh >= Traits::entity_mask)
            throw RegistryError(RegistryErrorCode::EntityCapacityExhausted);
        const auto hint = Traits::construct(storage_->next_fresh, 0);
        MutationGuard guard(*this);
        const auto before = entities.free_list();
        entt::entity created;
        try { created = fresh ? Native().create(hint) : Native().create(); }
        catch (...) {
            if (entities.free_list() != before) Fail(RegistryOperation::Create);
            throw;
        }
        if (!Native().valid(created) || (fresh && created != hint)) {
            Fail(RegistryOperation::Create);
            throw RegistryError(RegistryErrorCode::StorageInconsistent);
        }
        if (fresh) ++storage_->next_fresh;
        return ProjectId(created);
    }
    void Registry::DestroyEntity(EntityId id) {
        CheckStructural();
        const auto entity = RequireEntity(id);
        if (entt::to_version(entity) >= entt::entt_traits<entt::entity>::version_mask - 1)
            throw RegistryError(RegistryErrorCode::GenerationExhausted);
        MutationGuard guard(*this);
        try { Native().destroy(entity); }
        catch (...) { Fail(RegistryOperation::Destroy); throw; }
    }
    void Registry::Clear() {
        CheckStructural();
        const auto prepared = TakeBatch();
        MutationGuard guard(*this);
        try { Native().clear(); }
        catch (...) { Fail(RegistryOperation::Clear); throw; }
        storage_->batch = prepared;
        // Clear retains recyclable slots: do not rewind the fresh frontier.
    }
    std::size_t Registry::NumComponents(EntityId id) const {
        CheckRead();
        const auto entity = RequireEntity(id);
        std::size_t count = 0;
        for (const auto& entry : storage_->registrations)
            count += Native().storage(entry.key)->contains(entity) ? 1 : 0;
        return count;
    }

    namespace Detail {
        void RegistryTestAccess::PrepareNearCapacity(Registry& owner) {
            owner.CheckStructural();
            if (owner.Native().storage<entt::entity>().size() != 0 || owner.storage_->next_fresh != 0)
                throw RegistryError(RegistryErrorCode::InvalidContext);
            owner.storage_->next_fresh = entt::entt_traits<entt::entity>::entity_mask - 2;
        }
        EntityId RegistryTestAccess::PrepareGeneration4093(Registry& owner) {
            owner.CheckStructural();
            if (owner.Native().storage<entt::entity>().size() != 0 || owner.storage_->next_fresh != 0)
                throw RegistryError(RegistryErrorCode::InvalidContext);
            const auto hint = entt::entt_traits<entt::entity>::construct(0, 4093);
            const auto entity = owner.Native().create(hint);
            if (entity != hint) {
                owner.Fail(RegistryOperation::Create);
                throw RegistryError(RegistryErrorCode::StorageInconsistent);
            }
            owner.storage_->next_fresh = 1;
            return owner.ProjectId(entity);
        }
        void RegistryTestAccess::AdvanceIdentitySourceToLastBatch() {
            if (batches_exhausted) throw RegistryError(RegistryErrorCode::IdentityExhausted);
            next_batch = (std::numeric_limits<std::uint64_t>::max)();
        }
        void RegistryTestAccess::SimulatePostClearConsistencyFailure(Registry& owner) {
            owner.CheckStructural();
            Registry::MutationGuard guard(owner);
            owner.Native().clear();
            owner.Fail(RegistryOperation::Clear);
        }
        std::uint32_t RegistryTestAccess::FreshFrontier(const Registry& owner) {
            owner.CheckRead();
            return owner.storage_->next_fresh;
        }
        std::string RegistryTestAccess::Name(const Registry& owner, const void* token) {
            owner.CheckRead();
            for (const auto& entry : owner.storage_->registrations) if (entry.token == token) return entry.name;
            throw RegistryError(RegistryErrorCode::UnregisteredType);
        }
    }
}
