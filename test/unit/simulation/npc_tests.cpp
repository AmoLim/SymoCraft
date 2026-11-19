#include <symocraft/simulation/session.h>
#include <symocraft/simulation/component.h>
#include "npc_diagnostics.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>

namespace {
    thread_local bool fail_next_allocation = false;
    using namespace SymoCraft;
    using namespace Simulation;
    void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
    template<class Error, class Call> void Throws(Call&& call) {
        try { call(); } catch (const Error&) { return; }
        throw std::runtime_error("Expected exception was not thrown");
    }
    void Register(ECS::Registry& registry) {
        registry.RegisterComponent<Transform>("Transform");
        registry.RegisterComponent<PhysicsStepOrder>("PhysicsStepOrder");
        registry.RegisterComponent<Physics::RigidBody>("RigidBody");
        registry.RegisterComponent<Physics::HitBox>("HitBox");
        registry.RegisterComponent<Character::CharacterComponent>("Character");
        registry.RegisterComponent<Character::PlayerComponent>("Player");
    }
    struct Fixture {
        ECS::Registry registry;
        std::unique_ptr<World::VoxelWorld> world;
        std::unique_ptr<SimulationSession> session;
        explicit Fixture(const World::BlockDefinition& definition) {
            Register(registry);
            world = World::VoxelWorld::Create(definition, {424242, 2, false}, nullptr, true);
            for (int x = -6; x <= 6; ++x) for (int z = -6; z <= 6; ++z)
                world->TryEdit({World::EditOperation::Set, {x, 199, z}, 5});
            world->TryEdit({World::EditOperation::Set, {0, 200, 0}, 6});
            session = SimulationSession::Create(registry, *world, {{-2.5f, 200.9f, .5f}, 0, -20});
        }
        ECS::EntityId Add(std::uint64_t key = 1, glm::vec3 position = {-.5f, 200.9f, .5f}, NpcKind kind = NpcKind::Human) {
            NpcConfig config;
            config.kind = kind;
            config.order_key = key;
            config.position = position;
            return session->AddNpc(config);
        }
        AdvanceResult Advance(double seconds = .1) { return session->Advance(seconds, {}); }
        void Step(double seconds = .1) { const auto result = Advance(seconds); session->CommitFrame(*world, result); }
        AdvanceResult Ready(ECS::EntityId entity) {
            for (unsigned frame = 0; frame < 120; ++frame) {
                auto result = Advance();
                const auto action = session->InspectNpc(entity).action;
                if (action.phase == ActionPhase::Ready) return result;
                Require(action.phase != ActionPhase::Failed, ActionFailureName(action.failure));
                session->CommitFrame(*world, result);
            }
            throw std::runtime_error("NPC did not reach harvest-ready within bounded sequence");
        }
    };
    void AssemblyAndIsolation(const World::BlockDefinition& definition) {
        Fixture f(definition);
        const auto human = f.Add();
        const auto zombie = f.Add(2, {-.5f, 200.9f, .5f}, NpcKind::Zombie);
        Throws<Rejected>([&] { f.Add(1); });
        Throws<Rejected>([&] { f.session->StartHarvest(zombie, {0,200,0}); });
        const auto action = f.session->StartHarvest(human, {0,200,0});
        f.session->CancelAction(human, action);
        Require(f.session->InspectNpc(human).action.phase == ActionPhase::Cancelled, "Cancel did not terminate action");
        Require(f.session->InspectNpc(zombie).action.phase == ActionPhase::Idle, "Zombie action state leaked");
        auto old = f.session->InspectNpc(human);
        f.session->RemoveNpc(human);
        Throws<Rejected>([&] { f.session->InspectNpc(human); });
        const auto replacement = f.Add(1);
        Require(replacement != human && old.action.id == action, "Old identity or snapshot was reused");
        Throws<Rejected>([&] { f.session->CancelAction(replacement, action); });
    }
    void TimingAndPause(const World::BlockDefinition& definition) {
        Fixture f(definition);
        const auto human = f.Add();
        f.session->StartHarvest(human, {0,200,0});
        for (unsigned i=0; i<10 && f.session->InspectNpc(human).action.phase==ActionPhase::Moving; ++i) f.Step();
        Require(f.session->InspectNpc(human).action.phase==ActionPhase::Harvesting, "No legal harvest stage");
        const auto before = f.session->InspectNpc(human);
        auto frame = f.Advance(.00001);
        Require(frame.statistics.physical_steps == 0, "Timing fixture expected zero physical steps");
        const auto elapsed = f.session->InspectNpc(human).action.elapsed_seconds;
        Require(std::abs(elapsed-before.action.elapsed_seconds-frame.statistics.rule_seconds)<1e-9, "Zero-step harvest clock mismatch");
        f.session->CommitFrame(*f.world, frame);
        frame = f.Advance(.3);
        Require(std::abs(f.session->InspectNpc(human).action.elapsed_seconds-elapsed-frame.statistics.rule_seconds)<1e-9,
                "Long frame used raw time or physical-step time");
        f.session->CommitFrame(*f.world, frame);
        const auto paused = f.session->InspectNpc(human).action.elapsed_seconds;
        f.session->Pause();
        f.session->Resume();
        f.Step(5);
        Require(f.session->InspectNpc(human).action.elapsed_seconds==paused, "Resume caught up paused time");
        const auto id=f.session->InspectNpc(human).action.id;
        f.session->CancelAction(human,id);
        f.session->StartHarvest(human,{0,200,0});
        Require(f.session->InspectNpc(human).action.elapsed_seconds==0, "New action inherited progress");
    }
    void OrderedContest(const World::BlockDefinition& definition) {
        for (bool reverse : {false, true}) {
            Fixture f(definition);
            ECS::EntityId first, second;
            if (reverse) { second=f.Add(2); first=f.Add(1); }
            else { first=f.Add(1); second=f.Add(2); }
            f.session->StartHarvest(second,{0,200,0});
            f.session->StartHarvest(first,{0,200,0});
            const auto frame=f.Ready(first);
            f.session->CommitFrame(*f.world,frame);
            const auto winner=f.session->InspectNpc(first);
            const auto loser=f.session->InspectNpc(second);
            Require(winner.action.phase==ActionPhase::Succeeded && winner.resources.size()==1 && winner.resources[0].quantity==2,
                    "Explicit order did not pick a single credited winner");
            Require(loser.action.phase==ActionPhase::Failed && loser.resources.empty(), "Loser received resources");
            Throws<Rejected>([&] { f.session->CommitFrame(*f.world,frame); });
            for (unsigned i=0;i<4;++i) f.Step();
            Require(f.session->InspectNpc(first).resources[0].quantity==2, "Repeated advance credited a terminal action");
        }
    }
    void TargetReplacementAndRemoval(const World::BlockDefinition& definition) {
        Fixture f(definition);
        const auto human=f.Add();
        f.session->StartHarvest(human,{0,200,0});
        const auto frame=f.Ready(human);
        f.world->TryEdit({World::EditOperation::Remove,{0,200,0}});
        f.world->TryEdit({World::EditOperation::Set,{0,200,0},6});
        f.session->CommitFrame(*f.world,frame);
        Require(f.session->InspectNpc(human).resources.empty() && f.world->QueryBlock({0,200,0}).block.block_id==6,
                "Old action consumed a replacement block");
        f.session->StartHarvest(human,{0,200,0});
        const auto ready=f.Ready(human);
        f.session->RemoveNpc(human);
        // Batch owner remains valid, but the removed entity has no eligible action.
        f.session->CommitFrame(*f.world,ready);
        Require(f.world->QueryBlock({0,200,0}).block.block_id==6, "Removed NPC still consumed target");
    }
    void CreditPreparation(const World::BlockDefinition& definition) {
        Fixture f(definition);
        const auto first=f.Add(1), second=f.Add(2);
        Detail::NpcDiagnosticsAdapter::SetResource(*f.session,first,"wood",(std::numeric_limits<std::uint64_t>::max)());
        f.session->StartHarvest(first,{0,200,0});
        f.session->StartHarvest(second,{0,200,0});
        auto frame=f.Ready(first);
        f.session->CommitFrame(*f.world,frame);
        Require(f.session->InspectNpc(first).action.failure==ActionFailure::CreditOverflow, "Counter overflow was not rejected");
        Require(f.session->InspectNpc(second).action.phase==ActionPhase::Succeeded, "Recoverable failure blocked later contender");
        Fixture allocation(definition);
        const auto actor=allocation.Add();
        allocation.session->StartHarvest(actor,{0,200,0});
        frame=allocation.Ready(actor);
        fail_next_allocation=true;
        allocation.session->CommitFrame(*allocation.world,frame);
        Require(!fail_next_allocation, "Preparation allocation fault was not reached");
        Require(allocation.session->InspectNpc(actor).action.failure==ActionFailure::CreditPreparation &&
                allocation.world->QueryBlock({0,200,0}).block.block_id==6, "Allocation failure consumed the block");
    }
    void PostRemovalFailure(const World::BlockDefinition& definition) {
        Fixture f(definition);
        const auto human=f.Add();
        f.session->StartHarvest(human,{0,200,0});
        const auto frame=f.Ready(human);
        Detail::NpcDiagnosticsAdapter::FailAfterRemoveOnce(*f.session);
        try { f.session->CommitFrame(*f.world,frame); throw std::runtime_error("Missing commit fault"); }
        catch (const HarvestCommitFailed& failure) {
            Require(failure.Stage()==HarvestCommitStage::Credit && failure.WorldChanged() && !failure.ResourceCredited(),
                    "Partial commit facts are incorrect");
        }
        Require(f.session->State()==SessionState::Failed && f.world->QueryBlock({0,200,0}).block.block_id==1,
                "Post-removal fault did not stop session or incorrectly restored target");
        Throws<Rejected>([&] { f.Advance(); });
    }
    void PlayerPriority(const World::BlockDefinition& definition) {
        Fixture f(definition);
        const auto human=f.Add();
        f.session->StartHarvest(human,{0,200,0});
        for (unsigned frame=0;frame<30;++frame) {
            const auto prior=f.session->InspectNpc(human).action;
            AdvanceInput intent;
            intent.remove=prior.phase==ActionPhase::Harvesting && prior.elapsed_seconds+.10001>=prior.duration_seconds;
            const auto result=f.session->Advance(.1,intent);
            f.session->CommitFrame(*f.world,result);
            if (intent.remove) {
                Require(result.edit.has_value(), "Player priority fixture did not hit target");
                Require(f.session->InspectNpc(human).action.phase==ActionPhase::Failed && f.session->InspectNpc(human).resources.empty(),
                        "NPC submitted before ordinary player edit");
                return;
            }
        }
        throw std::runtime_error("Player priority fixture did not finish");
    }
    void RealMovementAndBlockage(const World::BlockDefinition& definition) {
        Fixture moving(definition);
        const auto human=moving.Add(1,{-3.5f,200.9f,.5f});
        moving.session->StartHarvest(human,{0,200,0});
        moving.Step();
        const auto early=moving.session->InspectNpc(human);
        Require(early.position.x>-3.5f && early.position.x<-.5f, "NPC did not move by physical substeps");
        const auto frame=moving.Ready(human);
        moving.session->CommitFrame(*moving.world,frame);
        Require(moving.session->InspectNpc(human).action.phase==ActionPhase::Succeeded,"Real movement did not reach harvest");
        Fixture blocked(definition);
        for(int z=-6;z<=6;++z) for(int y=200;y<203;++y) blocked.world->TryEdit({World::EditOperation::Set,{-2,y,z},5});
        const auto actor=blocked.Add(1,{-3.5f,200.9f,.5f});
        blocked.session->StartHarvest(actor,{0,200,0});
        for(unsigned i=0;i<70 && blocked.session->InspectNpc(actor).action.phase!=ActionPhase::Failed;++i) blocked.Step();
        Require(blocked.session->InspectNpc(actor).action.failure==ActionFailure::Blocked,"Terrain obstruction did not terminate movement");
    }
    void OwnedDisplayFacts(const World::BlockDefinition& definition) {
        Fixture f(definition);
        const auto human = f.Add(1, {-3.5f, 200.9f, .5f});
        const auto action = f.session->StartHarvest(human, {0, 200, 0});
        const auto initial = f.session->NpcDisplays().front();
        Require(initial.session_id != 0 && initial.advance_id == 0 && initial.action.id == action &&
                initial.horizontal_distance == 0 && initial.resources.empty(), "Initial display facts invalid");
        f.Step();
        const auto moving = f.session->NpcDisplays().front();
        const auto repeated = f.session->NpcDisplays().front();
        Require(moving.session_id == initial.session_id && moving.advance_id > initial.advance_id &&
                moving.horizontal_distance > 0 && moving.horizontal_distance == repeated.horizontal_distance &&
                moving.action.elapsed_seconds == repeated.action.elapsed_seconds,
                "Display reads consumed motion or changed action facts");
        f.session->Pause();
        const auto paused = f.session->NpcDisplays().front();
        Require(paused.advance_id == moving.advance_id && paused.position == moving.position &&
                paused.horizontal_distance == moving.horizontal_distance, "Pause changed pose facts");
        f.session->Resume();
        f.Step(5);
        Require(f.session->NpcDisplays().front().horizontal_distance == moving.horizontal_distance,
                "Resume caught up paused movement");
        const auto frame = f.Ready(human);
        auto ready = f.session->NpcDisplays().front();
        Require(ready.advance_id == frame.frame_id && ready.session_id == frame.frame_owner &&
                ready.action.phase == ActionPhase::Ready && ready.resources.empty(),
                "Ready display claimed credit before commit");
        f.session->CommitFrame(*f.world, frame);
        const auto completed = f.session->NpcDisplays().front();
        Require(completed.action.phase == ActionPhase::Succeeded && completed.resources.size() == 1 &&
                completed.resources.front().quantity == 2 && ready.resources.empty() &&
                ready.action.phase == ActionPhase::Ready, "Display result was aliased or not committed");
        ready.resources.push_back({"fake", 999});
        f.Step();
        const auto idle = f.session->NpcDisplays().front();
        Require(idle.horizontal_distance == completed.horizontal_distance && idle.resources.front().quantity == 2 &&
                idle.resources.size() == 1, "Display copy mutated production state or idle motion");
        Fixture next(definition);
        next.Add();
        Require(next.session->NpcDisplays().front().session_id != initial.session_id,
                "Fresh run reused display session identity");
    }
    void WorkPositionIsRevalidatedAtCommit(const World::BlockDefinition& definition) {
        for (bool remove_support : {true, false}) {
            Fixture f(definition);
            const auto human = f.Add();
            f.session->StartHarvest(human, {0, 200, 0});
            const auto frame = f.Ready(human);
            // A player edit is allowed after physics, before NPC commit. The
            // cached grounded flag must not authorize work against changed terrain.
            if (remove_support) f.world->TryEdit({World::EditOperation::Remove, {-1, 199, 0}});
            else f.world->TryEdit({World::EditOperation::Set, {-1, 200, 0}, 5});
            f.session->CommitFrame(*f.world, frame);
            const auto result = f.session->InspectNpc(human);
            Require(result.action.phase == ActionPhase::Failed && result.action.failure == ActionFailure::Interrupted,
                    "Commit trusted a stale grounded or clear-space fact");
            Require(result.resources.empty() && f.world->QueryBlock({0, 200, 0}).block.block_id == 6,
                    "Invalid work position consumed target or credited resources");
        }
    }
}

