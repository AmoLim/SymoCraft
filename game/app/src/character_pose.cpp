#include "character_pose.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace SymoCraft::Application::Detail {
float CharacterPose::Angle(std::string_view part) const noexcept {
    if (part == "left_arm") return left_arm;
    if (part == "right_arm") return right_arm;
    if (part == "left_leg") return left_leg;
    if (part == "right_leg") return right_leg;
    return 0.0f;
}
CharacterPose CharacterPoses::Update(const CharacterMotion& motion) {
    if (!std::isfinite(motion.horizontal_distance) || motion.horizontal_distance < 0 ||
        !std::isfinite(motion.action.elapsed_seconds) || motion.action.elapsed_seconds < 0 ||
        !std::isfinite(motion.action.duration_seconds) || motion.action.duration_seconds < 0)
        throw std::invalid_argument("invalid character motion facts");
    auto [found, inserted] = entries_.try_emplace(motion.entity);
    auto& entry = found->second;
    const bool same_run = !inserted && entry.session == motion.session_id;
    if (same_run && entry.advance == motion.advance_id && entry.action == motion.action.id &&
        entry.phase == motion.action.phase && entry.elapsed == motion.action.elapsed_seconds &&
        entry.duration == motion.action.duration_seconds && entry.distance == motion.horizontal_distance)
        return entry.pose; // Paused and repeated reads preserve the exact accepted pose.
    const double previous = same_run ? entry.distance : motion.horizontal_distance;
    CharacterPose pose;
    constexpr double tau = 2.0 * std::numbers::pi;
    if (motion.action.phase == Simulation::ActionPhase::Moving && motion.horizontal_distance > previous + 1e-7) {
        const float swing = static_cast<float>(std::sin(std::fmod(motion.horizontal_distance, 1.4) * tau / 1.4) * .65);
        pose.left_arm = -swing; pose.right_arm = swing;
        pose.left_leg = swing; pose.right_leg = -swing;
    } else if (motion.action.phase == Simulation::ActionPhase::Harvesting && motion.action.duration_seconds > 0) {
        const double phase = std::fmod(motion.action.elapsed_seconds, .65) * tau / .65;
        pose.right_arm = static_cast<float>(-.95 + .55 * std::sin(phase));
        pose.left_arm = -.35f;
    }
    entry = {motion.session_id, motion.advance_id, motion.action.id, motion.action.phase,
             motion.action.elapsed_seconds,motion.action.duration_seconds,motion.horizontal_distance,pose};
    return pose;
}
void CharacterPoses::Retain(std::span<const ECS::EntityId> active) {
    std::erase_if(entries_, [&](const auto& entry) {
        return std::find(active.begin(), active.end(), entry.first) == active.end();
    });
}
}
