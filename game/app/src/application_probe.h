#pragma once
#include "shutdown_report.h"
#include "demo_control.h"
#include <symocraft/simulation/session.h>

namespace SymoCraft::Application::Detail {
    enum class AssemblyStage { DebugState, Definition, Runtime, Window, Renderer, Atlas, World, Registry, Session, Meshes, Count };
    using AssemblyProbe = void (*)(void*, AssemblyStage);
    using ShutdownProbe = void (*)(void*, ShutdownStep);
    // Private synchronous probes; no alternate owners or production/test library split.
    void SetAssemblyProbe(AssemblyProbe probe, void* context) noexcept;
    void SetShutdownProbe(ShutdownProbe probe, void* context) noexcept;
    struct DemoSnapshot {
        std::uint64_t round{}, world_id{}, frame_id{};
        DemoState state = DemoState::Ready;
        bool contest{}, advanced{}, session_paused{};
        Simulation::AdvanceStatistics statistics;
        Simulation::PlayerPose player;
        std::vector<Simulation::NpcDisplay> npcs;
        std::vector<World::BlockId> target_blocks;
    };
    using DemoProbe = void (*)(void*, const DemoSnapshot&);
    void SetDemoProbe(DemoProbe probe, void* context) noexcept;
}
