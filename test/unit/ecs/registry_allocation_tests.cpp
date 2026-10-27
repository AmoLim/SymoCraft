#include "contract_components.h"
#include <symocraft/foundation/memory.h>
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <new>
#include <string_view>
#include <vector>

namespace {
    bool armed{};
    int ordinal{}, allocations{}, hits{};
    std::size_t failed_bytes{};
    bool failed_aligned{};
    void BeforeAllocate(std::size_t bytes, bool aligned) {
        if (armed && allocations++ == ordinal) {
            ++hits; failed_bytes = bytes; failed_aligned = aligned;
            throw std::bad_alloc();
        }
    }
    void Arm(int index) { ordinal = index; allocations = hits = 0; failed_bytes = 0; armed = true; }
    void Require(bool condition) { if (!condition) throw std::runtime_error("allocation contract assertion"); }
}
void* operator new(std::size_t bytes) {
    BeforeAllocate(bytes, false);
    if (void* pointer = std::malloc(bytes ? bytes : 1)) return pointer;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }
void* operator new(std::size_t bytes, std::align_val_t alignment) {
    BeforeAllocate(bytes, true);
    if (void* pointer = _aligned_malloc(bytes ? bytes : 1, static_cast<std::size_t>(alignment))) return pointer;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes, std::align_val_t alignment) { return ::operator new(bytes, alignment); }
void operator delete(void* pointer, std::align_val_t) noexcept { _aligned_free(pointer); }
void operator delete[](void* pointer, std::align_val_t) noexcept { _aligned_free(pointer); }
void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept { _aligned_free(pointer); }
void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept { _aligned_free(pointer); }

