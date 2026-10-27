#include "symocraft/ecs/registry.h"
using namespace SymoCraft::ECS;
struct Value { int value{}; };
struct Other { int value{}; };
struct NonDefault {
    explicit NonDefault(int) {}
    NonDefault(NonDefault&&) noexcept = default;
    NonDefault& operator=(NonDefault&&) noexcept = default;
};
struct ThrowingMove {
    ThrowingMove() = default;
    ThrowingMove(ThrowingMove&&) noexcept(false) {}
    ThrowingMove& operator=(ThrowingMove&&) noexcept(false) { return *this; }
};
void misuse(Registry& owner, const Registry& readonly, EntityId id) {
#if NEGATIVE_CASE == 1
    owner.RegisterComponent<const Value>("const");
#elif NEGATIVE_CASE == 2
    owner.RegisterComponent<Value&>("reference");
#elif NEGATIVE_CASE == 3
    owner.GetComponent<volatile Value>(id);
#elif NEGATIVE_CASE == 4
    readonly.GetComponent<Value>(id).value = 1;
#elif NEGATIVE_CASE == 5
    owner.GetComponent<const Value>(id).value = 1;
#elif NEGATIVE_CASE == 6
    owner.HasComponent<const Value>(id);
#elif NEGATIVE_CASE == 7
    owner.IsComponentRegistered<const Value>();
#elif NEGATIVE_CASE == 8
    owner.Emplace<const Value>(id);
#elif NEGATIVE_CASE == 9
    owner.RemoveComponent<const Value>(id);
#elif NEGATIVE_CASE == 10
    owner.Each<Value, const Value>([](EntityId, Value&, const Value&) {});
#elif NEGATIVE_CASE == 11
    owner.Each<Value>(Exclude<Value>{}, [](EntityId, Value&) {});
#elif NEGATIVE_CASE == 12
    owner.Each<>(Exclude<const Value>{}, [](EntityId) {});
#elif NEGATIVE_CASE == 13
    owner.Each<Value>([](EntityId, Value&) { return true; });
#elif NEGATIVE_CASE == 14
    owner.Each<>([](EntityId) { return 1; });
#elif NEGATIVE_CASE == 15
    owner.Each<Value>([](Value&) {});
#elif NEGATIVE_CASE == 16
    readonly.Each<Value>([](EntityId, Value&) {});
#elif NEGATIVE_CASE == 17
    owner.EachOrdered<Value>([](EntityId, Value&) { return 1; }, [](EntityId, Value&) {});
#elif NEGATIVE_CASE == 18
    owner.GetOrAdd<NonDefault>(id);
#elif NEGATIVE_CASE == 19
    owner.RegisterComponent<ThrowingMove>("throwing move");
#elif NEGATIVE_CASE == 20
    Registry copy(owner);
#elif NEGATIVE_CASE == 21
    Registry moved(std::move(owner));
#elif NEGATIVE_CASE == 22
    owner.Native();
#elif NEGATIVE_CASE == 23
    auto integer = static_cast<std::uint64_t>(id);
#elif NEGATIVE_CASE == 24
    auto ordered = id < EntityId{};
#elif NEGATIVE_CASE == 25
    owner.Each<>(Exclude<Value, Value>{}, [](EntityId) {});
#elif NEGATIVE_CASE == 26
    owner.EachOrdered<Value>([](EntityId, const Value& v) { return v.value; },
                            [](EntityId, Value&) { return false; });
#elif NEGATIVE_CASE == 27
    readonly.EachOrdered<Value>([](EntityId, const Value& v) { return v.value; },
                               [](EntityId, Value&) {});
#elif NEGATIVE_CASE == 28
    owner.GetComponent<Value&>(id);
#elif NEGATIVE_CASE == 29
    owner.Emplace<NonDefault>(id, "wrong constructor");
#endif
}
