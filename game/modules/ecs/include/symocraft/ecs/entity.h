#pragma once
#include <cstdint>
#include <functional>

namespace SymoCraft::ECS {
    class Registry;
    class EntityId {
    public:
        constexpr EntityId() noexcept = default;
        constexpr bool operator==(const EntityId&) const noexcept = default;
    private:
        friend class Registry;
        friend struct std::hash<EntityId>;
        constexpr EntityId(std::uint64_t batch, std::uint32_t value) noexcept : batch_(batch), value_(value) {}
        std::uint64_t batch_{};
        std::uint32_t value_{};
    };
    inline constexpr EntityId null_entity{};
}

template<> struct std::hash<SymoCraft::ECS::EntityId> {
    std::size_t operator()(const SymoCraft::ECS::EntityId& id) const noexcept {
        const auto first = std::hash<std::uint64_t>{}(id.batch_);
        const auto second = std::hash<std::uint32_t>{}(id.value_);
        return first ^ (second + 0x9e3779b9u + (first << 6) + (first >> 2));
    }
};
