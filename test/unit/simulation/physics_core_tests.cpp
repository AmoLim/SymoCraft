#include <symocraft/physics/physics.h>

#include <array>
#include <cmath>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace {
    using namespace SymoCraft::Physics;
    constexpr float Step = 1.0f / 120.0f;

    void Require(bool condition, std::string_view field)
    {
        if (!condition) throw std::runtime_error(std::string(field));
    }

    void Near(float actual, float expected, std::string_view field)
    {
        if (!std::isfinite(actual) || !std::isfinite(expected) || std::abs(actual - expected) > 1e-4f) {
            std::ostringstream message;
            message << field << ": actual=" << actual << ", expected=" << expected;
            throw std::runtime_error(message.str());
        }
    }

    void Near(const glm::vec3& actual, const glm::vec3& expected, std::string_view field)
    {
        for (int axis = 0; axis < 3; ++axis)
            Near(actual[axis], expected[axis], std::string(field) + "[" + std::to_string(axis) + "]");
    }

    void Same(const BodyState& actual, const BodyState& expected, std::string_view field)
    {
        Require(actual.position == expected.position && actual.velocity == expected.velocity &&
            actual.acceleration == expected.acceleration && actual.use_gravity == expected.use_gravity &&
            actual.is_sensor == expected.is_sensor && actual.on_ground == expected.on_ground, field);
    }

    template<class Error, class Call>
    void Throws(Call&& call, std::string_view field)
    {
        bool caught = false;
        try { call(); }
        catch (const Error&) { caught = true; }
        Require(caught, field);
    }

    struct Terrain {
        std::set<std::tuple<int, int, int>> solids;

        void Add(glm::ivec3 cell) { solids.emplace(cell.x, cell.y, cell.z); }
        void Remove(glm::ivec3 cell) { solids.erase({cell.x, cell.y, cell.z}); }

        static bool IsSolid(const void* context, const glm::ivec3& cell)
        {
            const auto& terrain = *static_cast<const Terrain*>(context);
            return terrain.solids.contains({cell.x, cell.y, cell.z});
        }

        TerrainCollisionQuery Query() const { return {this, IsSolid}; }
    };

    bool Empty(const void*, const glm::ivec3&) { return false; }
    constexpr TerrainCollisionQuery EmptyQuery{nullptr, Empty};
    const TerrainCollisionQuery QueryMustNotRun{nullptr, [](const void*, const glm::ivec3&) -> bool {
        throw std::runtime_error("unexpected terrain query");
    }};

    void SingleStepRegression()
    {
        struct Case {
            const char* name;
            BodyState input, expected;
            AabbShape shape{};
            std::initializer_list<glm::ivec3> solids;
        };
        // Literal legacy outcomes, including strict nonminimum ties and scan order.
        const Case cases[]{
            {"integration", {{0.5f, 200.5f, 0.5f}, {12, 6, -3}, {120, 60, -240}, true},
                {{0.6f, 200.55f, 0.475f}, {13, 6.333333333f, -5}, {120, 60, -240}, true}},
            {"sensor", {{0.5f, 200.5f, 0.5f}, {60, -60, 60}, {120, -120, 120}, true, true, true},
                {{1, 200, 1}, {50, -50, 50}, {120, -120, 120}, true, true}},
            {"wall", {{0.45f, 200.5f, 0.5f}, {12, 0, 0}, {3, 4, 5}},
                {{0.5f, 200.5f, 0.5f}, {0, 4 * Step, 5 * Step}, {0, 4, 5}}, {}, {{1, 200, 0}}},
            {"ceiling", {{4.5f, 200.45f, 0.5f}, {0, 12, 0}, {2, 3, 4}},
                {{4.5f, 200.5f, 0.5f}, {2 * Step, 0, 4 * Step}, {2, 0, 4}}, {}, {{4, 201, 0}}},
            {"corner-tie-z", {{-0.25f, 200.5f, -0.25f}, {6, 0, 6}},
                {{-0.2f, 200.5f, -0.5f}, {6, 0, 0}}, {}, {{0, 200, 0}}},
            {"moving-out", {{3.8f, 200.5f, 0.5f}, {-6, 0, 0}, {-2, 0, 0}},
                {{3.75f, 200.5f, 0.5f}, {-6 - 2 * Step, 0, 0}, {-2, 0, 0}}, {}, {{4, 200, 0}}},
            {"nonminimum-xy-tie-z", {{1.125f, 201.125f, 0.5f}}, {{1.125f, 201.125f, -0.25f}},
                {glm::vec3(0.5f), glm::vec3(0)}, {{0, 200, 0}}},
            {"scan-y-descending", {{0.5f, 201, 1}}, {{0.5f, 201, 1.25f}},
                {{0.5f, 1, 0.5f}, glm::vec3(0)}, {{0, 201, 1}, {0, 200, 0}}},
            {"scan-x-ascending", {{1, 200.5f, 1}}, {{1, 200.5f, 0.75f}},
                {{1, 0.5f, 0.5f}, glm::vec3(0)}, {{0, 200, 0}, {1, 200, 1}}},
            {"scan-z-ascending", {{0.5f, 200.5f, 1}}, {{0.5f, 200.5f, 0.5f}},
                {{0.5f, 0.5f, 1}, glm::vec3(0)}, {{0, 200, 0}, {0, 200, 1}}}
        };
        for (const auto& test : cases) {
            try {
                Terrain terrain;
                for (auto cell : test.solids) terrain.Add(cell);
                const auto before = test.input;
                const auto query = test.input.is_sensor ? QueryMustNotRun : terrain.Query();
                const auto result = StepBody(test.input, test.shape, query).body;
                Near(result.position, test.expected.position, "position");
                Near(result.velocity, test.expected.velocity, "velocity");
                Near(result.acceleration, test.expected.acceleration, "acceleration");
                Require(result.on_ground == test.expected.on_ground && result.is_sensor == test.expected.is_sensor &&
                    result.use_gravity == test.expected.use_gravity, "flags");
                Same(test.input, before, "input-not-mutated");
            } catch (const std::exception& error) {
                throw std::runtime_error(std::string(test.name) + ": " + error.what());
            }
        }
    }

    void SupportAndCoordinates()
    {
        Terrain terrain;
        terrain.Add({0, 200, 0});
        BodyState body{};
        body.position = {0.5f, 201.55f, 0.5f};
        body.velocity = {0, -12, 0};
        body.acceleration = {0, -2, 0};
        body.use_gravity = true;
        auto result = StepBody(body, {}, terrain.Query()).body;
        Near(result.position, {0.5f, 201.5f, 0.5f}, "landing.position");
        Near(result.velocity, glm::vec3(0), "landing.velocity");
        Near(result.acceleration, glm::vec3(0), "landing.acceleration");
        Require(result.on_ground, "landing.ground");
        result = StepBody(result, {}, terrain.Query()).body;
        Require(result.on_ground, "strict-face-contact.support");
        terrain.Remove({0, 200, 0});
        result = StepBody(result, {}, terrain.Query()).body;
        Require(!result.on_ground, "removed-support.ground");
        Near(result.velocity.y, -20 * Step, "removed-support.gravity");
        Near(result.position.y, 201.5f, "removed-support.position-before-gravity");
        terrain.Add({0, 200, 0});
        body = {};
        body.position = {0.5f, 201.501f, 0.5f};
        Require(StepBody(body, {}, terrain.Query()).body.on_ground, "ground.within-tolerance");
        body.position.y = 201.503f;
        Require(!StepBody(body, {}, terrain.Query()).body.on_ground, "ground.outside-tolerance");
        body.position.y = 201.5f;
        body.velocity.y = 1;
        Require(!StepBody(body, {}, terrain.Query()).body.on_ground, "ground.upward-velocity");
        const AabbShape shape{glm::vec3(1), {0.25f, 0.25f, 0}};
        for (int x : {-17, -16, -1, 0, 15, 16}) {
            terrain = {};
            terrain.Add({x, 200, 3});
            body = {};
            body.position = {x + 0.25f, 201.25f, 3.5f};
            result = StepBody(body, shape, terrain.Query()).body;
            Require(result.on_ground, "offset-negative-chunk-boundary.support");
            Near(result.position, body.position, "offset.position");
            body.position.y = 201.3f;
            body.velocity.y = -12;
            result = StepBody(body, shape, terrain.Query()).body;
            Near(result.position.y, 201.25f, "offset.correction");
            Near(result.velocity, glm::vec3(0), "offset.velocity");
            Require(result.on_ground, "offset.corrected-ground");
        }
    }

    void ReadOnlyQueriesAndOwnedResults()
    {
        StepResult retained{};
        RayHit retained_ray{};
        {
            Terrain terrain;
            terrain.Add({0, 200, 0});
            const auto before = terrain.solids;
            BodyState body{};
            body.position = {0.5f, 201.5f, 0.5f};
            retained = StepBody(body, {}, terrain.Query());
            retained_ray = RayCast({0.5f, 202, 0.5f}, {0, -1, 0}, 3, terrain.Query());
            Require(terrain.solids == before, "query.terrain-read-only");
            Require(retained.body.on_ground && retained_ray.hit, "query.actual-support-and-ray");
            terrain.Remove({0, 200, 0});
            Require(!StepBody(body, {}, terrain.Query()).body.on_ground, "query.current-support");
            Require(!RayCast({0.5f, 202, 0.5f}, {0, -1, 0}, 3, terrain.Query()).hit, "query.current-ray");
        }
        Near(retained.body.position, {0.5f, 201.5f, 0.5f}, "owned-step.after-query-owner-destruction");
        Require(retained.body.on_ground && retained_ray.hit && retained_ray.cell == glm::ivec3(0, 200, 0),
            "owned-result.not-aliased-to-next-call");
        Near(retained_ray.distance, 1, "owned-ray.distance");
    }

    void RayTraversal()
    {
        for (const auto direction : std::array<glm::ivec3, 6>{glm::ivec3(1, 0, 0), {-1, 0, 0},
                {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}) {
            Terrain terrain;
            const auto cell = glm::ivec3(0, 200, 0) + direction * 2;
            terrain.Add(cell);
            const auto hit = RayCast({0.5f, 200.5f, 0.5f}, glm::vec3(direction) * 7.0f, 1.5f, terrain.Query());
            Require(hit.hit && hit.cell == cell, "ray.inclusive-endpoint-cell");
            Require(hit.normal == -glm::vec3(direction), "ray.axis-normal");
            Near(hit.distance, 1.5f, "ray.distance");
            Require(!RayCast({0.5f, 200.5f, 0.5f}, glm::vec3(direction), 1.49f, terrain.Query()).hit,
                "ray.out-of-range");
        }
        Terrain terrain;
        terrain.Add({-1, 200, 0});
        const auto boundary = RayCast({0, 200.5f, 0.5f}, {-1, 0, 0}, 0, terrain.Query());
        Require(boundary.hit && boundary.cell == glm::ivec3(-1, 200, 0) && boundary.normal == glm::vec3(1, 0, 0),
            "ray.negative-integer-boundary");
        Near(boundary.distance, 0, "ray.zero-distance");
        const auto inside = RayCast({-0.5f, 200.5f, 0.5f}, {1, 0, 0}, 0, terrain.Query());
        Require(inside.hit && inside.normal == glm::vec3(0), "ray.start-inside-normal");
        terrain = {};
        terrain.Add({1, 0, 0});
        terrain.Add({0, 1, 0});
        Require(!RayCast({0.5f, 0.5f, 0.5f}, {1, 1, 0}, 2, terrain.Query()).hit, "ray.skips-edge-only-cells");
        terrain.Add({1, 1, 0});
        const auto xy = RayCast({0.5f, 0.5f, 0.5f}, {1, 1, 0}, 2, terrain.Query());
        Require(xy.hit && xy.cell == glm::ivec3(1, 1, 0) && xy.normal == glm::vec3(-1, 0, 0),
            "ray.xy-crossing-first-axis-normal");
        Near(xy.distance, std::sqrt(0.5f), "ray.xy-distance");
        terrain = {};
        terrain.Add({1, 1, 1});
        const auto xyz = RayCast({0.5f, 0.5f, 0.5f}, {1, 1, 1}, 2, terrain.Query());
        Require(xyz.hit && xyz.cell == glm::ivec3(1) && xyz.normal == glm::vec3(-1, 0, 0),
            "ray.xyz-crossing");
        Near(xyz.distance, std::sqrt(0.75f), "ray.xyz-distance");
        terrain = {};
        terrain.Add({-1, -1, -1});
        const auto negative = RayCast(glm::vec3(0), glm::vec3(-1), 0, terrain.Query());
        Require(negative.hit && negative.cell == glm::ivec3(-1) && negative.normal == glm::vec3(1, 0, 0),
            "ray.negative-triple-integer-boundary");
    }

    void FixedStepSettingsAndIsolation()
    {
        BodyState body{};
        body.position = {0.5f, 200.5f, 0.5f};
        body.velocity = {2, 3, -4};
        body.acceleration = {1, 2, 3};
        body.is_sensor = body.use_gravity = true;
        PhysicsSettings settings{};
        settings.fixed_step = 0.5f;
        settings.gravity = {0, 2, 0};
        settings.terminal_velocity = {2.25f, 5, 2};
        const auto result = StepBody(body, {}, EmptyQuery, settings).body;
        Near(result.position, {1.5f, 202, -1.5f}, "settings.supplied-single-step");
        Near(result.velocity, {2.25f, 3, -2}, "settings.gravity-and-axis-limits");
        const auto first = StepBody(body, {}, EmptyQuery).body;
        BodyState other{};
        other.position = {32, -2, -16};
        other.velocity = {8, 0, 0};
        static_cast<void>(StepBody(other, {}, EmptyQuery));
        Same(StepBody(body, {}, EmptyQuery).body, first, "independent-call.no-global-budget");
        Terrain terrain;
        terrain.Add({0, 200, 0});
        BodyState grounded{};
        grounded.position = {0.5f, 201.503f, 0.5f};
        settings = {};
        settings.ground_tolerance = 0.004f;
        Require(StepBody(grounded, {}, terrain.Query(), settings).body.on_ground, "settings.ground-tolerance");
        for (float x : {2147483520.0f, -2147483648.0f}) {
            BodyState boundary{};
            boundary.position = {x, 200.5f, 0.5f};
            Same(StepBody(boundary, {}, EmptyQuery).body, boundary, "scan.safe-i32-edge-loop");
        }
    }

    void ParameterValidation()
    {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float inf = std::numeric_limits<float>::infinity();
        const auto preflight = QueryMustNotRun;
        for (int axis = 0; axis < 3; ++axis) {
            for (float invalid : {nan, inf, -inf}) {
                for (auto member : {&BodyState::position, &BodyState::velocity, &BodyState::acceleration}) {
                    BodyState body{};
                    (body.*member)[axis] = invalid;
                    Throws<PhysicsParameterError>([&] { return StepBody(body, {}, preflight); }, "body.nonfinite");
                }
                AabbShape shape{};
                shape.offset[axis] = invalid;
                Throws<PhysicsParameterError>([&] { return StepBody({}, shape, preflight); }, "shape.nonfinite-offset");
                shape = {};
                shape.size[axis] = invalid;
                Throws<PhysicsParameterError>([&] { return StepBody({}, shape, preflight); }, "shape.nonfinite-size");
                PhysicsSettings settings{};
                settings.gravity[axis] = invalid;
                Throws<PhysicsParameterError>([&] { return StepBody({}, {}, preflight, settings); }, "settings.nonfinite-gravity");
                settings = {};
                settings.terminal_velocity[axis] = invalid;
                Throws<PhysicsParameterError>([&] { return StepBody({}, {}, preflight, settings); }, "settings.nonfinite-limit");
            }
            for (float invalid : {0.0f, -1.0f}) {
                AabbShape shape{};
                shape.size[axis] = invalid;
                Throws<PhysicsParameterError>([&] { return StepBody({}, shape, preflight); }, "shape.nonpositive-size");
            }
            PhysicsSettings settings{};
            settings.terminal_velocity[axis] = -1;
            Throws<PhysicsParameterError>([&] { return StepBody({}, {}, preflight, settings); }, "settings.negative-limit");
        }
        for (float invalid : {0.0f, -1.0f, nan, inf}) {
            PhysicsSettings settings{};
            settings.fixed_step = invalid;
            Throws<PhysicsParameterError>([&] { return StepBody({}, {}, preflight, settings); }, "settings.invalid-step");
        }
        for (float invalid : {-1.0f, nan, inf}) {
            PhysicsSettings settings{};
            settings.ground_tolerance = invalid;
            Throws<PhysicsParameterError>([&] { return StepBody({}, {}, preflight, settings); }, "settings.invalid-tolerance");
        }
        Throws<PhysicsParameterError>([] { return StepBody({}, {}, {}); }, "query.missing-callback");
        BodyState sensor{};
        sensor.is_sensor = true;
        Throws<PhysicsParameterError>([&] { return StepBody(sensor, {}, {}); }, "sensor.missing-callback");
        BodyState bad_coordinate{};
        bad_coordinate.position.x = std::numeric_limits<float>::max();
        Throws<PhysicsParameterError>([&] { return StepBody(bad_coordinate, {}, preflight); }, "scan.coordinate-not-representable");
    }

    void RayValidationAndNoHit()
    {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float inf = std::numeric_limits<float>::infinity();
        const auto preflight = QueryMustNotRun;
        for (int axis = 0; axis < 3; ++axis) {
            for (float invalid : {nan, inf, -inf}) {
                glm::vec3 origin(0), direction(1, 0, 0);
                origin[axis] = invalid;
                Throws<PhysicsParameterError>([&] { return RayCast(origin, direction, 3, preflight); }, "ray.nonfinite-origin");
                origin = glm::vec3(0);
                direction[axis] = invalid;
                Throws<PhysicsParameterError>([&] { return RayCast(origin, direction, 3, preflight); }, "ray.nonfinite-direction");
            }
        }
        for (float invalid : {-1.0f, nan, inf, 1000001.0f})
            Throws<PhysicsParameterError>([&] { return RayCast({}, {1, 0, 0}, invalid, preflight); }, "ray.invalid-distance");
        Throws<PhysicsParameterError>([&] { return RayCast({}, {}, 3, preflight); }, "ray.zero-direction");
        Throws<PhysicsParameterError>([&] { return RayCast({1000001, 0, 0}, {1, 0, 0}, 3, preflight); }, "ray.coordinate-limit");
        Throws<PhysicsParameterError>([] { return RayCast({}, {1, 0, 0}, 3, {}); }, "ray.missing-callback");
        Require(!RayCast({}, {1, 0, 0}, 0, EmptyQuery).hit, "ray.valid-zero-distance-no-hit");
        Require(!RayCast({1000000, 0, 0}, {1, 0, 0}, 0, EmptyQuery).hit, "ray.valid-coordinate-limit");
        Require(!RayCast({}, {1, 0, 0}, 3, EmptyQuery).hit, "ray.valid-empty-no-hit");
        Terrain terrain;
        terrain.Add({1000001, 0, 0});
        const auto edge = RayCast({1000000, 0.5f, 0.5f}, {1, 0, 0}, 1, terrain.Query());
        Require(edge.hit && edge.cell == glm::ivec3(1000001, 0, 0), "ray.origin-bound-not-traversed-cell-bound");
        Near(edge.distance, 1, "ray.origin-limit-endpoint");
    }

    void NumericalAndQueryFailures()
    {
        BodyState body{};
        body.position = {0.5f, 200.5f, 0.5f};
        body.velocity = {0, -12, 0};
        body.on_ground = true;
        const auto before = body;
        struct QueryFailure : std::runtime_error { QueryFailure() : std::runtime_error("fixture query fault") {} };
        const TerrainCollisionQuery faulty{nullptr, [](const void*, const glm::ivec3&) -> bool { throw QueryFailure(); }};
        const auto nested_fault = [&](auto&& call) {
            bool outer = false, nested = false;
            try { call(); }
            catch (const PhysicsQueryError& error) {
                outer = true;
                try { std::rethrow_if_nested(error); }
                catch (const QueryFailure&) { nested = true; }
            }
            Require(outer && nested, "query.named-error-and-original-cause");
        };
        nested_fault([&] { return StepBody(body, {}, faulty); });
        Same(body, before, "query.failed-body-input-unchanged");
        nested_fault([&] { return RayCast(body.position, {0, -1, 0}, 3, faulty); });
        Same(body, before, "query.failed-ray-input-unchanged");
        BodyState overflow{};
        overflow.is_sensor = true;
        overflow.velocity.x = std::numeric_limits<float>::max();
        PhysicsSettings settings{};
        settings.fixed_step = 2;
        Throws<PhysicsNumericalError>([&] { return StepBody(overflow, {}, EmptyQuery, settings); }, "numerical.integrated-position-overflow");
        overflow.velocity = glm::vec3(0);
        overflow.acceleration.x = std::numeric_limits<float>::max();
        Throws<PhysicsNumericalError>([&] { return StepBody(overflow, {}, EmptyQuery, settings); }, "numerical.velocity-overflow-before-clamp");
        BodyState unrepresentable{};
        unrepresentable.position = {0.5f, 200.5f, 0.5f};
        unrepresentable.velocity.x = std::numeric_limits<float>::max();
        const auto unrepresentable_before = unrepresentable;
        Throws<PhysicsNumericalError>([&] { return StepBody(unrepresentable, {}, faulty); }, "numerical.integrated-scan-before-query");
        Same(unrepresentable, unrepresentable_before, "numerical.scan-failure-input-unchanged");
        Same(StepBody(body, {}, EmptyQuery).body, StepBody(body, {}, EmptyQuery).body, "query.failure-no-global-poison");
    }
}

int main()
{
    const char* active = "test dispatch";
    try {
        using Test = void (*)();
        const std::array<std::pair<const char*, Test>, 8> cases{{
            {"single-step integration and collision regressions", SingleStepRegression},
            {"support, tolerance and offset/chunk coordinates", SupportAndCoordinates},
            {"read-only query and owned results", ReadOnlyQueriesAndOwnedResults},
            {"axis rays, endpoints and simultaneous crossings", RayTraversal},
            {"fixed-step settings and independent calls", FixedStepSettingsAndIsolation}, {"parameter preflight", ParameterValidation},
            {"ray preflight and normal no hit", RayValidationAndNoHit}, {"numerical and nested query faults", NumericalAndQueryFailures}
        }};
        for (const auto& [name, test] : cases) {
            active = name;
            test();
            std::cout << "[PASS] " << name << '\n';
        }
        std::cout << "Independent production physics core: " << cases.size() << " groups passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << active << ": " << error.what() << '\n';
        return 1;
    }
}
