#include "allocation_probe.h"
#include <symocraft/ecs/registry.h>
#include <symocraft/foundation/memory.h>
#include <array>
#include <cstdlib>
#include <iostream>
#include <new>
#include <exception>
#include <cstdio>
#include <stdexcept>
#include <string_view>

namespace {
    thread_local std::ptrdiff_t fail_new_after = -1;
    std::size_t live_new_blocks = 0;
    std::size_t new_attempts = 0;
    void* New(std::size_t bytes) {
        ++new_attempts;
        if (fail_new_after == 0) throw std::bad_alloc();
        if (fail_new_after > 0) --fail_new_after;
        auto* memory = std::malloc(bytes ? bytes : 1);
        if (!memory) throw std::bad_alloc();
        ++live_new_blocks;
        return memory;
    }
    void Delete(void* memory) noexcept {
        if (memory) { --live_new_blocks; std::free(memory); }
    }
    using namespace SymoCraft::ECS;
    using namespace AmoBase::Detail;
    struct Position { std::uint64_t x, y, z; };
    struct Velocity { std::uint64_t x, y, z; };
    constexpr const char* long_name = "Position name deliberately exceeds the small string optimization capacity";
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    template<class Call> void BadAlloc(Call&& call) {
        bool failed = false;
        try { call(); } catch (const std::bad_alloc&) { failed = true; }
        Require(failed, "Allocation failure was not propagated");
    }
    struct Failure {
        AllocationStage stage;
        int remaining;
        bool hit = false;
        Failure(AllocationStage value, int ordinal = 0) : stage(value), remaining(ordinal) {
            SetAllocationFailureProbe([](AllocationStage current, void* context) noexcept {
                auto& self = *static_cast<Failure*>(context);
                if (current != self.stage || self.remaining-- > 0) return false;
                self.hit = true;
                return true;
            }, this);
        }
        ~Failure() { SetAllocationFailureProbe(nullptr, nullptr); }
    };
    void SameAllocations(const AllocationSnapshot& before) {
        const auto after = CaptureAllocations();
        Require(before.active_blocks == after.active_blocks && before.payload_bytes == after.payload_bytes &&
            after.sentinels_intact, "ECS failure leaked/damaged tracked storage");
    }
    void Initialization(bool tracked) {
        for (auto stage : {AllocationStage::Allocate, AllocationStage::TrackingRecord}) {
            if (!tracked && stage == AllocationStage::TrackingRecord) continue;
            for (int point = 0; point < 3; ++point) {
                const auto before = CaptureAllocations();
                {
                    Registry registry;
                    {
                        Failure fault(stage, point);
                        BadAlloc([&] { registry.RegisterComponent<Position>(long_name); });
                        Require(fault.hit, "Container initialization failure point not reached");
                    }
                    Require(!registry.IsComponentRegistered<Position>(), "Partially initialized type was published");
                    SameAllocations(before);
                    registry.RegisterComponent<Position>(long_name);
                    const auto entity = registry.CreateEntity();
                    registry.AddComponent<Position>(entity) = {1, 2, 3};
                    Require(registry.GetComponent<Position>(entity).z == 3, "Registration retry lost data");
                    {
                        Failure cleanup(stage);
                        fail_new_after = 0;
                        registry.Free();
                        registry.Free();
                        fail_new_after = -1;
                        Require(!cleanup.hit, "Cleanup tried to allocate");
                    }
                    SameAllocations(before);
                }
                SameAllocations(before);
            }
        }
    }
    void RegistrationTransfer(bool tracked) {
        // Sweep actual string/vector allocations in RegisterType, not a simulated handoff error.
        for (std::ptrdiff_t point = 0; point < 64; ++point) {
            const auto before = CaptureAllocations();
            const auto new_before = live_new_blocks;
            bool failed = false;
            std::size_t attempts;
            {
                Registry registry;
                registry.RegisterComponent<Position>(long_name);
                const auto entity = registry.CreateEntity();
                registry.AddComponent<Position>(entity) = {11, 22, 33};
                const auto owned = CaptureAllocations();
                new_attempts = 0;
                fail_new_after = point;
                try { registry.RegisterComponent<Velocity>(long_name); }
                catch (const std::bad_alloc&) { failed = true; }
                fail_new_after = -1;
                attempts = new_attempts;
                Require(registry.IsComponentRegistered<Velocity>() == !failed, "Registration published half a type");
                Require(registry.GetComponent<Position>(entity).z == 33, "Registration move damaged prior storage");
                if (failed) {
                    SameAllocations(owned);
                    registry.RegisterComponent<Velocity>(long_name);
                }
                registry.AddComponent<Velocity>(entity) = {44, 55, 66};
                Require(registry.GetComponent<Velocity>(entity).z == 66, "Registration retry failed after transfer error");
            }
            SameAllocations(before);
            if (!tracked) Require(live_new_blocks == new_before, "Registration leaked C++ owner allocations");
            if (!failed) {
                Require(point >= 3 && attempts >= 3, "Did not exercise string and both vector allocations");
                return;
            }
        }
        throw std::runtime_error("Registration allocation sweep did not reach success");
    }
    void Growth(bool tracked) {
        for (auto stage : {AllocationStage::Reallocate, AllocationStage::SentinelCopy,
                           AllocationStage::Allocate, AllocationStage::TrackingRecord}) {
            if (!tracked && (stage == AllocationStage::SentinelCopy || stage == AllocationStage::TrackingRecord)) continue;
            const int points = (stage == AllocationStage::Allocate || stage == AllocationStage::TrackingRecord) ? 2 : 1;
            for (int point = 0; point < points; ++point) for (bool retry : {false, true}) {
                const auto before = CaptureAllocations();
                {
                    Registry registry;
                    registry.RegisterComponent<Position>(long_name);
                    std::array<EntityId, 9> entities;
                    for (auto& entity : entities) entity = registry.CreateEntity();
                    for (std::size_t i = 0; i < 8; ++i) registry.AddComponent<Position>(entities[i]) = {i, i + 10, i + 20};
                    {
                        Failure fault(stage, point);
                        BadAlloc([&] { registry.AddComponent<Position>(entities[8]); });
                        Require(fault.hit, "Sparse/dense growth failure point not reached");
                    }
                    Require(!registry.HasComponent<Position>(entities[8]) && registry.NumComponents(entities[8]) == 0,
                        "Growth failure published a phantom component");
                    std::size_t count = 0;
                    for (auto entity : registry.View<Position>()) {
                        const auto& value = registry.GetComponent<Position>(entity);
                        Require(value.y == value.x + 10 && value.z == value.x + 20, "Growth lost old payload bytes");
                        ++count;
                    }
                    Require(count == 8, "Growth changed existing sparse mappings");
                    if (retry) {
                        registry.AddComponent<Position>(entities[8]) = {8, 18, 28};
                        Require(registry.GetComponent<Position>(entities[8]).z == 28, "Growth retry failed");
                    }
                    {
                        Failure cleanup(stage);
                        fail_new_after = 0;
                        registry.Free();
                        registry.Free();
                        fail_new_after = -1;
                        Require(!cleanup.hit, "Failed growth cleanup tried to allocate");
                    }
                    SameAllocations(before);
                    registry.Clear();
                    registry.RegisterComponent<Position>(long_name);
                    registry.AddComponent<Position>(registry.CreateEntity()) = {1, 2, 3};
                }
                SameAllocations(before);
            }
        }
    }
}

