#pragma once

#include "../render_backend.h"
#include <algorithm>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <vector>

namespace SymoCraft::Rendering::Detail {
// One instance belongs to one native pool/buffer bundle; replacement starts uninitialized.
class FrameBindingCache {
public:
    struct Writes {
        bool uniform{}, texture{}, sampler{};
        bool Any() const noexcept { return uniform || texture || sampler; }
    };
private:
    struct Entry {
        bool uniform{}, sampler{};
        std::optional<BackendResourceId> texture;
    };
    std::vector<Entry> entries_;
public:
    FrameBindingCache() = default;
    explicit FrameBindingCache(std::size_t capacity) : entries_(capacity) {}
    std::size_t Capacity() const noexcept { return entries_.size(); }
    static std::size_t CapacityFor(std::size_t current, std::size_t required, std::size_t limit) {
        if (required > limit) throw std::length_error("Vulkan frame binding capacity exceeded");
        if (current >= required) return current;
        auto capacity = std::max(std::size_t{1}, current);
        while (capacity < required) capacity = capacity > limit / 2 ? limit : capacity * 2;
        return capacity;
    }
    void Swap(FrameBindingCache& other) noexcept { entries_.swap(other.entries_); }
    template<class Write>
    bool WriteUniform(std::size_t index, Write&& write) {
        assert(index < Capacity());
        auto& entry = entries_[index];
        if (entry.uniform) return false;
        write(Writes{true, false, false});
        entry.uniform = true;
        return true;
    }
    template<class Write>
    bool WriteMesh(std::size_t index, BackendResourceId texture, Write&& write) {
        assert(index < Capacity());
        auto& entry = entries_[index];
        const Writes dirty{!entry.uniform, !entry.texture || *entry.texture != texture, !entry.sampler};
        if (!dirty.Any()) return false;
        if (dirty.texture) entry.texture.reset();
        write(dirty);
        entry.uniform = true; entry.sampler = true; entry.texture = texture;
        return true;
    }
};
}
