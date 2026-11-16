#include <symocraft/physics/physics.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>

namespace SymoCraft::Physics
{
    namespace
    {
        bool IsFinite(const glm::vec3& value)
        {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        }

        template<typename Error>
        int CheckedInteger(double value, const char* message)
        {
            if (!std::isfinite(value) || value < std::numeric_limits<int>::min() ||
                value > std::numeric_limits<int>::max())
                throw Error(message);
            return static_cast<int>(value);
        }

        void ValidateQuery(TerrainCollisionQuery query)
        {
            if (!query.is_solid)
                throw PhysicsParameterError("Terrain collision callback is missing");
        }

        bool IsSolid(TerrainCollisionQuery query, const glm::ivec3& cell)
        {
            try
            {
                return query.is_solid(query.context, cell);
            }
            catch (...)
            {
                std::throw_with_nested(PhysicsQueryError("Terrain collision query failed"));
            }
        }

        struct Bounds
        {
            glm::vec3 minimum;
            glm::vec3 maximum;
        };

        template<typename Error>
        Bounds BodyBounds(const BodyState& body, const AabbShape& shape, float tolerance)
        {
            const glm::vec3 center = body.position + shape.offset;
            const Bounds bounds{center - shape.size * 0.5f, center + shape.size * 0.5f};
            if (!IsFinite(center) || !IsFinite(bounds.minimum) || !IsFinite(bounds.maximum))
                throw Error("Body bounds are not finite");
            for (int axis = 0; axis < 3; ++axis)
            {
                CheckedInteger<Error>(std::ceil(bounds.minimum[axis]), "Body scan minimum is not representable");
                CheckedInteger<Error>(std::ceil(bounds.maximum[axis]), "Body scan maximum is not representable");
                if (axis != 1)
                {
                    CheckedInteger<Error>(std::floor(bounds.minimum[axis] + tolerance), "Body support minimum is not representable");
                    CheckedInteger<Error>(std::floor(bounds.maximum[axis] - tolerance), "Body support maximum is not representable");
                }
            }
            CheckedInteger<Error>(std::floor(bounds.minimum.y - tolerance), "Body support height is not representable");
            return bounds;
        }

        void ValidateParameters(const BodyState& body, const AabbShape& shape,
                                TerrainCollisionQuery query, const PhysicsSettings& settings)
        {
            ValidateQuery(query);
            if (!IsFinite(body.position) || !IsFinite(body.velocity) || !IsFinite(body.acceleration))
                throw PhysicsParameterError("Body position, velocity and acceleration must be finite");
            if (!IsFinite(shape.size) || !IsFinite(shape.offset) ||
                glm::any(glm::lessThanEqual(shape.size, glm::vec3(0.0f))))
                throw PhysicsParameterError("AABB size must be positive and size/offset finite");
            if (!IsFinite(settings.gravity) || !IsFinite(settings.terminal_velocity) ||
                glm::any(glm::lessThan(settings.terminal_velocity, glm::vec3(0.0f))) ||
                !std::isfinite(settings.ground_tolerance) || settings.ground_tolerance < 0.0f ||
                !std::isfinite(settings.fixed_step) || settings.fixed_step <= 0.0f)
                throw PhysicsParameterError("Physics settings require finite gravity, nonnegative limits/tolerance and positive step");
            if (!body.is_sensor)
                BodyBounds<PhysicsParameterError>(body, shape, settings.ground_tolerance);
        }

        bool Overlaps(const Bounds& body, const glm::vec3& voxel_center)
        {
            const glm::vec3 minimum = voxel_center - glm::vec3(0.5f);
            const glm::vec3 maximum = voxel_center + glm::vec3(0.5f);
            for (int axis = 0; axis < 3; ++axis)
                if (std::min(body.maximum[axis], maximum[axis]) -
                    std::max(body.minimum[axis], minimum[axis]) <= 0.0f)
                    return false;
            return true;
        }

        struct Collision
        {
            glm::vec3 overlap;
            int axis;
            bool bottom;
        };

        Collision CollisionInformation(const BodyState& body, const AabbShape& shape,
                                       const glm::vec3& voxel_center)
        {
            const glm::vec3 relative = body.position + shape.offset - voxel_center;
            const glm::vec3 direction(relative.x > 0.0f ? 1.0f : -1.0f,
                                      relative.y > 0.0f ? 1.0f : -1.0f,
                                      relative.z > 0.0f ? 1.0f : -1.0f);
            glm::vec3 expanded_size(1.0f);
            expanded_size += shape.size;
            const glm::vec3 quadrant = expanded_size * direction * 0.5f + voxel_center;
            const glm::vec3 delta = body.position + shape.offset - quadrant;
            const glm::vec3 absolute = glm::abs(delta);
            // Preserve the old strict branches, including nonminimal x/y ties falling to z.
            const int axis = absolute.x < absolute.y && absolute.x < absolute.z ? 0 :
                             absolute.y < absolute.x && absolute.y < absolute.z ? 1 : 2;
            glm::vec3 overlap(0.0f);
            overlap[axis] = delta[axis];
            return {overlap, axis, axis == 1 && relative.y > 0.0f};
        }

