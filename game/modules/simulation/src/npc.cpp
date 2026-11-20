#include <symocraft/simulation/npc.h>
#include <symocraft/simulation/session.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/player_math.h>
#include "npc_runtime.h"
#include "physics_adapter.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <exception>
#include <utility>

namespace SymoCraft::Simulation {
    const char* ActionPhaseName(ActionPhase phase) noexcept {
        switch (phase) {
        case ActionPhase::Idle: return "Idle";
        case ActionPhase::Moving: return "Moving";
        case ActionPhase::Harvesting: return "Harvesting";
        case ActionPhase::Ready: return "Ready";
        case ActionPhase::Succeeded: return "Succeeded";
        case ActionPhase::Failed: return "Failed";
        case ActionPhase::Cancelled: return "Cancelled";
        }
        return "Unknown";
    }
    const char* ActionFailureName(ActionFailure failure) noexcept {
        switch (failure) {
        case ActionFailure::None: return "None";
        case ActionFailure::InvalidTarget: return "InvalidTarget";
        case ActionFailure::Unharvestable: return "Unharvestable";
        case ActionFailure::Blocked: return "Blocked";
        case ActionFailure::Interrupted: return "Interrupted";
        case ActionFailure::Cancelled: return "Cancelled";
        case ActionFailure::CreditOverflow: return "CreditOverflow";
        case ActionFailure::CreditPreparation: return "CreditPreparation";
        case ActionFailure::WorldRejected: return "WorldRejected";
        }
        return "Unknown";
    }
    ActionId LivingEntity::Harvest(World::BlockCoord target) { return session_.StartHarvest(entity_, target); }
    void LivingEntity::Cancel(ActionId action) { session_.CancelAction(entity_, action); }
    ActionId Player::Harvest(World::BlockCoord) { throw Rejected("Timed harvest is an NPC capability"); }
}

namespace SymoCraft::Simulation::Detail {
    namespace {
        constexpr float body_half_width = 0.275f;
        constexpr float body_half_height = 0.9f;
        constexpr float fixed_seconds = 1.0f / 120.0f;
        bool Active(ActionPhase phase) noexcept {
            return phase == ActionPhase::Moving || phase == ActionPhase::Harvesting || phase == ActionPhase::Ready;
        }
        void CheckConfig(const NpcConfig& value) {
            if ((value.kind != NpcKind::Human && value.kind != NpcKind::Zombie) ||
                !PlayerMath::IsFinite(value.position) || glm::any(glm::greaterThan(glm::abs(value.position), glm::vec3(999998.0f))) ||
                !std::isfinite(value.yaw) || value.order_key == 0 ||
                !std::isfinite(value.efficiency) || value.efficiency <= 0 ||
                !std::isfinite(value.speed) || value.speed <= 0 || value.speed > 20 ||
                !std::isfinite(value.work_distance) || value.work_distance <= 0 || value.work_distance > 3 ||
                !std::isfinite(value.arrival_tolerance) || value.arrival_tolerance <= 0 || value.arrival_tolerance > .2f ||
                !std::isfinite(value.blocked_seconds) || value.blocked_seconds <= 0 || value.blocked_seconds > 30)
                throw Rejected("AddNpc: invalid configuration");
        }
    }

