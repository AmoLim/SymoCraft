#include "contract_components.h"
void ContractOtherTranslationUnit(SymoCraft::ECS::Registry& registry, SymoCraft::ECS::EntityId id) {
    registry.RegisterComponent<ContractValue>("other TU cannot overwrite");
    ++registry.GetComponent<ContractValue>(id).value;
}

namespace {
    struct NamedPropertyComponent {
        static constexpr bool in_place_delete = true;
        static inline bool reject{};
        int value;
        explicit NamedPropertyComponent(int input) : value(input) {
            if (reject) throw std::runtime_error("component constructor with retained slot");
        }
        NamedPropertyComponent(NamedPropertyComponent&&) noexcept = default;
        NamedPropertyComponent& operator=(NamedPropertyComponent&&) noexcept = default;
    };
}
void ContractNamedPropertyCase() {
    using namespace SymoCraft::ECS;
    Registry owner;
    owner.RegisterComponent<NamedPropertyComponent>("ordinary named property");
    const auto a = owner.CreateEntity(), b = owner.CreateEntity(), c = owner.CreateEntity();
    owner.Emplace<NamedPropertyComponent>(a, 11);
    owner.Emplace<NamedPropertyComponent>(b, 22);
    NamedPropertyComponent::reject = true;
    bool caught = false;
    try { owner.Emplace<NamedPropertyComponent>(c, 33); }
    catch (const std::runtime_error&) { caught = true; }
    NamedPropertyComponent::reject = false;
    if (!caught || owner.GetStatus().health != RegistryHealth::Healthy || owner.HasComponent<NamedPropertyComponent>(c) ||
        owner.GetComponent<NamedPropertyComponent>(a).value != 11 || owner.GetComponent<NamedPropertyComponent>(b).value != 22)
        throw std::runtime_error("physical hole incorrectly treated as inconsistent live storage");
    unsigned count = 0;
    owner.Each<const NamedPropertyComponent>([&](EntityId, const NamedPropertyComponent&) { ++count; });
    if (count != 2) throw std::runtime_error("query exposed rolled-back component hole");
    owner.Emplace<NamedPropertyComponent>(c, 33);
    owner.RemoveComponent<NamedPropertyComponent>(a);
    if (owner.NumComponents(c) != 1 || owner.GetComponent<NamedPropertyComponent>(c).value != 33)
        throw std::runtime_error("named-property retry or removal failed");
}
