#pragma once
#include "symocraft/ecs/registry.h"
#include <memory>
#include <stdexcept>

struct ContractValue { int value{}; };
struct ContractOther { int value{}; };
struct ContractEmpty {};
struct alignas(128) ContractResource {
    static inline int live{};
    std::unique_ptr<int> value;
    explicit ContractResource(int v) : value(std::make_unique<int>(v)) { ++live; }
    ContractResource(ContractResource&& other) noexcept : value(std::move(other.value)) { ++live; }
    ContractResource& operator=(ContractResource&&) noexcept = default;
    ~ContractResource() noexcept { --live; }
};
struct ContractThrowing {
    static inline bool reject{};
    int value;
    explicit ContractThrowing(int v) : value(v) { if (reject) throw std::runtime_error("component constructor"); }
    ContractThrowing(ContractThrowing&&) noexcept = default;
    ContractThrowing& operator=(ContractThrowing&&) noexcept = default;
    ~ContractThrowing() noexcept = default;
};
struct ContractCollisionA { int value{}; };
struct ContractCollisionB { int value{}; };
namespace entt {
    template<> struct type_hash<ContractCollisionA> { static constexpr id_type value() noexcept { return 0x12500001; } };
    template<> struct type_hash<ContractCollisionB> { static constexpr id_type value() noexcept { return 0x12500001; } };
}
void ContractOtherTranslationUnit(SymoCraft::ECS::Registry&, SymoCraft::ECS::EntityId);
