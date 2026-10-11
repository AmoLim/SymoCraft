#include "cpu_baseline_fixture.h"
#include <array>
#include <iostream>
#include <limits>
#include <vector>

namespace {
    using namespace CpuBaseline;

    void Integration(const char* config)
    {
        Fixture fixture(config);
        Physics::RigidBody body{};
        body.velocity = {12, 6, -3};
        body.acceleration = {120, 60, -240};
        body.use_gravity = true;
        const glm::vec3 start(0.5f, 200.5f, 0.5f);
        const auto entity = fixture.AddBody(start, body);
        Physics::Update(fixture.registry, *fixture.world, Step);
        const auto result = fixture.Capture(entity);
        Near(result.transform.position, start + glm::vec3(0.1f, 0.05f, -0.025f), "single-step.position");
        Near(result.body.velocity, {13, 6.333333333f, -5}, "single-step.velocity");
        Near(result.body.acceleration, body.acceleration, "single-step.acceleration");
        Require(!result.body.on_ground, "single-step.ground");
        Near(fixture.Capture(entity).transform.position, result.transform.position, "snapshot.read-only");
    }

    void LimitAndSensor(const char* config)
    {
        Fixture fixture(config);
        fixture.SetBlock({0, 200, 0});
        Physics::RigidBody body{};
        body.velocity = {60, -60, 60};
        body.acceleration = {120, -120, 120};
        body.is_sensor = body.on_ground = body.use_gravity = true;
        const auto entity = fixture.AddBody({0.5f, 200.5f, 0.5f}, body);
        Physics::Update(fixture.registry, *fixture.world, Step);
        const auto result = fixture.Capture(entity);
        Near(result.transform.position, {1, 200, 1}, "sensor.position-before-limit");
        Near(result.body.velocity, {50, -50, 50}, "sensor.velocity-limit");
        Near(result.body.acceleration, body.acceleration, "sensor.acceleration");
        Require(!result.body.on_ground && result.body.is_sensor && result.body.use_gravity, "sensor.flags");
    }

    void Timing(const char* config)
    {
        Fixture fixture(config);
        Physics::RigidBody body{};
        body.velocity = {12, 0, 0};
        const auto entity = fixture.AddBody({0.5f, 200.5f, 0.5f}, body);
        for (float delta : {0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(),
                            std::numeric_limits<float>::infinity()}) {
            Physics::Update(fixture.registry, *fixture.world, delta);
            Near(fixture.Capture(entity).transform.position.x, 0.5f, "legacy-invalid-time.no-step");
        }
        Physics::Update(fixture.registry, *fixture.world, Step / 2);
        Near(fixture.Capture(entity).transform.position.x, 0.5f, "half-step.no-step");
        Physics::Update(fixture.registry, *fixture.world, 0);
        Physics::Update(fixture.registry, *fixture.world, Step / 2);
        Near(fixture.Capture(entity).transform.position.x, 0.6f, "zero-time.retained-remainder");
        Physics::Update(fixture.registry, *fixture.world, Step * 3);
        Near(fixture.Capture(entity).transform.position.x, 0.9f, "three-steps.position");
        Physics::Update(fixture.registry, *fixture.world, 10);
        Near(fixture.Capture(entity).transform.position.x, 1.7f, "long-frame.eight-steps");
        Physics::Update(fixture.registry, *fixture.world, Step);
        Near(fixture.Capture(entity).transform.position.x, 1.8f, "long-frame.no-backlog");
        Physics::Update(fixture.registry, *fixture.world, Step / 2);
        Physics::ResetTiming();
        Physics::Update(fixture.registry, *fixture.world, Step / 2);
        Near(fixture.Capture(entity).transform.position.x, 1.8f, "reset.discarded-remainder");
    }

