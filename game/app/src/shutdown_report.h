#pragma once
#include <array>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <utility>

namespace SymoCraft::Application {
    enum class ShutdownStep : std::size_t {
        GpuSamples, MeshBindings, Texture, Renderer, Simulation, World, Registry, RendererObject, Window, Runtime, Count
    };
    enum class ReleaseFact { NotAcquired, Released, Failed, Unavailable };
    struct ShutdownItem {
        ReleaseFact fact = ReleaseFact::NotAcquired;
        std::exception_ptr error;
    };
    struct ShutdownReport {
        std::array<ShutdownItem, static_cast<std::size_t>(ShutdownStep::Count)> items{};
        template<class Action> void Attempt(ShutdownStep step, bool acquired, Action&& action) noexcept {
            if (!acquired) return;
            auto& item = items[static_cast<std::size_t>(step)];
            try { std::forward<Action>(action)(); item.fact = ReleaseFact::Released; }
            catch (...) { item.fact = ReleaseFact::Failed; item.error = std::current_exception(); }
        }
        void Unavailable(ShutdownStep step) noexcept { items[static_cast<std::size_t>(step)].fact = ReleaseFact::Unavailable; }
        bool Succeeded() const noexcept {
            for (std::size_t i = 0; i < items.size(); ++i)
                if (items[i].fact == ReleaseFact::Failed ||
                    (i != static_cast<std::size_t>(ShutdownStep::GpuSamples) && items[i].fact == ReleaseFact::Unavailable)) return false;
            return true;
        }
    };
    class ShutdownError : public std::runtime_error {
    public:
        explicit ShutdownError(ShutdownReport report) : std::runtime_error("Application shutdown failed"), report_(std::move(report)) {}
        const ShutdownReport& Report() const noexcept { return report_; }
    private:
        ShutdownReport report_;
    };
}
