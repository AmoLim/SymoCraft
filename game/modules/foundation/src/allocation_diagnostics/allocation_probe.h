#pragma once
#include <cstddef>

namespace AmoBase::Detail {
    // Private, thread-local fault seam; no public allocator replacement or test dependency.
    enum class AllocationStage { Allocate, Reallocate, SentinelCopy, TrackingRecord };
    using AllocationFailureProbe = bool (*)(AllocationStage, void*) noexcept;
    void SetAllocationFailureProbe(AllocationFailureProbe probe, void* context) noexcept;

    struct AllocationSnapshot {
        std::size_t active_blocks = 0;
        std::size_t payload_bytes = 0;
        bool sentinels_intact = true;
    };
    AllocationSnapshot CaptureAllocations();
}