    void GroundAndRemoval(const char* config)
    {
        Fixture fixture(config);
        fixture.SetBlock({0, 200, 0});
        Physics::RigidBody body{};
        body.velocity = {0, -12, 0};
        body.acceleration = {0, -2, 0};
        body.use_gravity = true;
        const auto entity = fixture.AddBody({0.5f, 201.55f, 0.5f}, body);
        Physics::Update(fixture.registry, *fixture.world, Step);
        auto result = fixture.Capture(entity);
        Near(result.transform.position, {0.5f, 201.5f, 0.5f}, "landing.position");
        Near(result.body.velocity, glm::vec3(0), "landing.velocity");
        Near(result.body.acceleration, glm::vec3(0), "landing.acceleration");
        Require(result.body.on_ground, "landing.ground");
        Physics::Update(fixture.registry, *fixture.world, Step);
        Require(fixture.Capture(entity).body.on_ground, "face-contact.support");
        fixture.RemoveBlock({0, 200, 0});
        Physics::Update(fixture.registry, *fixture.world, Step);
        result = fixture.Capture(entity);
        Require(!result.body.on_ground, "removed-support.ground");
        Near(result.body.velocity.y, -20.0f * Step, "removed-support.gravity");
        Near(result.transform.position.y, 201.5f, "removed-support.position-before-gravity");
    }

    void GroundTolerance(const char* config)
    {
        Fixture fixture(config);
        fixture.SetBlock({0, 200, 0});
        const auto near_ground = fixture.AddBody({0.5f, 201.501f, 0.5f}, {});
        const auto airborne = fixture.AddBody({0.5f, 201.503f, 0.5f}, {});
        Physics::Update(fixture.registry, *fixture.world, Step);
        Require(fixture.Capture(near_ground).body.on_ground, "support.within-0.002");
        Require(!fixture.Capture(airborne).body.on_ground, "support.outside-0.002");
    }

    void WallAndCeiling(const char* config)
    {
        Fixture fixture(config);
        fixture.SetBlock({1, 200, 0});
        fixture.SetBlock({4, 201, 0});
        Physics::RigidBody wall{};
        wall.velocity = {12, 0, 0};
        wall.acceleration = {3, 4, 5};
        const auto wall_entity = fixture.AddBody({0.45f, 200.5f, 0.5f}, wall);
        Physics::RigidBody top{};
        top.velocity = {0, 12, 0};
        top.acceleration = {2, 3, 4};
        const auto top_entity = fixture.AddBody({4.5f, 200.45f, 0.5f}, top);
        Physics::Update(fixture.registry, *fixture.world, Step);
        const auto side = fixture.Capture(wall_entity);
        Near(side.transform.position, {0.5f, 200.5f, 0.5f}, "wall.position");
        Near(side.body.velocity, {0, 4 * Step, 5 * Step}, "wall.velocity");
        Near(side.body.acceleration, {0, 4, 5}, "wall.acceleration");
        Require(!side.body.on_ground, "wall.not-ground");
        const auto ceiling = fixture.Capture(top_entity);
        Near(ceiling.transform.position, {4.5f, 200.5f, 0.5f}, "ceiling.position");
        Near(ceiling.body.velocity, {2 * Step, 0, 4 * Step}, "ceiling.velocity");
        Near(ceiling.body.acceleration, {2, 0, 4}, "ceiling.acceleration");
        Require(!ceiling.body.on_ground, "ceiling.not-ground");
    }

    void CornerTieAndExit(const char* config)
    {
        Fixture fixture(config);
        fixture.SetBlock({0, 200, 0});
        fixture.SetBlock({4, 200, 0});
        Physics::RigidBody corner{};
        corner.velocity = {6, 0, 6};
        const auto tied = fixture.AddBody({-0.25f, 200.5f, -0.25f}, corner);
        Physics::RigidBody exiting{};
        exiting.velocity = {-6, 0, 0};
        exiting.acceleration = {-2, 0, 0};
        const auto moving_out = fixture.AddBody({3.8f, 200.5f, 0.5f}, exiting);
        Physics::Update(fixture.registry, *fixture.world, Step);
        const auto result = fixture.Capture(tied);
        Near(result.transform.position, {-0.2f, 200.5f, -0.5f}, "equal-overlap.tie-z-position");
        Near(result.body.velocity, {6, 0, 0}, "equal-overlap.tie-z-velocity");
        Require(!result.body.on_ground, "equal-overlap.not-ground");
        const auto exit = fixture.Capture(moving_out);
        Near(exit.transform.position, {3.75f, 200.5f, 0.5f}, "moving-out.no-correction");
        Near(exit.body.velocity.x, -6 - 2 * Step, "moving-out.velocity");
        Near(exit.body.acceleration.x, -2, "moving-out.acceleration");
    }