    NpcRuntime::NpcRuntime(ECS::Registry& registry, const World::VoxelWorld& world)
        : registry_(registry), world_(world) {
        registry_.RegisterComponent<NpcComponent>("Npc");
    }
    NpcComponent& NpcRuntime::Require(ECS::EntityId entity) {
        if (std::find(entities_.begin(), entities_.end(), entity) == entities_.end() || !registry_.IsEntityValid(entity))
            throw Rejected("NPC identity is not active in this session");
        return registry_.GetComponent<NpcComponent>(entity);
    }
    const NpcComponent& NpcRuntime::Require(ECS::EntityId entity) const {
        if (std::find(entities_.begin(), entities_.end(), entity) == entities_.end() || !registry_.IsEntityValid(entity))
            throw Rejected("NPC identity is not active in this session");
        return std::as_const(registry_).GetComponent<NpcComponent>(entity);
    }
    ECS::EntityId NpcRuntime::Add(const NpcConfig& config) {
        CheckConfig(config);
        for (const auto entity : entities_)
            if (Require(entity).config.order_key == config.order_key) throw Rejected("AddNpc: duplicate order key");
        registry_.Each<const PhysicsStepOrder>([&](ECS::EntityId, const PhysicsStepOrder& order) {
            if (order.value == config.order_key) throw Rejected("AddNpc: duplicate physical order key");
        });
        entities_.reserve(entities_.size() + 1);
        const auto insertion = std::lower_bound(entities_.begin(), entities_.end(), config.order_key,
            [&](ECS::EntityId entity, std::uint64_t key) { return Require(entity).config.order_key < key; });
        const auto insertion_index = static_cast<std::size_t>(insertion - entities_.begin());
        NpcComponent candidate{};
        candidate.config = config;
        const auto entity = registry_.CreateEntity();
        try {
            registry_.Emplace<Transform>(entity);
            registry_.Emplace<Physics::HitBox>(entity, glm::vec3(.55f, 1.8f, .55f), glm::vec3(0.0f));
            registry_.Emplace<Physics::RigidBody>(entity);
            registry_.Emplace<Character::CharacterComponent>(entity);
            registry_.Emplace<PhysicsStepOrder>(entity, config.order_key);
            registry_.Emplace<NpcComponent>(entity, std::move(candidate));
            auto& transform = registry_.GetComponent<Transform>(entity);
            transform = {};
            transform.position = config.position;
            transform.scale = glm::vec3(1.0f);
            transform.yaw = config.yaw;
            transform.front = {std::cos(glm::radians(config.yaw)), 0, std::sin(glm::radians(config.yaw))};
            transform.up = {0, 1, 0};
            transform.right = glm::cross(transform.front, transform.up);
            auto& body = registry_.GetComponent<Physics::RigidBody>(entity);
            body = {};
            body.use_gravity = true;
            registry_.GetComponent<Character::CharacterComponent>(entity) = {};
            entities_.insert(entities_.begin() + insertion_index, entity);
            return entity;
        } catch (...) {
            // A half-assembled entity never enters the active list. Healthy storage can remove it.
            if (registry_.GetStatus().health == ECS::RegistryHealth::Healthy) registry_.DestroyEntity(entity);
            throw;
        }
    }
    void NpcRuntime::Remove(ECS::EntityId entity) {
        static_cast<void>(Require(entity));
        registry_.DestroyEntity(entity);
        entities_.erase(std::find(entities_.begin(), entities_.end(), entity));
    }
    bool NpcRuntime::WorkPointClear(const glm::vec3& position) const {
        const auto query = CollisionQuery(world_);
        const glm::vec3 half(body_half_width, body_half_height, body_half_width);
        const glm::ivec3 low(glm::floor(position - half + glm::vec3(.001f)));
        const glm::ivec3 high(glm::floor(position + half - glm::vec3(.001f)));
        for (int x = low.x; x <= high.x; ++x)
            for (int y = low.y; y <= high.y; ++y)
                for (int z = low.z; z <= high.z; ++z)
                    if (query.is_solid(query.context, {x, y, z})) return false;
        const glm::ivec3 support(glm::floor(position - glm::vec3(0, body_half_height + .01f, 0)));
        return query.is_solid(query.context, support);
    }
    ActionId NpcRuntime::Start(ECS::EntityId entity, World::BlockCoord position) {
        auto& npc = Require(entity);
        if (npc.config.kind != NpcKind::Human) throw Rejected("StartHarvest: this NPC has no harvest capability");
        if (next_action_ == (std::numeric_limits<ActionId>::max)()) throw Rejected("Action identity exhausted");
        ActionSnapshot action{};
        action.id = next_action_;
        action.target = position;
        auto token = world_.CaptureHarvestTarget(position);
        auto definition = token ? world_.DescribeBlock(token->block_id) : std::nullopt;
        if (!definition || !definition->harvest) {
            Stop(entity, npc, token ? ActionFailure::Unharvestable : ActionFailure::InvalidTarget);
            action.phase = ActionPhase::Failed;
            action.failure = npc.action.failure;
            npc.action = action;
            return next_action_++;
        }
        World::HarvestRule rule = *definition->harvest;
        action.duration_seconds = rule.base_seconds / npc.config.efficiency;
        if (!std::isfinite(action.duration_seconds) || action.duration_seconds <= 0)
            throw Rejected("StartHarvest: effective duration must be finite and positive");
        const auto origin = registry_.GetComponent<Transform>(entity).position;
        std::array<glm::vec3, 4> candidates{{
            {position.x - .5f, position.y + body_half_height, position.z + .5f},
            {position.x + .5f, position.y + body_half_height, position.z - .5f},
            {position.x + .5f, position.y + body_half_height, position.z + 1.5f},
            {position.x + 1.5f, position.y + body_half_height, position.z + .5f}}};
        std::stable_sort(candidates.begin(), candidates.end(), [&](const auto& left, const auto& right) {
            return glm::dot(left - origin, left - origin) < glm::dot(right - origin, right - origin);
        });
        const auto work = std::find_if(candidates.begin(), candidates.end(), [&](const auto& candidate) {
            return glm::distance(candidate, glm::vec3(position) + .5f) <= npc.config.work_distance && WorkPointClear(candidate);
        });
        Stop(entity, npc, ActionFailure::Cancelled);
        npc.action = action;
        npc.target = *token;
        npc.rule = std::move(rule);
        if (work == candidates.end()) {
            npc.action.phase = ActionPhase::Failed;
            npc.action.failure = ActionFailure::Blocked;
        } else {
            npc.work_position = *work;
            npc.previous_distance = glm::distance(origin, *work);
            npc.blocked_elapsed = 0;
            npc.action.phase = ActionPhase::Moving;
        }
        return next_action_++;
    }
    void NpcRuntime::Stop(ECS::EntityId entity, NpcComponent& npc, ActionFailure reason) {
        auto& body = registry_.GetComponent<Physics::RigidBody>(entity);
        body.velocity.x = body.velocity.z = 0;
        npc.action.phase = reason == ActionFailure::Cancelled ? ActionPhase::Cancelled : ActionPhase::Failed;
        npc.action.failure = reason;
        npc.action.elapsed_seconds = 0;
    }
    void NpcRuntime::Cancel(ECS::EntityId entity, ActionId action) {
        auto& npc = Require(entity);
        if (action != npc.action.id) throw Rejected("CancelAction: action identity is stale");
        if (Active(npc.action.phase)) Stop(entity, npc, ActionFailure::Cancelled);
    }
    NpcSnapshot NpcRuntime::Inspect(ECS::EntityId entity) const {
        const auto& npc = Require(entity);
        const auto& transform = std::as_const(registry_).GetComponent<Transform>(entity);
        return {entity, npc.config.kind, transform.position, transform.yaw, npc.action, npc.resources};
    }
    std::vector<NpcDisplay> NpcRuntime::Displays() const {
        std::vector<NpcDisplay> result;
        result.reserve(entities_.size());
        for (const auto entity : entities_) {
            const auto& npc = Require(entity);
            const auto& transform = std::as_const(registry_).GetComponent<Transform>(entity);
            result.push_back({entity, npc.config.kind, transform.position, transform.yaw,
                0, 0, npc.horizontal_distance, npc.action, npc.resources});
        }
        return result;
    }
    bool NpcRuntime::ValidWorkPosition(ECS::EntityId entity, const NpcComponent& npc) const {
        const auto& transform = registry_.GetComponent<Transform>(entity);
        const auto& body = registry_.GetComponent<Physics::RigidBody>(entity);
        return body.on_ground && glm::distance(transform.position, npc.work_position) <= npc.config.arrival_tolerance &&
            glm::distance(transform.position, glm::vec3(npc.action.target) + .5f) <= npc.config.work_distance &&
            WorkPointClear(transform.position);
    }
    void NpcRuntime::BeforeAdvance(float seconds) {
        for (const auto entity : entities_) {
            auto& npc = Require(entity);
            if (!Active(npc.action.phase)) continue;
            if (!world_.IsHarvestTargetValid(npc.target)) { Stop(entity, npc, ActionFailure::InvalidTarget); continue; }
            if (npc.action.phase == ActionPhase::Moving) {
                if (ValidWorkPosition(entity, npc)) npc.action.phase = ActionPhase::Harvesting;
                continue;
            }
            if (!ValidWorkPosition(entity, npc)) { Stop(entity, npc, ActionFailure::Interrupted); continue; }
            if (npc.action.phase == ActionPhase::Harvesting) {
                npc.action.elapsed_seconds = std::min(npc.action.duration_seconds, npc.action.elapsed_seconds + static_cast<double>(seconds));
                if (npc.action.elapsed_seconds >= npc.action.duration_seconds) npc.action.phase = ActionPhase::Ready;
            }
        }
    }
    void NpcRuntime::BeforePhysics() {
        for (const auto entity : entities_) {
            auto& npc = Require(entity);
            auto& transform = registry_.GetComponent<Transform>(entity);
            auto& body = registry_.GetComponent<Physics::RigidBody>(entity);
            npc.step_origin = transform.position;
            body.velocity.x = body.velocity.z = 0;
            if (npc.action.phase != ActionPhase::Moving) continue;
            auto direction = npc.work_position - transform.position;
            direction.y = 0;
            const auto distance = glm::length(direction);
            if (distance > npc.config.arrival_tolerance) {
                direction /= distance;
                const auto speed = std::min(npc.config.speed, distance / fixed_seconds);
                body.velocity.x = direction.x * speed;
                body.velocity.z = direction.z * speed;
                transform.yaw = glm::degrees(std::atan2(direction.z, direction.x));
            }
        }
    }
    void NpcRuntime::AfterPhysics() {
        for (const auto entity : entities_) {
            auto& npc = Require(entity);
            const auto offset = registry_.GetComponent<Transform>(entity).position - npc.step_origin;
            npc.horizontal_distance += std::hypot(static_cast<double>(offset.x), static_cast<double>(offset.z));
            if (npc.action.phase != ActionPhase::Moving) continue;
            if (ValidWorkPosition(entity, npc)) {
                npc.action.phase = ActionPhase::Harvesting;
                auto& body = registry_.GetComponent<Physics::RigidBody>(entity);
                body.velocity.x = body.velocity.z = 0;
                continue;
            }
            const auto distance = glm::distance(registry_.GetComponent<Transform>(entity).position, npc.work_position);
            npc.blocked_elapsed = distance < npc.previous_distance - .00001f ? 0 : npc.blocked_elapsed + fixed_seconds;
            npc.previous_distance = distance;
            if (npc.blocked_elapsed >= npc.config.blocked_seconds) Stop(entity, npc, ActionFailure::Blocked);
        }
    }
    void NpcRuntime::Pause() {
        for (const auto entity : entities_) {
            auto& body = registry_.GetComponent<Physics::RigidBody>(entity);
            body.velocity.x = body.velocity.z = 0;
        }
    }
    void NpcRuntime::Commit(World::VoxelWorld& world) {
        for (const auto entity : entities_) {
            auto& npc = Require(entity);
            if (npc.action.phase != ActionPhase::Ready) continue;
            if (!world.IsHarvestTargetValid(npc.target)) { Stop(entity, npc, ActionFailure::InvalidTarget); continue; }
            if (!ValidWorkPosition(entity, npc)) { Stop(entity, npc, ActionFailure::Interrupted); continue; }
            std::vector<ResourceCount> candidate;
            try {
                candidate = npc.resources;
                const auto entry = std::lower_bound(candidate.begin(), candidate.end(), npc.rule.resource_id,
                    [](const auto& count, const auto& key) { return count.resource_id < key; });
                if (entry != candidate.end() && entry->resource_id == npc.rule.resource_id) {
                    if (entry->quantity > (std::numeric_limits<std::uint64_t>::max)() - npc.rule.quantity) {
                        Stop(entity, npc, ActionFailure::CreditOverflow);
                        continue;
                    }
                    entry->quantity += npc.rule.quantity;
                } else candidate.insert(entry, {npc.rule.resource_id, npc.rule.quantity});
            } catch (const std::bad_alloc&) {
                Stop(entity, npc, ActionFailure::CreditPreparation);
                continue;
            }
            World::EditResult edited{};
            try { edited = world.TryHarvestRemove(npc.target); }
            catch (...) {
                std::throw_with_nested(HarvestCommitFailed(HarvestCommitStage::WorldRemove, entity, npc.action.id, false, false));
            }
            if (edited.status != World::EditStatus::Changed) { Stop(entity, npc, ActionFailure::WorldRejected); continue; }
            if (fail_after_remove_) {
                fail_after_remove_ = false;
                throw HarvestCommitFailed(HarvestCommitStage::Credit, entity, npc.action.id, true, false);
            }
            // All allocations, lookups and validation precede the World commit. These
            // final publications are noexcept and no callback observes the intermediate state.
            npc.resources.swap(candidate);
            npc.action.phase = ActionPhase::Succeeded;
            npc.action.failure = ActionFailure::None;
        }
    }
}
