#include "allocation_probe.h"
#include <symocraft/foundation/memory.h>
#include <crtdbg.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <string_view>

namespace {
    thread_local bool fail_new = false;
    using namespace AmoBase::Detail;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    struct Failure {
        AllocationStage stage;
        bool hit = false;
        explicit Failure(AllocationStage value) : stage(value) {
            SetAllocationFailureProbe([](AllocationStage current, void* context) noexcept {
                auto& self = *static_cast<Failure*>(context);
                if (current != self.stage) return false;
                self.hit = true;
                return true;
            }, this);
        }
        ~Failure() { SetAllocationFailureProbe(nullptr, nullptr); }
    };
    void SameAllocations(const AllocationSnapshot& before) {
        const auto after = CaptureAllocations();
        Require(before.active_blocks == after.active_blocks && before.payload_bytes == after.payload_bytes &&
            before.sentinels_intact == after.sentinels_intact, "Allocation failure changed live tracking");
    }
    void RealTrackingGrowthFailure() {
        // First tracked allocation in this process must grow the production record vector.
        _CrtMemState start{}, finish{};
        _CrtMemCheckpoint(&start);
        fail_new = true;
        auto* result = AmoMemory_Allocate(64);
        fail_new = false;
        _CrtMemCheckpoint(&finish);
        Require(result == nullptr && CaptureAllocations().active_blocks == 0,
            "Throwing production tracking-vector growth did not return nullptr");
#ifdef _DEBUG
        Require(start.lCounts[_NORMAL_BLOCK] == finish.lCounts[_NORMAL_BLOCK] &&
            start.lSizes[_NORMAL_BLOCK] == finish.lSizes[_NORMAL_BLOCK], "Throwing tracking growth leaked raw block");
#endif
    }
    void RejectedRequests(bool tracked) {
        const auto before = CaptureAllocations();
        Require(AmoMemory_Allocate(0) == nullptr && AmoMemory_ReAlloc(nullptr, 0) == nullptr, "Zero allocation not rejected");
        {
            Failure fail(AllocationStage::Allocate);
            Require(AmoMemory_Allocate(64) == nullptr && AmoMemory_ReAlloc(nullptr, 64) == nullptr && fail.hit,
                "Raw Allocate failure did not return nullptr");
        }
        if (tracked) {
            // Fail after malloc, before recording: the raw block must be released by the real owner.
            _CrtMemState start{}, finish{};
            _CrtMemCheckpoint(&start);
            {
                Failure fail(AllocationStage::TrackingRecord);
                Require(AmoMemory_Allocate(64) == nullptr && fail.hit, "Tracking failure did not fail Allocate");
            }
            _CrtMemCheckpoint(&finish);
#ifdef _DEBUG
            Require(start.lCounts[_NORMAL_BLOCK] == finish.lCounts[_NORMAL_BLOCK] &&
                start.lSizes[_NORMAL_BLOCK] == finish.lSizes[_NORMAL_BLOCK], "Metadata failure leaked raw malloc block");
#endif
            Require(AmoMemory_Allocate((std::numeric_limits<std::size_t>::max)()) == nullptr,
                "Sentinel padding overflow was not rejected");
        }
        SameAllocations(before);
    }
    void Reallocation(bool tracked, bool guards = true) {
        const auto before = CaptureAllocations();
        auto* memory = static_cast<unsigned char*>(AmoMemory_ReAlloc(nullptr, 32));
        Require(memory != nullptr, "ReAlloc nullptr did not allocate");
        for (unsigned i = 0; i < 32; ++i) memory[i] = static_cast<unsigned char>(i + 1);
        const auto owned = CaptureAllocations();
        for (auto stage : {AllocationStage::SentinelCopy, AllocationStage::Reallocate}) {
            if ((!tracked || !guards) && stage == AllocationStage::SentinelCopy) continue;
            {
                Failure fail(stage);
                Require(AmoMemory_ReAlloc(memory, 128) == nullptr && fail.hit, "ReAlloc fault was not reached");
            }
            for (unsigned i = 0; i < 32; ++i) Require(memory[i] == i + 1, "Failed ReAlloc changed old payload");
            SameAllocations(owned);
        }
        Require(AmoMemory_ReAlloc(memory, 0) == nullptr, "Zero ReAlloc was not rejected");
        if (tracked && guards) Require(AmoMemory_ReAlloc(memory, (std::numeric_limits<std::size_t>::max)()) == nullptr,
            "ReAlloc padding overflow was not rejected");
        SameAllocations(owned);
        fail_new = true;
        auto* grown = static_cast<unsigned char*>(AmoMemory_ReAlloc(memory, 4096));
        fail_new = false;
        Require(grown != nullptr, "ReAlloc grow failed");
        for (unsigned i = 0; i < 32; ++i) Require(grown[i] == i + 1, "Grow lost payload");
        auto* shrunk = static_cast<unsigned char*>(AmoMemory_ReAlloc(grown, 8));
        Require(shrunk != nullptr, "ReAlloc shrink failed");
        for (unsigned i = 0; i < 8; ++i) Require(shrunk[i] == i + 1, "Shrink lost payload");
        Require(CaptureAllocations().sentinels_intact, "ReAlloc damaged sentinels");
        if (tracked) Require(CaptureAllocations().payload_bytes == before.payload_bytes + 8, "ReAlloc size tracking stale");
        AmoMemory_Free(shrunk);
        SameAllocations(before);
    }
    void SentinelEvidence() {
        auto* memory = static_cast<unsigned char*>(AmoMemory_Allocate(16));
        Require(memory != nullptr, "Sentinel test allocation failed");
        memory[16] = 0; // Deliberate private guard corruption, not payload access by production.
        auto* resized = static_cast<unsigned char*>(AmoMemory_ReAlloc(memory, 64));
        Require(resized && !CaptureAllocations().sentinels_intact && resized[64] == 0,
            "ReAlloc erased old corruption evidence");
        resized[64] = 'A';
        Require(CaptureAllocations().sentinels_intact, "Sentinel evidence repair failed");
        AmoMemory_Free(resized);
    }
}

void* operator new(std::size_t bytes) {
    if (fail_new) throw std::bad_alloc();
    auto* result = std::malloc(bytes ? bytes : 1);
    if (!result) throw std::bad_alloc();
    return result;
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main(int argc, char** argv) {
    try {
        const bool tracked = argc == 2 && std::string_view(argv[1]) == "--tracked";
        Require(argc == 1 || tracked, "Unexpected allocator argument");
        AmoBase::AmoMemory_Init(tracked, 1024);
        if (tracked) RealTrackingGrowthFailure();
        RejectedRequests(tracked);
        Reallocation(tracked);
        if (tracked) {
            SentinelEvidence();
            AmoBase::AmoMemory_Init(true, 0); // No outstanding allocations when changing guard size.
            Reallocation(true, false);
        }
        Require(CaptureAllocations().active_blocks == 0, "Allocator test leaked tracked blocks");
        std::cout << "Allocator: rejection, failure preservation, resize and guards passed; tracking=" << tracked << '\n';
        return 0;
    } catch (const std::exception& error) { fail_new = false; std::cerr << error.what() << '\n'; return 1; }
}
