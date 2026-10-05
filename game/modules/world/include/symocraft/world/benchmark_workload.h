#pragma once
#include <symocraft/world/test_scene.h>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace SymoCraft::Benchmark {
    inline constexpr double EditInterval = 0.25;
    inline std::uint64_t DueEdits(double elapsed)
    {
        if (!std::isfinite(elapsed) || elapsed < 0 || elapsed > 1201)
            throw std::invalid_argument("Invalid benchmark elapsed time");
        return static_cast<std::uint64_t>(std::floor(elapsed / EditInterval));
    }
    inline TestScene::Edit CycleEdit(std::uint64_t index)
    {
        const auto edits = TestScene::Edits();
        const auto offset = static_cast<std::size_t>(index % (edits.size() * 2));
        if (offset < edits.size()) return edits[offset];
        const auto& reverse = edits[edits.size() * 2 - 1 - offset];
        return {reverse.position, reverse.after, reverse.before};
    }
}
