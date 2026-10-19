#pragma once

#include <symocraft/ecs/registry.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/physics_system.h>
#include <symocraft/world/world.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace CpuBaseline {
    using namespace SymoCraft;
    inline constexpr float Step = 1.0f / 120.0f;

    inline void Require(bool condition, std::string_view field)
    {
        if (!condition) throw std::runtime_error(std::string(field));
    }

    inline void Near(double actual, double expected, std::string_view field, double tolerance = 1e-4)
    {
        if (!std::isfinite(actual) || !std::isfinite(expected) || std::abs(actual - expected) > tolerance) {
            std::ostringstream message;
            message << std::setprecision(17) << field << ": actual=" << actual
                    << ", expected=" << expected << ", absolute_tolerance=" << tolerance;
            throw std::runtime_error(message.str());
        }
    }

    inline void Near(const glm::vec3& actual, const glm::vec3& expected, std::string_view field)
    {
        for (int axis = 0; axis < 3; ++axis)
            Near(actual[axis], expected[axis], std::string(field) + "[" + std::to_string(axis) + "]");
    }

    struct Snapshot {
        Transform transform;
        Physics::RigidBody body;
        Physics::HitBox shape;
    };

    class Fixture {
    public:
        explicit Fixture(const char* configuration)
        {
            std::ifstream file(configuration, std::ios::binary);
            Require(static_cast<bool>(file), "Cannot read block configuration");
            const std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
            world = World::VoxelWorld::Create(World::BlockDefinition::FromConfig(text), {424242, 2, false});
            registry.RegisterComponent<Transform>("Transform");
            registry.RegisterComponent<Physics::RigidBody>("RigidBody");
            registry.RegisterComponent<Physics::HitBox>("HitBox");
            registry.RegisterComponent<Character::CharacterComponent>("Character");
            registry.RegisterComponent<Character::PlayerComponent>("Player");
            // Each fixture owns its budget; no production process-global timing remains.
        }

        ~Fixture() = default;

        ECS::EntityId AddBody(const glm::vec3& position, const Physics::RigidBody& body,
                             const Physics::HitBox& shape = {glm::vec3(1.0f), glm::vec3(0.0f)})
        {
            const auto entity = registry.CreateEntity();
            auto& transform = registry.AddComponent<Transform>(entity);
            transform = {};
            transform.position = position;
            transform.scale = glm::vec3(1.0f);
            registry.AddComponent<Physics::RigidBody>(entity) = body;
            registry.AddComponent<Physics::HitBox>(entity) = shape;
            return entity;
        }

        Snapshot Capture(ECS::EntityId entity) const
        {
            return {registry.GetComponent<Transform>(entity),
                    registry.GetComponent<Physics::RigidBody>(entity),
                    registry.GetComponent<Physics::HitBox>(entity)};
        }

        void SetBlock(World::BlockCoord cell, World::BlockId id = 5)
        {
            const auto result = world->TryEdit({World::EditOperation::Set, cell, id});
            Require(result.Accepted(), "Fixture block edit rejected");
            Require(world->QueryBlock(cell).block.block_id == id, "Fixture block edit not observable");
        }

        void RemoveBlock(World::BlockCoord cell)
        {
            Require(world->TryEdit({World::EditOperation::Remove, cell}).Accepted(), "Fixture removal rejected");
        }

        std::unique_ptr<World::VoxelWorld> world;
        ECS::Registry registry;
        PlayerMath::FixedStepBudget budget;
    };
}