    void OffsetAndCoordinates(const char* config)
    {
        Fixture fixture(config);
        for (int x : {-17, -16, -1, 0, 15, 16}) {
            fixture.SetBlock({x, 200, 3});
            const auto entity = fixture.AddBody({x + 0.25f, 201.25f, 3.5f}, {},
                {glm::vec3(1), {0.25f, 0.25f, 0}});
            fixture.registry.GetComponent<Transform>(entity).scale = glm::vec3(20);
        }
        Physics::Update(fixture.registry, *fixture.world, Step);
        for (const auto entity : fixture.registry.View<Transform, Physics::RigidBody, Physics::HitBox>()) {
            const auto result = fixture.Capture(entity);
            Require(result.body.on_ground, "offset-negative-chunk-boundary.support");
            Near(result.transform.position.y, 201.25f, "offset.position");
            Near(result.transform.scale, glm::vec3(20), "transform-scale.ignored-by-physics");
        }
    }

    void NonminimumTie(const char* config)
    {
        Fixture fixture(config);
        fixture.SetBlock({0, 200, 0});
        const auto entity = fixture.AddBody({1.125f, 201.125f, 0.5f}, {},
            {glm::vec3(0.5f), glm::vec3(0)});
        Physics::Update(fixture.registry, *fixture.world, Step);
        const auto result = fixture.Capture(entity);
        // Equal x/y minima currently fall through to z, even when z is not minimal.
        Near(result.transform.position, {1.125f, 201.125f, -0.25f}, "xy-minimum-tie.falls-through-z");
        Require(!result.body.on_ground, "xy-minimum-tie.not-ground");
    }

    void OffsetCorrection(const char* config)
    {
        Fixture fixture(config);
        fixture.SetBlock({0, 200, 0});
        Physics::RigidBody body{};
        body.velocity = {0, -12, 0};
        const auto entity = fixture.AddBody({0.25f, 201.3f, 0.5f}, body,
            {glm::vec3(1), {0.25f, 0.25f, 0}});
        fixture.registry.GetComponent<Transform>(entity).scale = glm::vec3(20);
        Physics::Update(fixture.registry, *fixture.world, Step);
        const auto result = fixture.Capture(entity);
        Near(result.transform.position, {0.25f, 201.25f, 0.5f}, "offset.correction");
        Near(result.body.velocity, glm::vec3(0), "offset.velocity");
        Near(result.transform.scale, glm::vec3(20), "offset.scale-not-applied");
        Require(result.body.on_ground, "offset.corrected-ground");
    }

    void QueryReadOnly(const char* config)
    {
        Fixture fixture(config);
        fixture.SetBlock({0, 200, 0});
        fixture.world->RebuildDirtyMeshes();
        const auto digest = fixture.world->Digest();
        const auto identity = fixture.world->Identity();
        const auto chunks = fixture.world->ChunkCount();
        struct Publication {
            World::MeshIdentity identity;
            World::Revision revision, input;
            std::size_t size;
            bool operator==(const Publication&) const = default;
        };
        const auto publications = [&] {
            std::vector<Publication> values;
            fixture.world->VisitMeshes([&](const World::WorldMeshRecord& record) {
                values.push_back({record.identity, record.revision, record.mesh_input_revision, record.vertices.size()});
            });
            return values;
        };
        const auto before = publications();
        const auto entity = fixture.AddBody({0.5f, 201.5f, 0.5f}, {});
        Physics::Update(fixture.registry, *fixture.world, Step);
        Require(fixture.Capture(entity).body.on_ground, "read-only.actual-support-query");
        Require(Physics::RayCastStatic(*fixture.world, {0.5f, 202, 0.5f}, {0, -1, 0}, 3).hit, "read-only.actual-ray-query");
        Require(fixture.world->Digest() == digest && fixture.world->Identity() == identity && fixture.world->ChunkCount() == chunks,
                "queries.changed-world-content-or-identity");
        Require(publications() == before, "queries.changed-publications");
        Require(fixture.world->RebuildDirtyMeshes() == 0, "queries.dirtied-meshes");
    }

