#pragma once

#include "../render_backend.h"
#include <algorithm>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <vector>

namespace SymoCraft::Rendering::Detail {
// Identity-only metadata: the frame's native refs, not this cache, own textures.
class SrvBindingCache {
    std::vector<std::optional<BackendResourceId>> bindings_;
public:
    SrvBindingCache() = default;
    explicit SrvBindingCache(std::size_t capacity) : bindings_(capacity) {}
    std::size_t Capacity() const noexcept { return bindings_.size(); }
    static std::size_t CapacityFor(std::size_t current, std::size_t required, std::size_t limit) {
        if (required > limit) throw std::length_error("SRV cache exceeds descriptor limit");
        if (current >= required) return current;
        auto capacity = std::max(std::size_t{1}, current);
        while (capacity < required) capacity = capacity > limit / 2 ? limit : capacity * 2;
        return capacity;
    }
    void Swap(SrvBindingCache& other) noexcept { bindings_.swap(other.bindings_); }
    void Clear() noexcept {
        std::vector<std::optional<BackendResourceId>> empty;
        bindings_.swap(empty);
    }
    template<class Identity, class Write>
    std::size_t WriteChanged(std::size_t count, Identity&& identity, Write&& write) {
        assert(count <= Capacity());
        std::size_t written = 0;
        for (std::size_t index = 0; index < count; ++index) {
            const auto id = identity(index);
            if (bindings_[index] && *bindings_[index] == id) continue;
            // A writer that fails after touching a descriptor must not leave a cache hit.
            bindings_[index].reset();
            write(index);
            bindings_[index] = id;
            ++written;
        }
        return written;
    }
};
}
