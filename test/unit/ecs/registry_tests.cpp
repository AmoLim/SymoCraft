#include "symocraft/ecs/registry.h"
#include <symocraft/foundation/memory.h>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {
    struct Position { float x, y, z; };
    struct Velocity { float x, y, z; };
    struct Unregistered { int value; };
    using namespace SymoCraft::ECS;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }

    template<class Error, class Call>
    void Throws(Call&& call, std::string_view operation) {
        try { call(); }
        catch (const Error& error) {
            Require(std::string_view(error.what()).find(operation) != std::string_view::npos,
                "Failure did not identify its ECS operation");
            return;
        }
        throw std::runtime_error("Expected ECS failure was not reported");
    }

    void RegisterRequired(Registry& registry) {
        registry.RegisterComponent<Position>("Position");
        registry.RegisterComponent<Velocity>("Velocity");
    }

    void NormalFacade() {
        Registry registry;
        RegisterRequired(registry);
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
        RegisterRequired(second);
        const auto other = second.CreateEntity();
        second.AddComponent<Position>(other) = {7, 8, 9};
        Require(second.GetComponent<Position>(other).x == 7, "Separate registry storage collided");
    }

    void RegistrationAndMissingComponents() {
        Registry registry;
        Throws<std::logic_error>([&] { registry.RegisterComponent<Velocity>("Velocity"); }, "RegisterComponent");
        Throws<std::invalid_argument>([&] { registry.RegisterComponent<Position>(nullptr); }, "RegisterComponent");
        registry.RegisterComponent<Position>("Position");
        const Registry& read = registry;
        Require(read.IsComponentRegistered<Position>() && !read.IsComponentRegistered<Velocity>(), "Registration lookup changed");
        Throws<std::logic_error>([&] { registry.RegisterComponent<Position>("duplicate"); }, "RegisterComponent");
        const auto entity = registry.CreateEntity();
        Require(!registry.HasComponent<Position>(entity), "Missing component was invented");
        Throws<std::runtime_error>([&] { registry.GetComponent<Position>(entity); }, "GetComponent");
        Throws<std::runtime_error>([&] { registry.AddComponent<Velocity>(entity); }, "AddComponent");
        Throws<std::logic_error>([&] { registry.View<Velocity>(); }, "View");
        Require(registry.NumComponents(entity) == 0 && !read.IsComponentRegistered<Velocity>(), "Rejected call changed storage");
        registry.RegisterComponent<Velocity>("Velocity");
        registry.AddComponent<Position>(entity) = {1, 2, 3};
        registry.AddComponent<Velocity>(entity) = {4, 5, 6};
        Require(!read.IsComponentRegistered<Unregistered>(), "Lookup implicitly registered a component");
        Throws<std::runtime_error>([&] { registry.AddComponent<Unregistered>(entity); }, "AddComponent");
        Require(registry.NumComponents(entity) == 2 && read.GetComponent<Position>(entity).z == 3, "Valid state was damaged");
        for (int type : {-1, MaxComponents}) {
            Require(!registry.HasComponentByType(entity, type), "Invalid type passed Has");
            Require(registry.GetComponentByType(entity, type) == nullptr, "Invalid type passed Get");
            Require(registry.AddOrGetComponentByType(entity, type) == nullptr, "Invalid type passed Add");
        }
    }

    void GenerationsAndViews() {
        static_assert(std::is_same_v<decltype(*std::declval<Iterator&>()), EntityId>);
        Registry registry;
        RegisterRequired(registry);
        const auto old = registry.CreateEntity();
        registry.AddComponent<Position>(old) = {1, 2, 3};
        registry.DestroyEntity(old); // One tail component; general removal stays a T5 concern.
        Require(!registry.IsEntityValid(old) && !registry.HasComponent<Position>(old), "Destroyed ID remained live");
        Throws<std::invalid_argument>([&] { registry.DestroyEntity(old); }, "DestroyEntity");
        const auto current = registry.CreateEntity();
        Require(current != old && static_cast<uint32>(current) == static_cast<uint32>(old) && (current >> 32) == 1,
            "Reused slot lost its new generation");
        registry.AddComponent<Position>(current) = {7, 8, 9};
        const auto empty = registry.CreateEntity();
        registry.DestroyEntity(empty);
        const auto second = registry.CreateEntity();
        Require(second != current && (second >> 32) == 1, "Duplicate destroy corrupted the free list");
        registry.AddComponent<Position>(second) = {10, 11, 12};
        registry.AddComponent<Velocity>(second) = {4, 5, 6};
        const auto hole = registry.CreateEntity();
        registry.DestroyEntity(hole);
        for (auto invalid : std::array<EntityId, 4>{old, null_entity, EntityId(UINT32_MAX - 1), current + (EntityId(1) << 32)}) {
            Require(!registry.IsEntityValid(invalid) && !registry.HasComponent<Position>(invalid), "Invalid ID passed validation");
            Require(registry.GetComponentByType(invalid, 0) == nullptr && registry.AddOrGetComponentByType(invalid, 0) == nullptr,
                "Invalid ID reached component storage");
            Throws<std::runtime_error>([&] { registry.GetComponent<Position>(invalid); }, "GetComponent");
            Throws<std::runtime_error>([&] { registry.AddComponent<Position>(invalid); }, "AddComponent");
            Throws<std::invalid_argument>([&] { registry.DestroyEntity(invalid); }, "DestroyEntity");
        }
        Require(registry.GetComponent<Position>(current).z == 9 && registry.GetComponent<Position>(second).z == 12,
            "Invalid ID changed a live entity");
        int count = 0;
        for (auto id : registry.View<Position>()) {
            Require((id == current || id == second) && registry.IsEntityValid(id), "Iterator truncated generation");
            ++count;
        }
        Require(count == 2, "Filtered begin/increment lost reused entities");
        count = 0;
        for (auto id : registry.View<Position, Velocity>()) {
            Require(id == second, "Filter inspected only the entity index");
            ++count;
        }
        Require(count == 1, "Filtered begin missed a nonzero generation");
        count = 0;
        for (auto id : registry.View<>()) {
            Require(registry.IsEntityValid(id), "Unfiltered view returned a tombstone");
            ++count;
        }
        Require(count == 2, "Unfiltered view did not skip the dead slot");
    }

    void CleanupAndExplicitReset() {
        Registry registry;
        registry.RegisterComponent<Position>("Position");
        const auto entity = registry.CreateEntity();
        registry.AddComponent<Position>(entity) = {1, 2, 3};
        Throws<std::runtime_error>([&] { registry.AddComponent<Velocity>(entity); }, "AddComponent");
        registry.Free();
        registry.Free();
        Require(!registry.IsEntityValid(entity) && !registry.IsComponentRegistered<Position>(), "Freed storage remained accessible");
        Throws<std::runtime_error>([&] { registry.AddComponent<Position>(entity); }, "AddComponent");
        Throws<std::runtime_error>([&] { registry.GetComponent<Position>(entity); }, "GetComponent");
        Throws<std::logic_error>([&] { registry.CreateEntity(); }, "CreateEntity");
        Throws<std::logic_error>([&] { registry.RegisterComponent<Velocity>("Velocity"); }, "RegisterComponent");
        Throws<std::invalid_argument>([&] { registry.DestroyEntity(entity); }, "DestroyEntity");
        int count = 0;
        for (auto id : registry.View<>()) {
            static_cast<void>(id);
            ++count;
        }
        Require(count == 0, "Freed Registry was iterable");
        registry.Clear();
        RegisterRequired(registry);
        const auto fresh = registry.CreateEntity();
        const auto& value = registry.AddComponent<Position>(fresh);
        Require(registry.IsEntityValid(fresh) && value.x == 0 && value.y == 0 && value.z == 0, "Explicit Clear/reset failed");
    }
}

int main(int argc, char** argv) {
    try {
        const bool tracked = argc == 2 && std::string_view(argv[1]) == "--tracked";
        Require(argc == 1 || tracked, "Unexpected registry test argument");
        AmoBase::AmoMemory_Init(tracked, 1024);
        NormalFacade();
        RegistrationAndMissingComponents();
        GenerationsAndViews();
        CleanupAndExplicitReset();
        std::cout << "Registry: facade, preflight, generations/views and cleanup passed; tracking=" << tracked << '\n';
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
