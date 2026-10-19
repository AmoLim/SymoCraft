#pragma once
#include <symocraft/simulation/session.h>
#include <symocraft/simulation/component.h>

namespace SymoCraft::Simulation::Detail {
    struct MotionDiagnostics {
        PlayerPose pose;
        Physics::RigidBody body;
        Character::CharacterComponent character;
        LensState lens;
    };
    struct SessionDiagnostics {
        SessionState state;
        double budget_seconds;
        float edit_cooldown, material_cooldown;
        int selected_block;
        bool resume_pending;
        std::optional<MotionDiagnostics> motion;
    };
    // Production-owned adapter, authorized to narrow root tests only. No stored borrows.
    struct SessionDiagnosticsAdapter {
        static SessionDiagnostics Capture(const SimulationSession& session);
    };
}