    void ScanOrder(const char* config)
    {
        struct Case {
            glm::ivec3 first, second;
            glm::vec3 position, size, expected;
            const char* name;
        };
        const std::array<Case, 3> cases{{
            {{0, 201, 1}, {0, 200, 0}, {0.5f, 201, 1}, {0.5f, 1, 0.5f}, {0.5f, 201, 1.25f}, "scan.y-descending"},
            {{0, 200, 0}, {1, 200, 1}, {1, 200.5f, 1}, {1, 0.5f, 0.5f}, {1, 200.5f, 0.75f}, "scan.x-ascending"},
            {{0, 200, 0}, {0, 200, 1}, {0.5f, 200.5f, 1}, {0.5f, 0.5f, 1}, {0.5f, 200.5f, 0.5f}, "scan.z-ascending"}
        }};
        for (const auto& test : cases) {
            Fixture fixture(config);
            fixture.SetBlock(test.first);
            fixture.SetBlock(test.second);
            const auto entity = fixture.AddBody(test.position, {}, {test.size, glm::vec3(0)});
            Physics::Update(fixture.registry, *fixture.world, Step);
            const auto result = fixture.Capture(entity);
            Near(result.transform.position, test.expected, test.name);
            Require(!result.body.on_ground, std::string(test.name) + ".not-ground");
        }
    }

    void OutsideAndNonsolid(const char* config)
    {
        Fixture fixture(config);
        fixture.SetBlock({0, 200, 0}, 9);
        const auto rule = fixture.world->DescribeBlock(9);
        Require(rule && !rule->m_is_solid, "water.fixture-nonsolid");
        Physics::RigidBody body{};
        body.velocity = {0, -6, 0};
        body.use_gravity = body.on_ground = true;
        for (const glm::vec3 start : {glm::vec3(0.5f, 200.5f, 0.5f), glm::vec3(-34.5f, 200.5f, 0.5f),
                                     glm::vec3(50.5f, 200.5f, 0.5f), glm::vec3(0.5f, -2.5f, 0.5f),
                                     glm::vec3(0.5f, 258.5f, 0.5f)}) {
            const auto entity = fixture.AddBody(start, body);
            Physics::ResetTiming();
            const auto previous = fixture.Capture(entity);
            Physics::Update(fixture.registry, *fixture.world, Step);
            const auto result = fixture.Capture(entity);
            Near(result.transform.position, previous.transform.position + body.velocity * Step, "outside-or-water.position");
            Near(result.body.velocity.y, -6 - 20 * Step, "outside-or-water.velocity");
            Require(!result.body.on_ground, "outside-or-water.ground-cleared");
        }
    }

