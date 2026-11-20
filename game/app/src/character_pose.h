#pragma once

#include <symocraft/simulation/npc.h>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>

namespace SymoCraft::Application::Detail {
struct CharacterMotion {
    std::uint64_t session_id{}, advance_id{};
    ECS::EntityId entity;
    double horizontal_distance{};
    Simulation::ActionSnapshot action;
};
struct CharacterPose {
    float left_arm{}, right_arm{}, left_leg{}, right_leg{};
    float Angle(std::string_view part) const noexcept;
};

// Presentation cache only: all time and distance originate in the simulation.
class CharacterPoses {
public:
    CharacterPose Update(const CharacterMotion& motion);
    void Retain(std::span<const ECS::EntityId> active);
    void Clear() noexcept { entries_.clear(); }
private:
    struct Entry {
        std::uint64_t session{}, advance{};
        Simulation::ActionId action{};
        Simulation::ActionPhase phase = Simulation::ActionPhase::Idle;
        double elapsed{}, duration{};
        double distance{};
        CharacterPose pose;
    };
    std::unordered_map<ECS::EntityId, Entry> entries_;
};
}
