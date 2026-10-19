#include <symocraft/physics/physics.h>

namespace {
    bool Solid(const void*, const glm::ivec3& cell)
    {
        return cell == glm::ivec3(0, 0, 0);
    }
}

int main()
{
    using namespace SymoCraft::Physics;
    const TerrainCollisionQuery query{nullptr, Solid};
    BodyState body{};
    body.position = {0.5f, 1.5f, 0.5f};
    const auto step = StepBody(body, {}, query);
    const auto ray = RayCast({0.5f, 2, 0.5f}, {0, -1, 0}, 3, query);
    return step.body.on_ground && ray.hit && ray.cell == glm::ivec3(0) &&
        ray.normal == glm::vec3(0, 1, 0) && ray.distance == 1 ? 0 : 1;
}