    void RealWorldRays(const char* config)
    {
        Fixture fixture(config);
        for (const auto direction : std::array<glm::ivec3, 6>{glm::ivec3(1, 0, 0), {-1, 0, 0},
                {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}) {
            const glm::ivec3 cell = glm::ivec3(0, 200, 0) + direction * 2;
            fixture.SetBlock(cell);
            const auto hit = Physics::RayCastStatic(*fixture.world, {0.5f, 200.5f, 0.5f}, glm::vec3(direction) * 7.0f, 1.5f);
            Require(hit.hit, "ray.inclusive-endpoint");
            Near(hit.block_center, glm::vec3(cell) + glm::vec3(0.5f), "ray.center");
            Near(hit.hit_normal, -glm::vec3(direction), "ray.normal");
            Near(hit.point, glm::vec3(0.5f, 200.5f, 0.5f) + glm::vec3(direction) * 1.5f, "ray.point");
            Near(hit.block_size, glm::vec3(1), "ray.size");
            Require(!Physics::RayCastStatic(*fixture.world, {0.5f, 200.5f, 0.5f}, glm::vec3(direction), 1.49f).hit, "ray.out-of-range");
            fixture.RemoveBlock(cell);
        }
        fixture.SetBlock({-1, 200, 0});
        const auto boundary = Physics::RayCastStatic(*fixture.world, {0, 200.5f, 0.5f}, {-1, 0, 0}, 0);
        Require(boundary.hit, "ray.negative-integer-boundary");
        Near(boundary.point, {0, 200.5f, 0.5f}, "ray.zero-distance");
        Near(boundary.hit_normal, {1, 0, 0}, "ray.negative-boundary-normal");
        const auto inside = Physics::RayCastStatic(*fixture.world, {-0.5f, 200.5f, 0.5f}, {1, 0, 0}, 0);
        Require(inside.hit, "ray.start-inside");
        Near(inside.hit_normal, glm::vec3(0), "ray.start-inside-normal");
        Require(!Physics::RayCastStatic(*fixture.world, {-34, 200.5f, 0.5f}, {-1, 0, 0}, 3).hit, "ray.outside-world");
        Require(!Physics::RayCastStatic(*fixture.world, {0.5f, 200.5f, 0.5f}, glm::vec3(0), 3).hit, "legacy-ray.zero-direction-no-hit");
    }

    void RegistryTraversal(const char* config)
    {
        Fixture fixture(config);
        Physics::RigidBody body{};
        body.velocity = {12, 0, 0};
        const auto first = fixture.AddBody({0.5f, 200.5f, 0.5f}, body);
        const auto second = fixture.AddBody({2.5f, 200.5f, 0.5f}, body);
        const auto incomplete = fixture.registry.CreateEntity();
        fixture.registry.AddComponent<Transform>(incomplete) = {};
        fixture.registry.GetComponent<Transform>(incomplete).position = {4.5f, 200.5f, 0.5f};
        fixture.registry.AddComponent<Physics::RigidBody>(incomplete) = body;
        Physics::Update(fixture.registry, *fixture.world, Step * 2);
        Near(fixture.Capture(first).transform.position.x, 0.7f, "registry.first-two-steps");
        Near(fixture.Capture(second).transform.position.x, 2.7f, "registry.second-two-steps");
        Near(fixture.registry.GetComponent<Transform>(incomplete).position.x, 4.5f, "registry.missing-hitbox-skipped");
    }
}

int main(int argc, char** argv)
{
    const char* active = "arguments";
    try {
        CpuBaseline::Require(argc == 2, "Expected block configuration path");
        using Test = void (*)(const char*);
        const std::array<std::pair<const char*, Test>, 15> cases{{
            {"single-step integration order", Integration}, {"limit and sensor", LimitAndSensor},
            {"zero/multiple/long steps and reset", Timing}, {"landing and support removal", GroundAndRemoval},
            {"ground tolerance", GroundTolerance}, {"wall and low ceiling", WallAndCeiling},
            {"corner tie and moving out", CornerTieAndExit}, {"offset and negative/chunk coordinates", OffsetAndCoordinates},
            {"nonminimum tie fallthrough", NonminimumTie}, {"multi-block scan order", ScanOrder},
            {"offset penetration correction", OffsetCorrection}, {"world queries are read-only", QueryReadOnly},
            {"outside world and nonsolid", OutsideAndNonsolid}, {"real world rays", RealWorldRays},
            {"registry traversal", RegistryTraversal}
        }};
        for (const auto& [name, test] : cases) {
            active = name;
            test(argv[1]);
            std::cout << "[PASS] " << name << '\n';
        }
        std::cout << "Legacy production physics baseline: " << cases.size() << " cases passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << active << ": " << error.what() << '\n';
        return 1;
    }
}