void* operator new(std::size_t bytes) { return New(bytes); }
void* operator new[](std::size_t bytes) { return New(bytes); }
void operator delete(void* memory) noexcept { Delete(memory); }
void operator delete[](void* memory) noexcept { Delete(memory); }
void operator delete(void* memory, std::size_t) noexcept { Delete(memory); }
void operator delete[](void* memory, std::size_t) noexcept { Delete(memory); }

int main(int argc, char** argv) {
    try {
        std::set_terminate([] { std::fputs("Unexpected terminate during allocation regression\n", stderr); std::_Exit(134); });
        const bool tracked = argc == 2 && std::string_view(argv[1]) == "--tracked";
        Require(argc == 1 || tracked, "Unexpected ECS allocation argument");
        AmoBase::AmoMemory_Init(tracked, 1024);
        Initialization(tracked);
        RegistrationTransfer(tracked);
        Growth(tracked);
        Require(CaptureAllocations().active_blocks == 0, "ECS allocation regression leaked tracked blocks");
        std::cout << "ECS allocation: initialization, registration transfer, sparse/dense failure cleanup passed; tracking=" << tracked << '\n';
        return 0;
    } catch (const std::exception& error) {
        fail_new_after = -1;
        std::cerr << error.what() << '\n';
        return 1;
    }
}
