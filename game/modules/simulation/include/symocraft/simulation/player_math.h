#pragma once

#include <symocraft/foundation/math.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace SymoCraft::PlayerMath
{
    inline bool IsFinite(const glm::vec3& value)
    {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    class FixedStepBudget
    {
    public:
        static constexpr double Step = 1.0 / 120.0;
        static constexpr unsigned MaxSteps = 8;

        unsigned Consume(float frame_seconds)
        {
            if (!std::isfinite(frame_seconds) || frame_seconds <= 0.0f)
                return 0;
            accumulated_ += std::min(static_cast<double>(frame_seconds), Step * MaxSteps);
            const auto steps = std::min(static_cast<unsigned>((accumulated_ + 1e-9) / Step), MaxSteps);
            accumulated_ = std::max(0.0, accumulated_ - steps * Step);
            return steps;
        }

        void Reset() { accumulated_ = 0.0; }

    private:
        double accumulated_ = 0.0;
    };

    inline bool Overlaps(const glm::vec3& first_min, const glm::vec3& first_max,
                         const glm::vec3& second_min, const glm::vec3& second_max,
                         float tolerance = 0.0f)
    {
        for (int axis = 0; axis < 3; ++axis)
            if (std::min(first_max[axis], second_max[axis]) -
                std::max(first_min[axis], second_min[axis]) <= tolerance)
                return false;
        return true;
    }

    inline glm::vec3 DesiredVelocity(const glm::vec3& movement, float yaw_degrees,
                                     float speed, bool use_gravity)
    {
        const float angle = glm::radians(yaw_degrees);
        const glm::vec3 front(std::cos(angle), 0.0f, std::sin(angle));
        const glm::vec3 right(-front.z, 0.0f, front.x);
        glm::vec3 velocity = front * movement.x + right * movement.z;
        if (!use_gravity)
            velocity.y = movement.y;
        if (velocity.x != 0.0f || velocity.z != 0.0f)
            velocity *= speed / std::sqrt(glm::dot(velocity, velocity));
        return velocity;
    }

    template<typename IsSolid>
    bool HasGroundSupport(const glm::vec3& minimum, const glm::vec3& maximum, IsSolid&& is_solid)
    {
        constexpr float tolerance = 0.002f;
        const int y = static_cast<int>(std::floor(minimum.y - tolerance));
        if (std::abs(minimum.y - static_cast<float>(y + 1)) > tolerance)
            return false;
        for (int x = static_cast<int>(std::floor(minimum.x + tolerance));
             x <= static_cast<int>(std::floor(maximum.x - tolerance)); ++x)
            for (int z = static_cast<int>(std::floor(minimum.z + tolerance));
                 z <= static_cast<int>(std::floor(maximum.z - tolerance)); ++z)
                if (is_solid(glm::ivec3(x, y, z)))
                    return true;
        return false;
    }

    struct VoxelRayHit
    {
        bool hit = false;
        glm::ivec3 cell{0};
        glm::vec3 normal{0.0f};
        float distance = 0.0f;
    };

    // Unit-grid DDA. Equal crossings advance together; edge-only cells are not hits.
    template<typename IsSolid>
    VoxelRayHit RaycastVoxels(const glm::vec3& origin, const glm::vec3& direction,
                             float max_distance, IsSolid&& is_solid)
    {
        VoxelRayHit result;
        const float length_squared = glm::dot(direction, direction);
        if (!IsFinite(origin) || !IsFinite(direction) || !std::isfinite(length_squared) ||
            length_squared <= 0.0f || !std::isfinite(max_distance) || max_distance < 0.0f)
            return result;

        // The game's finite world is much smaller; keep invalid input away from integer overflow.
        constexpr float coordinate_limit = 1000000.0f;
        if (glm::any(glm::greaterThan(glm::abs(origin), glm::vec3(coordinate_limit))) ||
            max_distance > coordinate_limit)
            return result;

        const glm::vec3 ray = direction / std::sqrt(length_squared);
        glm::ivec3 cell = glm::ivec3(glm::floor(origin));
        glm::ivec3 step(0);
        glm::vec3 next(std::numeric_limits<float>::infinity());
        glm::vec3 interval(std::numeric_limits<float>::infinity());
        glm::vec3 normal(0.0f);
        for (int axis = 0; axis < 3; ++axis)
        {
            if (ray[axis] == 0.0f)
                continue;
            step[axis] = ray[axis] > 0.0f ? 1 : -1;
            if (step[axis] < 0 && origin[axis] == static_cast<float>(cell[axis]))
            {
                --cell[axis];
                if (normal == glm::vec3(0.0f))
                    normal[axis] = 1.0f;
            }
            interval[axis] = std::abs(1.0f / ray[axis]);
            const float boundary = static_cast<float>(cell[axis] + (step[axis] > 0 ? 1 : 0));
            next[axis] = (boundary - origin[axis]) / ray[axis];
        }

        float distance = 0.0f;
        for (;;)
        {
            if (is_solid(cell))
                return {true, cell, normal, distance};
            distance = std::min(next.x, std::min(next.y, next.z));
            if (!std::isfinite(distance) || distance > max_distance)
                return result;
            normal = glm::vec3(0.0f);
            for (int axis = 0; axis < 3; ++axis)
            {
                if (next[axis] == distance)
                {
                    if (normal == glm::vec3(0.0f))
                        normal[axis] = static_cast<float>(-step[axis]);
                    cell[axis] += step[axis];
                    next[axis] += interval[axis];
                }
            }
        }
    }
}
