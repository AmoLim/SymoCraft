#include "symocraft/simulation/player_math.h"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace Math = SymoCraft::PlayerMath;

namespace
{
    void Require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    void CheckFixedSteps()
    {
        Math::FixedStepBudget clock;
        Require(clock.Consume(1.0f / 240.0f) == 0, "Half a tick advanced simulation");
        Require(clock.Consume(1.0f / 240.0f) == 1, "Two half ticks did not produce one tick");
        Require(clock.Consume(10.0f) == Math::FixedStepBudget::MaxSteps, "Long frame exceeded the step budget");
        Require(clock.Consume(1.0f / 120.0f) == 1, "Long frame left an unbounded backlog");
        Require(clock.Consume(-1.0f) == 0, "Negative delta advanced simulation");
        Require(clock.Consume(std::numeric_limits<float>::quiet_NaN()) == 0, "NaN delta advanced simulation");
        clock.Consume(1.0f / 240.0f);
        clock.Reset();
        Require(clock.Consume(1.0f / 240.0f) == 0, "Reset retained the previous remainder");
    }

    void CheckRays()
    {
        const std::array<glm::ivec3, 6> directions{
            glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0), glm::ivec3(0, 1, 0),
            glm::ivec3(0, -1, 0), glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)
        };
        for (const auto direction : directions)
        {
            const glm::ivec3 target = direction * 2;
            const auto solid = [target](glm::ivec3 cell) { return cell == target; };
            const auto hit = Math::RaycastVoxels(glm::vec3(0.5f), glm::vec3(direction) * 7.0f, 3.0f, solid);
            Require(hit.hit && hit.cell == target, "Axis-aligned ray missed its cell");
            Require(hit.normal == -glm::vec3(direction), "Axis-aligned ray returned the wrong face normal");
            Require(std::abs(hit.distance - 1.5f) < 0.0001f, "Ray distance depends on direction magnitude");
            Require(!Math::RaycastVoxels(glm::vec3(0.5f), glm::vec3(direction), 1.49f, solid).hit,
                    "Ray hit a block outside its range");
            Require(Math::RaycastVoxels(glm::vec3(0.5f), glm::vec3(direction), 1.5f, solid).hit,
                    "Ray missed the inclusive distance endpoint");
        }

        const auto negative = Math::RaycastVoxels(glm::vec3(0.0f, 0.5f, 0.5f), glm::vec3(-1, 0, 0), 3.0f,
            [](glm::ivec3 cell) { return cell == glm::ivec3(-1, 0, 0); });
        Require(negative.hit && negative.distance == 0.0f && negative.normal == glm::vec3(1, 0, 0),
                "Ray at a negative-facing integer boundary is incorrect");

        const auto corner = Math::RaycastVoxels(glm::vec3(0.5f), glm::vec3(1, 1, 0), 3.0f,
            [](glm::ivec3 cell) { return cell == glm::ivec3(1, 0, 0) || cell == glm::ivec3(2, 2, 0); });
        Require(corner.hit && corner.cell == glm::ivec3(2, 2, 0), "A corner-only contact blocked the ray");

        unsigned queries = 0;
        const auto count = [&queries](glm::ivec3) { ++queries; return false; };
        Require(!Math::RaycastVoxels(glm::vec3(0.5f), glm::vec3(0), 3.0f, count).hit && queries == 0,
                "A zero direction queried the world");
        Require(!Math::RaycastVoxels(glm::vec3(std::numeric_limits<float>::infinity()), glm::vec3(1, 0, 0), 3.0f, count).hit,
                "A nonfinite origin was accepted");
        Require(!Math::RaycastVoxels(glm::vec3(0.5f), glm::vec3(1, 0, 0), -1.0f, count).hit,
                "A negative ray distance was accepted");
        Require(!Math::RaycastVoxels(glm::vec3(-0.5f), glm::vec3(1, 0, 0), 3.0f, count).hit && queries < 10,
                "An empty-world ray did not terminate at its range");
    }

    void CheckPlacementOverlap()
    {
        Require(Math::Overlaps(glm::vec3(0), glm::vec3(1), glm::vec3(0.5f), glm::vec3(1.5f)),
                "Overlapping placement was accepted");
        Require(!Math::Overlaps(glm::vec3(0), glm::vec3(1), glm::vec3(1, 0, 0), glm::vec3(2, 1, 1)),
                "Touching faces were treated as penetration");
        Require(!Math::Overlaps(glm::vec3(-2), glm::vec3(-1), glm::vec3(0), glm::vec3(1)),
                "Separated negative-coordinate boxes overlap");
    }

    void CheckMovementAndGround()
    {
        const auto forward = Math::DesiredVelocity(glm::vec3(1, 0, 0), 0.0f, 4.4f, true);
        const auto diagonal = Math::DesiredVelocity(glm::vec3(1, 0, 1), 0.0f, 4.4f, true);
        Require(std::abs(glm::length(forward) - 4.4f) < 0.0001f &&
                std::abs(glm::length(diagonal) - 4.4f) < 0.0001f, "Diagonal movement changes speed");
        const auto rotated = Math::DesiredVelocity(glm::vec3(1, 0, 0), 90.0f, 6.2f, true);
        Require(std::abs(rotated.x) < 0.0001f && std::abs(rotated.z - 6.2f) < 0.0001f,
                "Movement does not follow yaw or running speed");
        Require(Math::DesiredVelocity(glm::vec3(0), 17.0f, 4.4f, true) == glm::vec3(0),
                "Released movement retains horizontal velocity");
        Require(Math::DesiredVelocity(glm::vec3(0, -1, 0), 0.0f, 4.4f, false) == glm::vec3(0, -1, 0),
                "Existing vertical-only sensor speed changed");

        const auto ground = [](glm::ivec3 cell) { return cell == glm::ivec3(0, 0, 0); };
        Require(Math::HasGroundSupport(glm::vec3(0.2f, 1.0f, 0.2f), glm::vec3(0.75f, 2.8f, 0.75f), ground),
                "Standing exactly on a block lost ground contact");
        Require(!Math::HasGroundSupport(glm::vec3(1.1f, 1.0f, 0.2f), glm::vec3(1.65f, 2.8f, 0.75f), ground),
                "Walking off the block retained ground contact");
        Require(!Math::HasGroundSupport(glm::vec3(0.2f, 1.1f, 0.2f), glm::vec3(0.75f, 2.9f, 0.75f), ground),
                "An airborne body was considered grounded");
    }
}

int main()
{
    try
    {
        CheckFixedSteps();
        CheckRays();
        CheckPlacementOverlap();
        CheckMovementAndGround();
        std::cout << "Player fixed-step, voxel-ray, and placement contracts passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Player math test failed: " << error.what() << '\n';
        return 1;
    }
}
