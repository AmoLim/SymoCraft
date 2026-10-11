#include "symocraft/ecs/registry.h"
#include <iostream>
#include <stdexcept>

namespace {
    struct Position { float x, y, z; };
    struct Velocity { float x, y, z; };
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
}
int main() {
    using namespace SymoCraft::ECS;
    try {
        Registry registry;
        registry.RegisterComponent<Position>("Position");
        registry.RegisterComponent<Velocity>("Velocity");
        const auto entity = registry.CreateEntity();
        Require(registry.IsEntityValid(entity), "Entity was not created");
        auto& position = registry.AddComponent<Position>(entity);
        Require(position.x == 0 && position.y == 0 && position.z == 0, "New component not zero initialized");
        position = {1, 2, 3};
        registry.AddComponent<Velocity>(entity) = {4, 5, 6};
        Require(registry.NumComponents(entity) == 2, "Component count changed");
        Require(registry.GetComponent<Position>(entity).z == 3, "Component data lost");
        int count = 0;
        for (const auto id : registry.View<Position, Velocity>()) {
            Require(id == entity, "View returned different entity");
            ++count;
        }
        Require(count == 1, "View did not find the player-shaped entity");
        registry.Free();
        registry.Free();
        Registry second;
        second.RegisterComponent<Position>("Position");
        second.RegisterComponent<Velocity>("Velocity");
        const auto other = second.CreateEntity();
        second.AddComponent<Position>(other) = {7, 8, 9};
        Require(second.GetComponent<Position>(other).x == 7, "Separate registry storage collided");
        std::cout << "Registry public typed facade and private storage lifecycle passed.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