void* operator new(std::size_t size) {
    if (fail_next_allocation) { fail_next_allocation=false; throw std::bad_alloc(); }
    if (void* value=std::malloc(size ? size : 1)) return value;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value,std::size_t) noexcept { std::free(value); }
void operator delete[](void* value,std::size_t) noexcept { std::free(value); }

int main(int argc,char** argv) {
    try {
        Require(argc==2,"Expected block definition path");
        std::ifstream file(argv[1],std::ios::binary);
        std::string text((std::istreambuf_iterator<char>(file)),{});
        const auto place=text.find("oak_log:");
        Require(place!=std::string::npos,"Missing oak log fixture");
        text.insert(place+8,"\n  harvest: {seconds: 0.5, resource: wood, quantity: 2}");
        const auto definition=World::BlockDefinition::FromConfig(text);
        AssemblyAndIsolation(definition);
        TimingAndPause(definition);
        OrderedContest(definition);
        TargetReplacementAndRemoval(definition);
        CreditPreparation(definition);
        PostRemovalFailure(definition);
        PlayerPriority(definition);
        RealMovementAndBlockage(definition);
        WorkPositionIsRevalidatedAtCommit(definition);
        OwnedDisplayFacts(definition);
        std::cout<<"NPC action contracts: 10 groups passed\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"NPC action contracts failed: "<<error.what()<<'\n';
        return 1;
    }
}
