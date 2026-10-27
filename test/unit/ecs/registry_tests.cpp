#include "symocraft/ecs/registry.h"
#include <symocraft/foundation/memory.h>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

using namespace SymoCraft::ECS;
namespace {
    struct Position { int x{}, y{}, z{}; };
    struct Velocity { int x{}, y{}, z{}; };
    void Require(bool condition) { if (!condition) throw std::runtime_error("public Registry regression"); }
}
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--tracked") AmoBase::AmoMemory_Init(true);
        Registry registry, other;
        registry.RegisterComponent<Velocity>("velocity first");
        registry.RegisterComponent<Position>("position second");
        registry.RegisterComponent<Position>("idempotent");
        other.RegisterComponent<Position>("independent first");
        std::array<EntityId, 3> entities;
        for (std::size_t i = 0; i < entities.size(); ++i) {
            entities[i] = registry.CreateEntity();
            registry.Emplace<Position>(entities[i], static_cast<int>(i), 2, 3);
        }
        registry.Emplace<Velocity>(entities[1], 4, 5, 6);
        registry.RemoveComponent<Position>(entities[0]);
        Require(registry.IsEntityValid(entities[0]) && registry.NumComponents(entities[0]) == 0);
        Require(registry.GetComponent<Position>(entities[2]).x == 2);
        std::unordered_set<EntityId> matching;
        registry.Each<Position, const Velocity>([&](EntityId id, Position& position, const Velocity& velocity) {
            matching.insert(id); position.x += velocity.x;
        });
        Require(matching.size() == 1 && matching.contains(entities[1]));
        registry.DestroyEntity(entities[1]);
        const auto replacement = registry.CreateEntity();
        Require(replacement != entities[1] && !registry.IsEntityValid(entities[1]));
        Require(!registry.IsEntityValid(other.CreateEntity()));
        registry.Clear();
        Require(registry.IsComponentRegistered<Position>() && !registry.IsEntityValid(entities[2]));
        registry.GetOrAdd<Position>(registry.CreateEntity()).x = 10;
        registry.Clear(); registry.Clear();
        std::puts("PASS public Registry registration, objects, identity, query and reusable Clear");
        return 0;
    } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
