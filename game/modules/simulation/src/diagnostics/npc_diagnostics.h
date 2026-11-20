#pragma once

#include <symocraft/simulation/session.h>

namespace SymoCraft::Simulation::Detail {
    // Narrow test-only setup for otherwise impractical counter and invariant limits.
    struct NpcDiagnosticsAdapter {
        static void SetResource(SimulationSession& session, ECS::EntityId entity,
                                std::string resource, std::uint64_t quantity);
        static void FailAfterRemoveOnce(SimulationSession& session);
    };
}
