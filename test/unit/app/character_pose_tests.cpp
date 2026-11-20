#include "character_pose.h"
#include <symocraft/ecs/registry.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace SymoCraft;
using namespace SymoCraft::Application::Detail;
namespace {
void Require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
bool Neutral(const CharacterPose& pose) {
    return pose.left_arm==0 && pose.right_arm==0 && pose.left_leg==0 && pose.right_leg==0;
}
}
int main() {
    try {
        ECS::Registry registry;
        const auto first=registry.CreateEntity(),second=registry.CreateEntity();
        CharacterPoses poses;
        CharacterMotion a{1,1,first,0,{7,Simulation::ActionPhase::Moving}};
        Require(Neutral(poses.Update(a)),"unmoved NPC must start neutral");
        a.advance_id=2; a.horizontal_distance=.3;
        const auto walk=poses.Update(a);
        Require(std::abs(walk.left_leg)>.1 && walk.left_leg==-walk.right_leg &&
            walk.left_arm==-walk.right_arm,"actual walking must swing opposite limbs");
        Require(poses.Update(a).left_leg==walk.left_leg,"paused/repeated fact must preserve exact pose");
        CharacterMotion b{1,2,second,0,{8,Simulation::ActionPhase::Idle}};
        Require(Neutral(poses.Update(b)),"shared models must not share pose state");
        a.advance_id=3;
        Require(Neutral(poses.Update(a)),"blocked NPC must not walk in place");
        a.action.phase=Simulation::ActionPhase::Harvesting;
        a.action.duration_seconds=2; a.action.elapsed_seconds=.15;
        const auto harvest=poses.Update(a);
        Require(harvest.right_arm<-.3 && harvest.left_leg==0,"harvesting must drive arms only");
        Require(poses.Update(a).right_arm==harvest.right_arm,"repeated harvesting display must freeze");
        a.advance_id=4; a.action.elapsed_seconds=.3;
        Require(poses.Update(a).right_arm!=harvest.right_arm,"effective harvest time must change pose");
        // Commit/cancel can change phase after Advance, without changing its identity.
        for (auto phase : {Simulation::ActionPhase::Ready,Simulation::ActionPhase::Succeeded,
                           Simulation::ActionPhase::Failed,Simulation::ActionPhase::Cancelled,
                           Simulation::ActionPhase::Idle}) {
            a.action.phase=phase;
            Require(Neutral(poses.Update(a)),"same-advance terminal phase must immediately stop harvest swing");
        }
        a.session_id=2; a.action.phase=Simulation::ActionPhase::Moving; a.horizontal_distance=0;
        Require(Neutral(poses.Update(a)),"new run must not inherit old gait");
        const ECS::EntityId only[]{second}; poses.Retain(only);
        a.horizontal_distance=.5;
        Require(Neutral(poses.Update(a)),"removed identity must not retain presentation history");
        a.action.elapsed_seconds=std::numeric_limits<double>::quiet_NaN();
        bool rejected=false;
        try { poses.Update(a); } catch (const std::invalid_argument&) { rejected=true; }
        Require(rejected,"nonfinite motion fact must be rejected");
        std::cout<<"character pose contracts passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
