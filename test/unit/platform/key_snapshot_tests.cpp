#include "symocraft/platform/key_snapshot.h"

#include <iostream>
#include <stdexcept>

namespace Keys = SymoCraft::KeySampling;

namespace
{
    void Require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }
}

int main()
{
    try
    {
        std::array<bool, Keys::Count> pending{};
        std::array<unsigned, Keys::Count> reads{};
        pending.fill(true);
        const auto consume = [&](Keys::Control key)
        {
            const auto index = static_cast<std::size_t>(key);
            ++reads[index];
            const bool value = pending[index];
            pending[index] = false;
            return value;
        };
        const auto first = Keys::Capture(consume);
        Require(first.Down(Keys::Control::Forward) || first.Down(Keys::Control::Backward),
                "Captured movement was not available");
        Require(first.Down(Keys::Control::Exit) && first.Down(Keys::Control::Jump) &&
                first.Down(Keys::Control::NextBlock) && first.Down(Keys::Control::PreviousBlock),
                "A short action press was lost during capture");
        for (std::size_t index = 0; index < Keys::Count; ++index)
            Require(reads[index] == 1 && !pending[index], "Capture skipped or reread a sticky control");
        const auto second = Keys::Capture(consume);
        for (std::size_t index = 0; index < Keys::Count; ++index)
            Require(!second.pressed[index], "A short-circuited old key leaked into the next frame");
        Require(first.Down(Keys::Control::Jump) && first.Down(Keys::Control::Jump),
                "Reading a snapshot consumed its stored action");

        const auto held = [](Keys::Control key) { return key == Keys::Control::Forward; };
        Require(Keys::Capture(held).Down(Keys::Control::Forward) &&
                Keys::Capture(held).Down(Keys::Control::Forward), "Held-key semantics were not retained");
        std::cout << "Complete key snapshot and sticky-consumption contracts passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Key snapshot test failed: " << error.what() << '\n';
        return 1;
    }
}
