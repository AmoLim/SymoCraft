#include "backend/vulkan/frame_binding_cache.h"
#include <array>
#include <iostream>
#include <limits>
#include <utility>

namespace {
using namespace SymoCraft::Rendering::Detail;
void Require(bool ok) { if (!ok) throw std::runtime_error("Vulkan frame binding basic check failed"); }
unsigned Mask(FrameBindingCache::Writes writes) {
    return unsigned(writes.uniform) | (unsigned(writes.texture) << 1) | (unsigned(writes.sampler) << 2);
}
void NormalBindingChanges() {
    FrameBindingCache cache(4);
    std::array<BackendResourceId,2> ids{{{1,2,3},{1,4,5}}};
    std::vector<unsigned> masks;
    auto apply = [&] {
        masks.clear();
        for (size_t i = 0; i < ids.size(); ++i) {
            const bool written = cache.WriteMesh(i, ids[i], [&](auto dirty) { masks.push_back(Mask(dirty)); });
            if (!written) masks.push_back(0);
        }
    };
    apply(); Require(masks == std::vector<unsigned>{7,7});
    apply(); Require(masks == std::vector<unsigned>{0,0});
    ids[0] = {1,6,7};
    apply(); Require(masks == std::vector<unsigned>{2,0});
    std::swap(ids[0], ids[1]);
    apply(); Require(masks == std::vector<unsigned>{2,2});
    apply(); Require(masks == std::vector<unsigned>{0,0});
}
void IndependentFrameSlots() {
    std::array<FrameBindingCache,3> slots{FrameBindingCache(4), FrameBindingCache(4), FrameBindingCache(4)};
    unsigned initialized = 0;
    for (auto& slot : slots)
        Require(slot.WriteMesh(0, {1,2,3}, [&](auto dirty) { Require(Mask(dirty) == 7); ++initialized; }));
    Require(initialized == 3);
    for (auto& slot : slots)
        Require(!slot.WriteMesh(0, {1,2,3}, [&](auto) { ++initialized; }));
    Require(initialized == 3);
}
void SeparateSelectionBinding() {
    FrameBindingCache meshes(4), selection(1);
    unsigned selection_writes = 0;
    auto select = [&] {
        return selection.WriteUniform(0, [&](auto dirty) { Require(Mask(dirty) == 1); ++selection_writes; });
    };
    Require(select());
    for (size_t count : {size_t{1}, size_t{3}, size_t{2}}) {
        for (size_t i = 0; i < count; ++i) meshes.WriteMesh(i, {1,i+1,1}, [](auto) {});
        Require(!select());
    }
    Require(selection_writes == 1);
}
void NormalBundleReplacement() {
    FrameBindingCache cache(4);
    cache.WriteMesh(0, {1,2,3}, [](auto) {});
    const auto capacity = FrameBindingCache::CapacityFor(cache.Capacity(), 6, 64);
    Require(capacity == 8);
    FrameBindingCache candidate(capacity);
    Require(!cache.WriteMesh(0, {1,2,3}, [](auto) {}));
    cache.Swap(candidate);
    unsigned writes = 0;
    Require(cache.WriteMesh(0, {1,2,3}, [&](auto dirty) { Require(Mask(dirty) == 7); ++writes; }));
    Require(writes == 1 && cache.Capacity() == 8 && candidate.Capacity() == 4);
    Require(FrameBindingCache::CapacityFor(cache.Capacity(), 6, 64) == 8);
}
void FailedWritesRetry() {
    FrameBindingCache cache(2), selection(1);
    auto fail = [](auto) { throw std::runtime_error("controlled descriptor write failure"); };
    bool rejected = false;
    try { cache.WriteMesh(0, {1, 2, 3}, fail); }
    catch (const std::runtime_error&) { rejected = true; }
    Require(rejected);
    Require(cache.WriteMesh(0, {1, 2, 3}, [](auto dirty) { Require(Mask(dirty) == 7); }));
    rejected = false;
    try { cache.WriteMesh(0, {1, 4, 5}, fail); }
    catch (const std::runtime_error&) { rejected = true; }
    Require(rejected);
    Require(cache.WriteMesh(0, {1, 2, 3}, [](auto dirty) { Require(Mask(dirty) == 2); }));
    Require(!cache.WriteMesh(0, {1, 2, 3}, fail));
    rejected = false;
    try { selection.WriteUniform(0, fail); }
    catch (const std::runtime_error&) { rejected = true; }
    Require(rejected);
    Require(selection.WriteUniform(0, [](auto dirty) { Require(Mask(dirty) == 1); }));
    Require(!selection.WriteUniform(0, fail));
}
void CapacityBoundaries() {
    Require(FrameBindingCache::CapacityFor(0, 0, 0) == 0);
    Require(FrameBindingCache::CapacityFor(0, 1, 1) == 1);
    Require(FrameBindingCache::CapacityFor(4, 7, 7) == 7);
    const auto limit = std::numeric_limits<std::size_t>::max();
    Require(FrameBindingCache::CapacityFor(limit / 2 + 1, limit, limit) == limit);
    bool rejected = false;
    try { (void)FrameBindingCache::CapacityFor(4, 8, 7); }
    catch (const std::length_error&) { rejected = true; }
    Require(rejected);
}
}
int main() {
    try {
        NormalBindingChanges(); IndependentFrameSlots(); SeparateSelectionBinding(); NormalBundleReplacement();
        FailedWritesRetry(); CapacityBoundaries();
        std::cout << "6 CPU metadata cases passed: binding deltas, independent slots, selection, replacement, failed-write retry and capacity boundaries; no native GPU allocation or performance claim\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
