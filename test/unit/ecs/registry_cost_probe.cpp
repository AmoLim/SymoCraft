#include "symocraft/ecs/registry.h"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <new>

namespace {
    bool counting{};
    std::size_t requests{}, requested_bytes{};
    void Count(std::size_t bytes) { if (counting) { ++requests; requested_bytes += bytes; } }
    struct Value { std::uint32_t key; std::int64_t value; };
    std::int64_t checksum{};
    template<class Fn> void Measure(unsigned repeat, const char* phase, Fn action) {
        requests = requested_bytes = 0;
        const auto start = std::chrono::steady_clock::now();
        counting = true;
        action();
        counting = false;
        const auto us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
        std::printf("repeat=%u phase=%s us=%.3f allocation_requests=%zu requested_bytes=%zu\n",
                    repeat, phase, us, requests, requested_bytes);
    }
}
void* operator new(std::size_t bytes) {
    Count(bytes);
    if (void* pointer = std::malloc(bytes ? bytes : 1)) return pointer;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }
void* operator new(std::size_t bytes, std::align_val_t alignment) {
    Count(bytes);
    if (void* pointer = _aligned_malloc(bytes ? bytes : 1, static_cast<std::size_t>(alignment))) return pointer;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes, std::align_val_t alignment) { return ::operator new(bytes, alignment); }
void operator delete(void* pointer, std::align_val_t) noexcept { _aligned_free(pointer); }
void operator delete[](void* pointer, std::align_val_t) noexcept { _aligned_free(pointer); }
void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept { _aligned_free(pointer); }
void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept { _aligned_free(pointer); }

int main() {
    using namespace SymoCraft::ECS;
    std::puts("Production Registry bounded observation: entities=1025 rounds=16 repeats=3; owner construction excluded; peak/retained bytes not measured");
    for (unsigned repeat = 0; repeat < 3; ++repeat) {
        Registry owner;
        std::array<EntityId, 1025> ids;
        Measure(repeat, "register", [&] { owner.RegisterComponent<Value>("cost value"); });
        Measure(repeat, "create-attach-growth", [&] {
            for (std::uint32_t i = 0; i < ids.size(); ++i) {
                ids[i] = owner.CreateEntity();
                owner.Emplace<Value>(ids[i], i, std::int64_t{1});
            }
        });
        Measure(repeat, "each-16", [&] {
            for (unsigned round = 0; round < 16; ++round)
                owner.Each<Value>([](EntityId, Value& value) { checksum += ++value.value; });
        });
        Measure(repeat, "ordered-16", [&] {
            for (unsigned round = 0; round < 16; ++round)
                owner.EachOrdered<Value>([](EntityId, const Value& value) { return value.key; },
                                        [](EntityId, Value& value) { checksum += ++value.value; });
        });
        Measure(repeat, "remove-readd-half", [&] {
            for (std::uint32_t i = 0; i < ids.size(); i += 2) {
                owner.RemoveComponent<Value>(ids[i]);
                owner.Emplace<Value>(ids[i], i, std::int64_t{1});
            }
        });
        Measure(repeat, "clear", [&] { owner.Clear(); });
    }
    std::printf("checksum=%lld\n", static_cast<long long>(checksum));
}
