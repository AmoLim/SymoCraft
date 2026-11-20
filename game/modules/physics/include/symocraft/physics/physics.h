#pragma once

#include <symocraft/foundation/math.h>
#include <stdexcept>

namespace SymoCraft::Physics
{
    struct BodyState
    {
        glm::vec3 position{0.0f};
        glm::vec3 velocity{0.0f};
        glm::vec3 acceleration{0.0f};
        bool use_gravity = false;
        bool is_sensor = false;
        bool on_ground = false;
    };

    struct AabbShape
    {
        glm::vec3 size{1.0f};
        glm::vec3 offset{0.0f};
    };

    struct PhysicsSettings
    {
        glm::vec3 gravity{0.0f, 20.0f, 0.0f};
        glm::vec3 terminal_velocity{50.0f};
        float ground_tolerance = 0.002f;
        float fixed_step = 1.0f / 120.0f;
    };

    // Borrowed only during the call. The callback reads terrain; false means empty/nonsolid.
    struct TerrainCollisionQuery
    {
        const void* context = nullptr;
        bool (*is_solid)(const void*, const glm::ivec3&) = nullptr;
    };

    struct StepResult
    {
        BodyState body;
    };

    struct RayHit
    {
        bool hit = false;
        glm::ivec3 cell{0};
        glm::vec3 normal{0.0f};
        float distance = 0.0f;
    };

    class PhysicsParameterError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class PhysicsNumericalError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class PhysicsQueryError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    // Exactly one fixed step, with no accumulated timing or stored body/query state.
    // Inputs are unchanged on failure; only a returned value is eligible for caller commit.
    // Calls sharing borrowed terrain must obey its serialized, non-reentrant read contract.
    // Values must be finite, shape sizes/step positive, limits/tolerance nonnegative.
    // Unsafe parameters throw PhysicsParameterError; invalid computed values throw
    // PhysicsNumericalError. Callback faults throw PhysicsQueryError with a nested cause.
    [[nodiscard]] StepResult StepBody(const BodyState& body, const AabbShape& shape,
                                      TerrainCollisionQuery query, PhysicsSettings settings = {});

    // Unit-grid DDA. Invalid parameters throw; a valid miss has hit == false.
    // Coordinates and maximum distance retain the legacy 1,000,000-unit safety bound.
    // Callback faults retain their original cause as a nested PhysicsQueryError.
    [[nodiscard]] RayHit RayCast(const glm::vec3& origin, const glm::vec3& direction,
                                 float max_distance, TerrainCollisionQuery query);
}