        bool HasGroundSupport(const Bounds& bounds, TerrainCollisionQuery query, float tolerance)
        {
            const int y = CheckedInteger<PhysicsNumericalError>(std::floor(bounds.minimum.y - tolerance),
                                                               "Ground cell is not representable");
            if (std::abs(bounds.minimum.y - static_cast<float>(static_cast<std::int64_t>(y) + 1)) > tolerance)
                return false;
            const int left = CheckedInteger<PhysicsNumericalError>(std::floor(bounds.minimum.x + tolerance),
                                                                  "Ground x minimum is not representable");
            const int right = CheckedInteger<PhysicsNumericalError>(std::floor(bounds.maximum.x - tolerance),
                                                                   "Ground x maximum is not representable");
            const int back = CheckedInteger<PhysicsNumericalError>(std::floor(bounds.minimum.z + tolerance),
                                                                  "Ground z minimum is not representable");
            const int front = CheckedInteger<PhysicsNumericalError>(std::floor(bounds.maximum.z - tolerance),
                                                                   "Ground z maximum is not representable");
            for (std::int64_t x = left; x <= right; ++x)
                for (std::int64_t z = back; z <= front; ++z)
                    if (IsSolid(query, {static_cast<int>(x), y, static_cast<int>(z)}))
                        return true;
            return false;
        }

        void ResolveCollision(BodyState& body, const AabbShape& shape,
                              TerrainCollisionQuery query, float tolerance)
        {
            body.on_ground = false;
            const Bounds initial = BodyBounds<PhysicsNumericalError>(body, shape, tolerance);
            const glm::ivec3 low(glm::ceil(initial.minimum));
            const glm::ivec3 high(glm::ceil(initial.maximum));
            for (std::int64_t y = high.y; y >= low.y; --y)
                for (std::int64_t x = low.x; x <= high.x; ++x)
                    for (std::int64_t z = low.z; z <= high.z; ++z)
                    {
                        const glm::vec3 voxel_center(static_cast<float>(x) - 0.5f,
                                                     static_cast<float>(y) - 0.5f,
                                                     static_cast<float>(z) - 0.5f);
                        glm::ivec3 cell;
                        for (int axis = 0; axis < 3; ++axis)
                            cell[axis] = CheckedInteger<PhysicsNumericalError>(
                                std::floor(static_cast<double>(voxel_center[axis])),
                                "Collision cell is not representable");
                        if (!IsSolid(query, cell) ||
                            !Overlaps(BodyBounds<PhysicsNumericalError>(body, shape, tolerance), voxel_center))
                            continue;
                        const Collision collision = CollisionInformation(body, shape, voxel_center);
                        const float dot = glm::dot(collision.overlap, body.velocity);
                        if (!IsFinite(collision.overlap) || !std::isfinite(dot))
                            throw PhysicsNumericalError("Collision correction is not finite");
                        if (dot < 0.0f)
                            continue;
                        body.position -= collision.overlap;
                        body.velocity[collision.axis] = 0.0f;
                        body.acceleration[collision.axis] = 0.0f;
                        body.on_ground = body.on_ground || collision.bottom;
                        BodyBounds<PhysicsNumericalError>(body, shape, tolerance);
                    }
            if (body.velocity.y <= 0.0f && HasGroundSupport(
                BodyBounds<PhysicsNumericalError>(body, shape, tolerance), query, tolerance))
            {
                body.on_ground = true;
                body.velocity.y = 0.0f;
                body.acceleration.y = 0.0f;
            }
        }
    }

    StepResult StepBody(const BodyState& input, const AabbShape& shape,
                        TerrainCollisionQuery query, PhysicsSettings settings)
    {
        ValidateParameters(input, shape, query, settings);
        BodyState body = input;
        body.position += body.velocity * settings.fixed_step;
        body.velocity += body.acceleration * settings.fixed_step;
        if (body.use_gravity)
            body.velocity -= settings.gravity * settings.fixed_step;
        if (!IsFinite(body.position) || !IsFinite(body.velocity))
            throw PhysicsNumericalError("Body integration is not finite");
        body.velocity = glm::clamp(body.velocity, -settings.terminal_velocity, settings.terminal_velocity);
        if (body.is_sensor)
            body.on_ground = false;
        else
            ResolveCollision(body, shape, query, settings.ground_tolerance);
        return {body};
    }

    RayHit RayCast(const glm::vec3& origin, const glm::vec3& direction,
                   float max_distance, TerrainCollisionQuery query)
    {
        ValidateQuery(query);
        const float length_squared = glm::dot(direction, direction);
        constexpr float coordinate_limit = 1000000.0f;
        if (!IsFinite(origin) || !IsFinite(direction) || !std::isfinite(length_squared) ||
            length_squared <= 0.0f || !std::isfinite(max_distance) || max_distance < 0.0f ||
            glm::any(glm::greaterThan(glm::abs(origin), glm::vec3(coordinate_limit))) ||
            max_distance > coordinate_limit)
            throw PhysicsParameterError("Ray origin/direction/distance is invalid or outside the safe coordinate range");

        const glm::vec3 ray = direction / std::sqrt(length_squared);
        if (!IsFinite(ray))
            throw PhysicsNumericalError("Normalized ray is not finite");
        glm::ivec3 cell(glm::floor(origin));
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
            if (IsSolid(query, cell))
                return {true, cell, normal, distance};
            distance = std::min(next.x, std::min(next.y, next.z));
            if (!std::isfinite(distance) || distance > max_distance)
                return {};
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
