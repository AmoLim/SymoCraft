#pragma once

#include <array>
#include <cstddef>

namespace SymoCraft::KeySampling
{
    enum class Control : std::size_t
    {
        Exit, Run, Sensor, Descend, Forward, Backward, Right, Left, Jump, NextBlock, PreviousBlock,
        Count
    };

    inline constexpr auto Count = static_cast<std::size_t>(Control::Count);

    struct Snapshot
    {
        std::array<bool, Count> pressed{};

        bool Down(Control control) const { return pressed[static_cast<std::size_t>(control)]; }
    };

    // Consume every sticky key once, even when later gameplay conditions short-circuit.
    template<typename ReadKey>
    Snapshot Capture(ReadKey&& read_key)
    {
        Snapshot result;
        for (std::size_t index = 0; index < Count; ++index)
            result.pressed[index] = read_key(static_cast<Control>(index));
        return result;
    }
}
