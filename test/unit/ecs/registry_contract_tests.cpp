#include "contract_components.h"
#include "registry_test_access.h"
#include <array>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

using namespace SymoCraft::ECS;
using Access = Detail::RegistryTestAccess;
void ContractNamedPropertyCase();
namespace {
    bool model_active{};
    unsigned model_step{};
    const char* model_field = "setup";
    void Require(bool condition) { if (!condition) throw std::runtime_error("contract assertion"); }
    template<class Fn> void Reject(RegistryErrorCode code, Fn fn) {
        bool caught = false;
        try { fn(); } catch (const RegistryError& error) { caught = error.Code() == code; }
        Require(caught);
    }
    template<std::size_t N> struct Many { int value{}; };
    struct ThrowingKey {
        int value;
        static inline bool reject{};
        bool operator<(const ThrowingKey& other) const {
            if (reject) throw std::runtime_error("key comparison");
            return value < other.value;
        }
    };
    template<std::size_t... N> void RegisterMany(Registry& owner, std::index_sequence<N...>) {
        (owner.RegisterComponent<Many<N>>("same name allowed"), ...);
    }
    void Objects() {
        static_assert(!std::is_copy_constructible_v<Registry> && !std::is_move_constructible_v<Registry>);
        static_assert(std::is_nothrow_destructible_v<Registry>);
        static_assert(!std::is_convertible_v<EntityId, std::uint64_t>);
        static_assert(std::is_same_v<decltype(std::declval<Registry&>().GetComponent<const ContractValue>({})), const ContractValue&>);
        static_assert(std::is_same_v<decltype(std::declval<const Registry&>().GetComponent<ContractValue>({})), const ContractValue&>);
        static_assert(std::is_same_v<decltype(std::declval<const Registry&>().TryGetComponent<const ContractValue>({})), const ContractValue*>);
        {
            Registry owner, other;
            std::string name(100, 'n');
            owner.RegisterComponent<ContractValue>(name);
            name.clear();
            owner.RegisterComponent<ContractResource>("resource");
            owner.RegisterComponent<ContractEmpty>("empty");
            other.RegisterComponent<ContractEmpty>("first");
            other.RegisterComponent<ContractValue>("second");
            Require(!owner.IsComponentRegistered<ContractOther>());
            const auto id = owner.CreateEntity(), foreign = other.CreateEntity();
            Require(id != foreign && !owner.IsEntityValid(foreign) && owner.NumComponents(id) == 0);
            Reject(RegistryErrorCode::UnregisteredType, [&] { owner.HasComponent<ContractOther>(id); });
            Require(!owner.HasComponent<ContractValue>(id) && !owner.TryGetComponent<ContractValue>(id));
            Reject(RegistryErrorCode::MissingComponent, [&] { owner.GetComponent<ContractValue>(id); });
            owner.Emplace<ContractValue>(id, 7);
            ContractOtherTranslationUnit(owner, id);
            Require(owner.GetOrAdd<ContractValue>(id).value == 8 && Access::FirstName<ContractValue>(owner) == std::string(100, 'n'));
            Reject(RegistryErrorCode::DuplicateComponent, [&] { owner.Emplace<ContractValue>(id, 99); });
            Reject(RegistryErrorCode::InvalidName, [&] { owner.RegisterComponent<ContractValue>(nullptr); });
            Reject(RegistryErrorCode::InvalidName, [&] { owner.RegisterComponent<ContractOther>(std::string_view{}); });
            owner.Emplace<ContractEmpty>(id);
            Require(owner.TryGetComponent<ContractEmpty>(id) != nullptr);
            const auto next = owner.CreateEntity();
            owner.Emplace<ContractResource>(id, 11);
            owner.Emplace<ContractResource>(next, 22);
            Require(reinterpret_cast<std::uintptr_t>(owner.TryGetComponent<ContractResource>(id)) % 128 == 0);
            Require(owner.NumComponents(id) == 3 && ContractResource::live == 2);
            Require(owner.RemoveComponent<ContractResource>(id) && !owner.RemoveComponent<ContractResource>(id));
            Require(*owner.GetComponent<ContractResource>(next).value == 22 && ContractResource::live == 1);
            owner.DestroyEntity(id);
            Require(!owner.IsEntityValid(id));
            Reject(RegistryErrorCode::InvalidEntity, [&] { owner.HasComponent<ContractValue>(id); });
            Reject(RegistryErrorCode::InvalidEntity, [&] { owner.TryGetComponent<ContractValue>(null_entity); });
            Reject(RegistryErrorCode::InvalidEntity, [&] { owner.DestroyEntity(id); });
            const auto reused = owner.CreateEntity();
            Require(reused != id && owner.IsEntityValid(reused));
            std::unordered_set<EntityId> keys{id, foreign, reused, null_entity};
            owner.Clear();
            Require(keys.contains(id) && !owner.IsEntityValid(next) && ContractResource::live == 0);
            Require(owner.IsComponentRegistered<ContractValue>() && Access::FirstName<ContractValue>(owner) == std::string(100, 'n'));
            const auto high = Access::FreshFrontier(owner);
            for (std::uint32_t index = 0; index < high; ++index) (void)owner.CreateEntity();
            Require(Access::FreshFrontier(owner) == high);
            (void)owner.CreateEntity();
            Require(Access::FreshFrontier(owner) == high + 1);
            owner.Clear(); owner.Clear();
            owner.Emplace<ContractResource>(owner.CreateEntity(), 33);
        }
        Require(ContractResource::live == 0);
        Registry many;
        RegisterMany(many, std::make_index_sequence<260>{});
        many.Emplace<Many<259>>(many.CreateEntity(), 259);
        many.RegisterComponent<ContractCollisionA>("collision");
        Reject(RegistryErrorCode::TypeCollision, [&] { many.RegisterComponent<ContractCollisionB>("collision"); });
        Require(!many.IsComponentRegistered<ContractCollisionB>());
    }
    void Boundaries() {
        Registry owner;
        owner.RegisterComponent<ContractResource>("resource");
        const auto near_last = Access::PrepareGeneration4093(owner);
        owner.Emplace<ContractResource>(near_last, 7);
        owner.DestroyEntity(near_last);
        const auto last = owner.CreateEntity();
        owner.Emplace<ContractResource>(last, 8);
        Reject(RegistryErrorCode::GenerationExhausted, [&] { owner.DestroyEntity(last); });
        Require(*owner.GetComponent<ContractResource>(last).value == 8);
        owner.Clear();
        const auto reset = owner.CreateEntity();
        Require(reset != near_last && reset != last && owner.IsEntityValid(reset));
        Registry capacity;
        Access::PrepareNearCapacity(capacity);
        const auto a = capacity.CreateEntity(), b = capacity.CreateEntity();
        Reject(RegistryErrorCode::EntityCapacityExhausted, [&] { capacity.CreateEntity(); });
        capacity.Clear();
        Require(!capacity.IsEntityValid(a) && !capacity.IsEntityValid(b));
        (void)capacity.CreateEntity(); (void)capacity.CreateEntity();
        Reject(RegistryErrorCode::EntityCapacityExhausted, [&] { capacity.CreateEntity(); });
    }
    void ConstructorAndFailed() {
        Registry owner;
        owner.RegisterComponent<ContractThrowing>("throwing");
        const auto a = owner.CreateEntity(), b = owner.CreateEntity();
        owner.Emplace<ContractThrowing>(a, 4);
        ContractThrowing::reject = true;
        bool caught = false;
        try { owner.Emplace<ContractThrowing>(b, 5); }
        catch (const std::runtime_error&) { caught = true; }
        ContractThrowing::reject = false;
        Require(caught && !owner.HasComponent<ContractThrowing>(b) && owner.GetComponent<ContractThrowing>(a).value == 4);
        owner.Emplace<ContractThrowing>(b, 6);
        Access::SimulatePostClearConsistencyFailure(owner);
        const auto status = owner.GetStatus();
        Require(status.health == RegistryHealth::Failed && status.operation == RegistryOperation::Clear && !status.entity_facts_known);
        Reject(RegistryErrorCode::Failed, [&] { owner.IsEntityValid(null_entity); });
        Reject(RegistryErrorCode::Failed, [&] { owner.RegisterComponent<ContractValue>(nullptr); });
        Reject(RegistryErrorCode::Failed, [&] { owner.IsComponentRegistered<ContractValue>(); });
        Reject(RegistryErrorCode::Failed, [&] { owner.Clear(); });
    }
    void IdentityExhaustion() {
        Registry owner;
        owner.RegisterComponent<ContractValue>("identity retained name");
        const auto old = owner.CreateEntity();
        owner.Emplace<ContractValue>(old, 17);
        Access::AdvanceIdentitySourceToLastBatch();
        owner.Clear();
        const auto current = owner.CreateEntity();
        owner.Emplace<ContractValue>(current, 23);
        Reject(RegistryErrorCode::IdentityExhausted, [&] { owner.Clear(); });
        Reject(RegistryErrorCode::IdentityExhausted, [] { Registry impossible; });
        Require(!owner.IsEntityValid(old) && owner.IsEntityValid(current));
        Require(owner.NumComponents(current) == 1 && owner.GetComponent<ContractValue>(current).value == 23 &&
                Access::FirstName<ContractValue>(owner) == "identity retained name");
        (void)owner.CreateEntity();
    }
    void ModelSequence() {
        struct Row { EntityId id; bool alive{}, attached{}; int value{}; };
        std::array<Row, 32> model{};
        std::vector<EntityId> stale;
        Registry owner;
        owner.RegisterComponent<ContractValue>("model value");
        std::uint32_t seed = 0x35A91u;
        model_active = true;
        for (unsigned step = 0; step < 600; ++step) {
            model_step = step;
            model_field = "operation or expected rejection";
            seed = seed * 1664525u + 1013904223u;
            const auto slot = (seed >> 8) % model.size();
            auto& row = model[slot];
            switch ((seed >> 24) % 8) {
            case 0:
                if (!row.alive) { row = {owner.CreateEntity(), true, false, 0}; }
                break;
            case 1:
                if (row.alive) { owner.DestroyEntity(row.id); stale.push_back(row.id); row.alive = false; }
                break;
            case 2:
                if (row.alive) {
                    if (row.attached) Reject(RegistryErrorCode::DuplicateComponent, [&] { owner.Emplace<ContractValue>(row.id, 9); });
                    else { owner.Emplace<ContractValue>(row.id, 9); row.attached = true; row.value = 9; }
                }
                break;
            case 3:
                if (row.alive) { Require(owner.RemoveComponent<ContractValue>(row.id) == row.attached); row.attached = false; }
                break;
            case 4:
                if (row.alive) {
                    auto& value = owner.GetOrAdd<ContractValue>(row.id);
                    if (!row.attached) row.value = 0;
                    row.attached = true;
                    ++value.value; ++row.value;
                }
                break;
            case 5:
                if (step % 5 == 0) {
                    owner.Clear();
                    for (auto& item : model) if (item.alive) { stale.push_back(item.id); item.alive = false; }
                }
                break;
            default: break;
            }
            std::unordered_set<EntityId> expected, actual;
            for (const auto& item : model) if (item.alive) {
                const auto equal = [&](auto left, auto right, const char* field) {
                    model_field = field;
                    if (left != right) {
                        std::fprintf(stderr, "model row=%zu field=%s actual=%lld expected=%lld\n",
                            static_cast<std::size_t>(&item - model.data()), field,
                            static_cast<long long>(left), static_cast<long long>(right));
                        Require(false);
                    }
                };
                equal(owner.IsEntityValid(item.id), true, "alive");
                equal(owner.HasComponent<ContractValue>(item.id), item.attached, "attached");
                equal(owner.NumComponents(item.id), item.attached ? 1u : 0u, "component count");
                if (item.attached) {
                    equal(owner.GetComponent<ContractValue>(item.id).value, item.value, "component value");
                    expected.insert(item.id);
                }
            }
            model_field = "query membership and uniqueness";
            owner.Each<const ContractValue>([&](EntityId id, const ContractValue&) { Require(actual.insert(id).second); });
            Require(expected == actual);
            model_field = "stale ID validity";
            for (const auto id : stale) Require(!owner.IsEntityValid(id));
        }
        model_active = false;
    }
    void Queries() {
        Registry owner;
        owner.RegisterComponent<ContractValue>("value");
        owner.RegisterComponent<ContractOther>("other");
        owner.RegisterComponent<ContractEmpty>("empty");
        const auto a = owner.CreateEntity(), b = owner.CreateEntity(), empty = owner.CreateEntity();
        owner.Emplace<ContractValue>(a, 2); owner.Emplace<ContractValue>(b, 1);
        owner.Emplace<ContractOther>(b, 9);
        unsigned seen = 0;
        owner.Each<>([&](EntityId) { ++seen; });
        Require(seen == 3);
        seen = 0;
        owner.Each<const ContractValue>(Exclude<ContractOther>{}, [&](EntityId id, const ContractValue& value) {
            Require(id == a && value.value == 2); ++seen;
            Require(owner.Each<>([](EntityId) { return VisitControl::Stop; }) == QueryResult::Stopped);
            Reject(RegistryErrorCode::InvalidContext, [&] { owner.RegisterComponent<ContractValue>(nullptr); });
            Reject(RegistryErrorCode::InvalidContext, [&] { owner.GetOrAdd<ContractValue>(a); });
            Reject(RegistryErrorCode::InvalidContext, [&] { owner.RemoveComponent<ContractEmpty>(a); });
            Reject(RegistryErrorCode::InvalidContext, [&] { owner.CreateEntity(); });
            Reject(RegistryErrorCode::InvalidContext, [&] { owner.Clear(); });
        });
        Require(seen == 1);
        const Registry& readonly = owner;
        readonly.Each<ContractOther, const ContractValue>([&](EntityId id, const ContractOther& x, const ContractValue& v) {
            Require(id == b && x.value == 9 && v.value == 1);
        });
        std::vector<int> order;
        Require(owner.EachOrdered<ContractValue>([](EntityId, const ContractValue& v) { return v.value; },
            [&](EntityId, ContractValue& v) { order.push_back(v.value); v.value += 10; }) == QueryResult::Completed);
        Require(order == std::vector<int>({1, 2}));
        unsigned callbacks = 0;
        Reject(RegistryErrorCode::OrderingTie, [&] {
            owner.EachOrdered<ContractValue>([](EntityId, const ContractValue&) { return 0; },
                [&](EntityId, ContractValue&) { ++callbacks; });
        });
        Require(callbacks == 0);
        bool caught = false;
        try { owner.Each<ContractValue>([](EntityId, ContractValue& v) { ++v.value; throw std::runtime_error("visitor"); }); }
        catch (const std::runtime_error&) { caught = true; }
        Require(caught);
        owner.DestroyEntity(empty);
        Require(readonly.Each<ContractEmpty>([](EntityId, const ContractEmpty&) {}) == QueryResult::Completed);
        Require(readonly.Each<>([](EntityId) { return VisitControl::Stop; }) == QueryResult::Stopped);
        owner.Each<>([&](EntityId) {
            bool inner_caught = false;
            try { owner.Each<>([](EntityId) { throw std::runtime_error("inner visitor"); }); }
            catch (const std::runtime_error&) { inner_caught = true; }
            Require(inner_caught);
            Reject(RegistryErrorCode::InvalidContext, [&] { owner.DestroyEntity(a); });
            return VisitControl::Stop;
        });
        callbacks = 0;
        caught = false;
        try {
            owner.EachOrdered<ContractValue>([&](EntityId, const ContractValue& v) {
                Reject(RegistryErrorCode::InvalidContext, [&] { owner.Clear(); });
                if (v.value > 0) throw std::runtime_error("key creation");
                return v.value;
            }, [&](EntityId, ContractValue&) { ++callbacks; });
        } catch (const std::runtime_error&) { caught = true; }
        Require(caught && callbacks == 0);
        ThrowingKey::reject = true;
        caught = false;
        try {
            readonly.EachOrdered<ContractValue>([](EntityId, const ContractValue& v) { return ThrowingKey{v.value}; },
                [&](EntityId, const ContractValue&) { ++callbacks; });
        } catch (const std::runtime_error&) { caught = true; }
        ThrowingKey::reject = false;
        Require(caught && callbacks == 0);
        owner.RegisterComponent<ContractEmpty>("guard released");
        Require(owner.Each<ContractValue>([token = std::make_unique<int>(1)](EntityId, ContractValue&) {
            return *token ? VisitControl::Stop : VisitControl::Continue;
        }) == QueryResult::Stopped);
        unsigned ordered_empty = 0;
        readonly.EachOrdered<>([&](EntityId id) { return id == a ? 1 : 2; }, [&](EntityId) { ++ordered_empty; });
        Require(ordered_empty == 2);
    }
}
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "identity-exhaustion") IdentityExhaustion();
        else { Objects(); Boundaries(); ConstructorAndFailed(); Queries(); ModelSequence(); ContractNamedPropertyCase(); }
        Require(ContractResource::live == 0);
        std::puts("PASS production Registry contracts");
        return 0;
    } catch (const std::exception& error) {
        if (model_active) std::fprintf(stderr, "model seed=0x35A91 step=%u field=%s MSVC=%d\n", model_step, model_field, _MSC_FULL_VER);
        std::fprintf(stderr, "%s\n", error.what()); return 1;
    }
}
