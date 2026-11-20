#include "application.h"
#include "application_probe.h"
#include "startup_options.h"
#include <SDL3/SDL.h>
#include <chrono>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using namespace SymoCraft;
using namespace Application::Detail;
void Require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
struct Probe {
    SDL_WindowID window{};
    unsigned stage{}, held_frames{}, restarts{};
    bool first_resume{}, finished{};
    std::uint64_t frame{}, round{};
    std::set<std::uint64_t> worlds, sessions;
    DemoSnapshot saved;
    std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();

    void Push(SDL_Event event) { Require(SDL_PushEvent(&event), "Cannot queue demo input"); }
    void Key(SDL_Scancode key) {
        SDL_Event event{}; event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = window;
        event.key.scancode = key; event.key.down = true; Push(event);
        event.type = SDL_EVENT_KEY_UP; event.key.down = false; Push(event);
    }
    void Focus(bool gain) {
        SDL_Event event{}; event.type = gain ? SDL_EVENT_WINDOW_FOCUS_GAINED : SDL_EVENT_WINDOW_FOCUS_LOST;
        event.window.windowID = window; Push(event);
    }
    static const Simulation::NpcDisplay& Human(const DemoSnapshot& s) {
        for (const auto& npc : s.npcs) if (npc.kind == Simulation::NpcKind::Human) return npc;
        throw std::runtime_error("No human in demo snapshot");
    }
    static void Frozen(const DemoSnapshot& before, const DemoSnapshot& after) {
        Require(!after.advanced && after.session_paused && after.statistics.rule_seconds == 0 &&
                after.statistics.physical_steps == 0, "Paused/Ready demo advanced");
        Require(before.npcs.size() == after.npcs.size(), "Paused demo changed entity count");
        Require(before.target_blocks == after.target_blocks && before.player.position == after.player.position,
                "Frozen demo changed World or player position");
        for (std::size_t i=0; i<before.npcs.size(); ++i) {
            const auto& a=before.npcs[i]; const auto& b=after.npcs[i];
            Require(a.entity == b.entity && a.position == b.position && a.advance_id == b.advance_id &&
                    a.horizontal_distance == b.horizontal_distance && a.action.id == b.action.id &&
                    a.action.phase == b.action.phase && a.action.elapsed_seconds == b.action.elapsed_seconds &&
                    a.resources.size() == b.resources.size(), "Frozen demo facts changed");
            for (std::size_t j=0; j<a.resources.size(); ++j)
                Require(a.resources[j].resource_id == b.resources[j].resource_id &&
                        a.resources[j].quantity == b.resources[j].quantity, "Paused resource changed");
        }
    }
    void Fresh(const DemoSnapshot& s) {
        Require(s.state == DemoState::Ready && s.session_paused && !s.advanced, "New demo not Ready");
        Require(worlds.insert(s.world_id).second && sessions.insert(Human(s).session_id).second,
                "Replay reused World or Session identity");
        Require(s.round > round, "Replay did not advance round"); round=s.round;
        Require(s.target_blocks.size() == 2 && s.target_blocks[0] == 12 && s.target_blocks[1] == 12,
                "Replay did not restore actual resource blocks");
        unsigned humans=0,zombies=0;
        for (const auto& npc:s.npcs) {
            humans += npc.kind == Simulation::NpcKind::Human;
            zombies += npc.kind == Simulation::NpcKind::Zombie;
            Require(npc.action.phase == Simulation::ActionPhase::Idle && npc.action.id == 0 &&
                    npc.action.elapsed_seconds == 0 && npc.resources.empty() && npc.horizontal_distance == 0,
                    "Replay retained old action, motion, or credit");
        }
        Require(humans == (s.contest ? 2u : 1u) && zombies == 1, "Wrong demo scene population");
    }
    static void Completed(const DemoSnapshot& s) {
        unsigned succeeded=0,failed=0;
        for (const auto& npc:s.npcs) {
            if (npc.kind == Simulation::NpcKind::Zombie) {
                Require(npc.action.phase == Simulation::ActionPhase::Idle && npc.resources.empty(), "Zombie left Idle");
                continue;
            }
            if (npc.action.phase == Simulation::ActionPhase::Succeeded) {
                ++succeeded;
                Require(npc.resources.size() == 1 && npc.resources.front().resource_id == "wood" &&
                        npc.resources.front().quantity == 2, "Winner missing exactly one real resource credit");
            } else {
                ++failed;
                Require(npc.action.phase == Simulation::ActionPhase::Failed &&
                        npc.action.failure == Simulation::ActionFailure::InvalidTarget && npc.resources.empty(),
                        "Contest loser received credit or wrong terminal");
            }
        }
        Require(succeeded == 1 && failed == (s.contest ? 1u : 0u), "Incorrect demo settlement");
        Require(s.target_blocks.size() == 2 && s.target_blocks[0] == 1 && s.target_blocks[1] == 12,
                "Settlement does not match real World removal");
    }
    void Observe(const DemoSnapshot& s) {
        Require(std::chrono::steady_clock::now()-started < std::chrono::seconds(75), "Demo probe timed out");
        Require(s.frame_id > frame, "Replay reset native frame sequence"); frame=s.frame_id;
        const auto& human=Human(s);
        switch(stage) {
        case 0:
            if (!held_frames) { Fresh(s); saved=s; }
            Frozen(saved,s);
            if (++held_frames == 4) { Key(SDL_SCANCODE_SPACE); stage=1; }
            break;
        case 1:
            if (s.advanced && human.action.phase == Simulation::ActionPhase::Moving && human.horizontal_distance > .05) {
                saved=s; Key(SDL_SCANCODE_SPACE); held_frames=0; stage=2;
            }
            break;
        case 2:
            Require(s.state == DemoState::Paused, "Movement did not pause"); Frozen(saved,s);
            if (++held_frames == 2) { Focus(false); Focus(true); Key(SDL_SCANCODE_W); }
            if (held_frames == 5) { Key(SDL_SCANCODE_SPACE); first_resume=true; stage=3; }
            break;
        case 3:
            if (s.advanced && first_resume) {
                Require(s.statistics.rule_seconds == 0 && s.statistics.physical_steps == 0 &&
                        human.horizontal_distance == Human(saved).horizontal_distance,
                        "Resume caught up movement"); first_resume=false;
            }
            if (s.advanced && human.action.phase == Simulation::ActionPhase::Harvesting && human.action.elapsed_seconds > .1) {
                saved=s; Key(SDL_SCANCODE_SPACE); held_frames=0; stage=4;
            }
            break;
        case 4:
            Require(s.state == DemoState::Paused, "Harvest did not pause"); Frozen(saved,s);
            if (++held_frames == 4) { Key(SDL_SCANCODE_SPACE); first_resume=true; stage=5; }
            break;
        case 5:
            if (s.advanced && first_resume) {
                Require(s.statistics.rule_seconds == 0 && human.action.elapsed_seconds == Human(saved).action.elapsed_seconds,
                        "Resume caught up harvest time"); first_resume=false;
            }
            if (s.state == DemoState::Completed) { Completed(s); saved=s; Key(SDL_SCANCODE_Q); stage=6; }
            break;
        case 6:
            Fresh(s);
            if (++restarts < 4) Key(SDL_SCANCODE_E);
            else { Require(s.contest, "Expected contest after scene switches"); Key(SDL_SCANCODE_SPACE); stage=7; }
            break;
        case 7:
            if (s.advanced && human.action.phase == Simulation::ActionPhase::Moving && human.horizontal_distance > .05) {
                if (restarts == 4) { Key(SDL_SCANCODE_Q); stage=10; break; }
                if (restarts == 5) { saved=s; Key(SDL_SCANCODE_SPACE); held_frames=0; stage=11; break; }
            }
            if (s.state == DemoState::Completed) { Completed(s); saved=s; held_frames=0; stage=8; }
            break;
        case 8:
            Completed(s); Frozen(saved,s);
            if (++held_frames==3) { Key(SDL_SCANCODE_Q); stage=9; }
            break;
        case 9:
            Fresh(s); finished=true; Key(SDL_SCANCODE_ESCAPE); stage=10;
            break;
        case 10:
            Fresh(s); ++restarts; Key(SDL_SCANCODE_SPACE); stage=7;
            break;
        case 11:
            Require(s.state == DemoState::Paused, "Contest did not pause before restart"); Frozen(saved,s);
            if (++held_frames == 3) { Key(SDL_SCANCODE_Q); stage=12; }
            break;
        case 12:
            Fresh(s); ++restarts; Key(SDL_SCANCODE_SPACE); stage=7;
            break;
        default: break;
        }
    }
};
}
int main(int argc,char** argv) {
    try {
        Require(argc==2 || argc==3, "Expected renderer and optional restart-fault mode");
        const bool fault_mode=argc==3 && std::string(argv[2])=="restart-fault";
        Require(argc==2 || fault_mode,"Unknown demo probe mode");
        // Explicit finite-frame diagnostics may run behind the editor window.
        // The normal interactive demo retains ordinary real-focus suspension.
        const std::string_view args[]{"--renderer",argv[1],"--npc-scene","demo","--validation","1",
                                      "--smoke-frames","10000"};
        const auto options=SymoCraft::ParseStartupOptions(args);
        SymoCraft::Application::Application app(options,std::chrono::steady_clock::now());
        app.Initialize();
        int count=0; auto** windows=SDL_GetWindows(&count);
        Require(windows && count==1,"Expected one demo window");
        Probe probe; probe.window=SDL_GetWindowID(windows[0]); SDL_free(windows);
        probe.Focus(true);
        bool fault_injected=false,expected_fault=false;
        if (fault_mode) {
            SetAssemblyProbe([](void* p,AssemblyStage stage) {
                if (stage==AssemblyStage::Meshes) {
                    *static_cast<bool*>(p)=true;
                    throw std::runtime_error("Injected demo restart assembly failure");
                }
            }, &fault_injected);
            SetDemoProbe([](void* p,const DemoSnapshot& s) {
                Require(s.state==DemoState::Ready,"Fault probe expected Ready before restart");
                static_cast<Probe*>(p)->Key(SDL_SCANCODE_Q);
            }, &probe);
        } else SetDemoProbe([](void* p,const DemoSnapshot& s){static_cast<Probe*>(p)->Observe(s);}, &probe);
        try { app.Run(); }
        catch (const std::exception& error) {
            SetDemoProbe(nullptr,nullptr); SetAssemblyProbe(nullptr,nullptr);
            if (!fault_mode || std::string(error.what())!="Injected demo restart assembly failure") throw;
            expected_fault=true;
        }
        SetDemoProbe(nullptr,nullptr);
        SetAssemblyProbe(nullptr,nullptr);
        Require(fault_mode ? fault_injected && expected_fault : probe.finished,"Demo exited before full validation");
        Require(app.Shutdown().Succeeded(),"Demo shutdown failed");
        std::cout<<"[demo-probe] passed; backend="<<argv[1]<<"; restart_fault="<<fault_mode<<"; rounds="<<probe.worlds.size()
                 <<"; pause_movement=true; pause_harvest=true; focus_latch=true; settlement=true; frame_sequence=true\n";
        return 0;
    } catch(const std::exception& error) {
        SetDemoProbe(nullptr,nullptr); SetAssemblyProbe(nullptr,nullptr);
        std::cerr<<"[demo-probe] failed: "<<error.what()<<'\n'; return 1;
    }
}
