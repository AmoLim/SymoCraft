#pragma once
#include "shutdown_report.h"

namespace SymoCraft::Application::Detail {
    enum class AssemblyStage { Observation, Definition, Runtime, Window, Renderer, Atlas, World, Registry, Session, Meshes, Count };
    using AssemblyProbe = void (*)(void*, AssemblyStage);
    using ShutdownProbe = void (*)(void*, ShutdownStep);
    // Private synchronous probes; no alternate owners or production/test library split.
    void SetAssemblyProbe(AssemblyProbe probe, void* context) noexcept;
    void SetShutdownProbe(ShutdownProbe probe, void* context) noexcept;
}