using namespace SymoCraft::ECS;
int main(int argc, char** argv) {
    try {
        Require(argc == 1 || (argc == 2 && std::string_view(argv[1]) == "--tracked") || argc == 3);
        if (argc == 2) AmoBase::AmoMemory_Init(true);
        const std::string_view scenario = argc == 3 ? argv[1] : "first-create";
        const int index = argc == 3 ? std::atoi(argv[2]) : 0;
        bool caught = false;
        if (scenario == "owner") {
            Arm(index);
            try { Registry pending; } catch (const std::bad_alloc&) { caught = true; }
            armed = false;
            Require(caught && hits == 1);
            Registry retry;
            Require(retry.IsEntityValid(retry.CreateEntity()));
        } else if (scenario == "first-create") {
            Registry owner;
            EntityId unpublished;
            Arm(index);
            try { unpublished = owner.CreateEntity(); } catch (const std::bad_alloc&) { caught = true; }
            armed = false;
            Require(caught && hits == 1 && unpublished == null_entity && owner.GetStatus().health == RegistryHealth::Healthy);
            Require(owner.IsEntityValid(owner.CreateEntity()));
        } else if (scenario == "resource-destroy" || scenario == "cleanup") {
            {
                Registry owner;
                owner.RegisterComponent<ContractResource>("resource");
                const auto first = owner.CreateEntity(), second = owner.CreateEntity();
                for (int i = 2; i < 9001; ++i) (void)owner.CreateEntity();
                const auto high = owner.CreateEntity();
                owner.Emplace<ContractResource>(first, 1);
                owner.Emplace<ContractResource>(second, 2);
                owner.Emplace<ContractResource>(high, 3);
                if (scenario == "resource-destroy") {
                    Arm(index);
                    owner.DestroyEntity(second); owner.DestroyEntity(first);
                    armed = false;
                    Require(allocations == 0 && hits == 0 && ContractResource::live == 1);
                    Require(*owner.GetComponent<ContractResource>(high).value == 3);
                } else Arm(index);
            }
            armed = false;
            Require(allocations == 0 && hits == 0 && ContractResource::live == 0);
        } else {
            Registry owner;
            owner.RegisterComponent<ContractValue>("value");
            const auto old = owner.CreateEntity();
            owner.Emplace<ContractValue>(old, 77);
            const std::string name(96, 'r');
            EntityId pending{};
            if (scenario == "resource" || scenario == "aligned") owner.RegisterComponent<ContractResource>("resource");
            if (scenario == "growth") {
                for (int i = 1; i < 1024; ++i) owner.Emplace<ContractValue>(owner.CreateEntity(), i);
                pending = owner.CreateEntity();
            } else if (scenario == "resource" || scenario == "aligned") {
                pending = owner.CreateEntity();
                if (scenario == "resource") owner.Emplace<ContractResource>(old, 10);
            }
            Arm(index);
            try {
                if (scenario == "registration") owner.RegisterComponent<ContractResource>(name);
                else if (scenario == "create") pending = owner.CreateEntity();
                else if (scenario == "growth") owner.Emplace<ContractValue>(pending, 1000);
                else if (scenario == "resource" || scenario == "aligned") owner.Emplace<ContractResource>(pending, 1000);
                else if (scenario == "ordered" || scenario == "nested-ordered") {
                    auto ordered = [&] { owner.EachOrdered<ContractValue>(
                    [](EntityId, const ContractValue& v) { return v.value; },
                    [](EntityId, ContractValue&) { throw std::runtime_error("unexpected business visitor"); }); };
                    if (scenario == "ordered") ordered();
                    else owner.Each<>([&](EntityId) {
                        try { ordered(); } catch (const std::bad_alloc&) { caught = true; }
                        try { owner.Clear(); Require(false); }
                        catch (const RegistryError& error) { Require(error.Code() == RegistryErrorCode::InvalidContext); }
                    });
                }
                else if (scenario == "clear") owner.Clear();
                else if (scenario == "destroy") owner.DestroyEntity(old);
                else throw std::runtime_error("unknown fault scenario");
            } catch (const std::bad_alloc&) { caught = true; }
            armed = false;
            if (scenario == "clear" || scenario == "destroy") {
                Require(!caught && allocations == 0 && hits == 0 && !owner.IsEntityValid(old));
                if (scenario == "clear") Require(owner.IsComponentRegistered<ContractValue>());
            } else {
                Require(caught && hits == 1 && owner.GetStatus().health == RegistryHealth::Healthy);
                Require(owner.IsEntityValid(old) && owner.GetComponent<ContractValue>(old).value == 77);
                if (scenario == "registration") {
                    Require(!owner.IsComponentRegistered<ContractResource>());
                    owner.RegisterComponent<ContractResource>(name);
                } else if (scenario == "create") {
                    Require(pending == null_entity);
                    Require(owner.IsEntityValid(owner.CreateEntity()));
                } else if (scenario == "growth") {
                    Require(!owner.HasComponent<ContractValue>(pending));
                    owner.Emplace<ContractValue>(pending, 1000);
                } else if (scenario == "resource" || scenario == "aligned") {
                    Require(!owner.HasComponent<ContractResource>(pending));
                    Require(ContractResource::live == (scenario == "resource" ? 1 : 0));
                    if (scenario == "resource") Require(*owner.GetComponent<ContractResource>(old).value == 10);
                    owner.Emplace<ContractResource>(pending, 1000);
                } else if (scenario == "ordered" || scenario == "nested-ordered") {
                    unsigned seen = 0;
                    owner.Each<ContractValue>([&](EntityId, ContractValue&) { ++seen; });
                    Require(seen == 1);
                    owner.RegisterComponent<ContractEmpty>("guard released");
                }
            }
        }
        Require(ContractResource::live == 0);
        std::printf("PASS %.*s #%d allocations=%d hits=%d failed-bytes=%zu aligned=%d caught=%d\n",
                    static_cast<int>(scenario.size()), scenario.data(), index,
                    allocations, hits, failed_bytes, failed_aligned, caught);
        return 0;
    } catch (const std::exception& error) {
        armed = false;
        std::fprintf(stderr, "%s allocations=%d hits=%d\n", error.what(), allocations, hits);
        return 1;
    }
}
