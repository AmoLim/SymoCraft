#include "backend/d3d12/srv_binding_cache.h"
#include <array>
#include <iostream>
#include <limits>

namespace {
using namespace SymoCraft::Rendering::Detail;
void Require(bool ok) { if (!ok) throw std::runtime_error("SRV cache basic check failed"); }
void NormalChanges() {
    SrvBindingCache cache(8);
    const BackendResourceId first{1, 2, 3}, second{1, 4, 5}, third{1, 6, 7};
    std::array ids{first, first, second};
    std::vector<std::size_t> writes;
    auto apply = [&] {
        writes.clear();
        return cache.WriteChanged(ids.size(), [&](std::size_t index) { return ids[index]; },
            [&](std::size_t index) { writes.push_back(index); });
    };
    Require(apply() == 3 && writes == std::vector<std::size_t>{0, 1, 2});
    Require(apply() == 0 && writes.empty());
    ids[1] = third;
    Require(apply() == 1 && writes == std::vector<std::size_t>{1});
    Require(apply() == 0);
    std::swap(ids[0], ids[2]);
    Require(apply() == 2 && writes == std::vector<std::size_t>{0, 2});
    Require(cache.Capacity() == 8);
}
void IndependentSlots() {
    std::array<SrvBindingCache, 3> slots{SrvBindingCache(8), SrvBindingCache(8), SrvBindingCache(8)};
    const std::array<BackendResourceId, 3> ids{{{1, 2, 3}, {1, 4, 5}, {1, 6, 7}}};
    unsigned writes = 0;
    auto apply = [&](SrvBindingCache& slot) {
        return slot.WriteChanged(ids.size(), [&](std::size_t index) { return ids[index]; },
            [&](std::size_t) { ++writes; });
    };
    for (auto& slot : slots) Require(apply(slot) == 3);
    Require(writes == 9);
    for (auto& slot : slots) Require(apply(slot) == 0);
    Require(writes == 9);
}
void OrdinaryCapacityPreparation() {
    SrvBindingCache cache(8);
    const auto capacity = SrvBindingCache::CapacityFor(cache.Capacity(), 12, 64);
    Require(capacity == 16);
    SrvBindingCache replacement(capacity);
    cache.Swap(replacement);
    Require(cache.Capacity() == 16 && replacement.Capacity() == 8);
    Require(SrvBindingCache::CapacityFor(cache.Capacity(), 12, 64) == 16);
    unsigned writes = 0;
    Require(cache.WriteChanged(12, [](std::size_t index) { return BackendResourceId{1, index + 1, 1}; },
        [&](std::size_t) { ++writes; }) == 12);
    Require(writes == 12);
}
void FailedWritesRetry() {
    SrvBindingCache cache(3);
    std::array<BackendResourceId, 3> ids{{{1, 2, 3}, {1, 4, 5}, {1, 6, 7}}};
    bool failed = false;
    try {
        cache.WriteChanged(ids.size(), [&](auto i) { return ids[i]; }, [](auto i) {
            if (i == 1) throw std::runtime_error("controlled descriptor write failure");
        });
    } catch (const std::runtime_error&) { failed = true; }
    Require(failed);
    std::vector<std::size_t> writes;
    auto apply = [&] {
        writes.clear();
        return cache.WriteChanged(ids.size(), [&](auto i) { return ids[i]; },
            [&](auto i) { writes.push_back(i); });
    };
    Require(apply() == 2 && writes == std::vector<std::size_t>{1, 2});
    ids[1] = {1, 8, 9};
    failed = false;
    try {
        cache.WriteChanged(ids.size(), [&](auto i) { return ids[i]; }, [](auto) {
            throw std::runtime_error("controlled replacement failure");
        });
    } catch (const std::runtime_error&) { failed = true; }
    Require(failed);
    ids[1] = {1, 4, 5};
    Require(apply() == 1 && writes == std::vector<std::size_t>{1});
    Require(apply() == 0);
}
void CapacityBoundaries() {
    Require(SrvBindingCache::CapacityFor(0, 0, 0) == 0);
    Require(SrvBindingCache::CapacityFor(0, 1, 1) == 1);
    Require(SrvBindingCache::CapacityFor(4, 7, 7) == 7);
    const auto limit = std::numeric_limits<std::size_t>::max();
    Require(SrvBindingCache::CapacityFor(limit / 2 + 1, limit, limit) == limit);
    bool rejected = false;
    try { (void)SrvBindingCache::CapacityFor(4, 8, 7); }
    catch (const std::length_error&) { rejected = true; }
    Require(rejected);
}
}
int main() {
    try {
        NormalChanges(); IndependentSlots(); OrdinaryCapacityPreparation(); FailedWritesRetry(); CapacityBoundaries();
        std::cout << "5 CPU metadata cases passed: binding deltas, independent slots, replacement, failed-write retry and capacity boundaries; no native GPU allocation or performance claim\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
