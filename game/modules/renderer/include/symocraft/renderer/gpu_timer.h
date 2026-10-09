#pragma once
#include <array>
#include <cstddef>
#include <vector>

namespace SymoCraft {
    // Main-context thread only. Never waits for results or reuses a pending query.
    class GpuTimer {
    public:
        struct Result { std::size_t frame; double milliseconds; };
        GpuTimer();
        ~GpuTimer();
        GpuTimer(const GpuTimer&) = delete;
        GpuTimer& operator=(const GpuTimer&) = delete;
        void BeginFrame(std::size_t frame);
        void BeginDraw();
        void EndDraw();
        void EndFrame();
        std::vector<Result> Poll();
        bool Supported() const { return supported_; }
    private:
        struct Slot { std::array<unsigned int, 2> queries{}; std::size_t frame{}; unsigned count{}; bool pending{}; };
        std::array<Slot, 64> slots_{};
        Slot* current_{};
        bool supported_{}, active_draw_{};
    };
}
