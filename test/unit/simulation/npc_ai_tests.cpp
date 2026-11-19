#include <symocraft/simulation/npc_ai.h>
#include <symocraft/simulation/session.h>
#include <symocraft/simulation/component.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
    using namespace SymoCraft;
    using namespace SymoCraft::Simulation;

    void Require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    template <class Callable>
    void Reject(Callable&& call, const char* message) {
        try { call(); } catch (const std::exception&) { return; }
        throw std::runtime_error(message);
    }
    struct FakeActions {
        ActionSnapshot action;
        unsigned starts{}, cancellations{};
        std::vector<ResourceCount> resources;
        NpcActionGateway Gateway() {
            return {
                [this](World::BlockCoord target) {
                    ++starts;
                    action = {starts, ActionPhase::Harvesting, ActionFailure::None, target, 0.0, 1.0};
                    return action.id;
                },
                [this](ActionId id) { Require(id == action.id, "poll identity"); return action; },
                [this](ActionId id) { Require(id == action.id, "cancel identity"); ++cancellations; action.phase = ActionPhase::Cancelled; },
                [this] { NpcSnapshot state; state.action = action; state.resources = resources; return state; }
            };
        }
    };
    const std::string human = R"(<root BTCPP_format="4"><BehaviorTree ID="Human"><Sequence><ForceSuccess><Harvest target_x="{target_x}" target_y="{target_y}" target_z="{target_z}"/></ForceSuccess><Idle name="final_idle"/></Sequence></BehaviorTree></root>)";

    void IsolationAndTerminalState() {
        FakeActions a, b;
        auto left = NpcBrain::FromText(human, "Human", {}, {1, 200, 2}, a.Gateway());
        auto right = NpcBrain::FromText(human, "Human", {}, {3, 201, 4}, b.Gateway());
        left->Tick(0);
        Require(a.starts == 0, "zero rule time started tree");
        left->Tick(.02);
        right->Tick(.02);
        Require(a.starts == 1 && b.starts == 1, "independent start");
        Require(a.action.target == World::BlockCoord(1, 200, 2), "left target");
        Require(b.action.target == World::BlockCoord(3, 201, 4), "right target");
        a.action.phase = ActionPhase::Succeeded;
        a.resources.push_back({"wood", 2});
        left->Tick(.02);
        Require(left->Inspect().idling, "success did not enter Idle");
        Require(right->Inspect().npc.action.phase == ActionPhase::Harvesting, "tree states leaked");
        b.action.phase = ActionPhase::Failed;
        b.action.failure = ActionFailure::InvalidTarget;
        right->Tick(.02);
        Require(right->Inspect().idling, "failure did not enter Idle");
        const auto saved = left->Inspect();
        for (unsigned i = 0; i < 5; ++i) { left->Tick(.02); right->Tick(.02); }
        Require(a.starts == 1 && b.starts == 1, "terminal action restarted");
        Require(saved.npc.resources.size() == 1 && saved.npc.resources.front().quantity == 2, "owned snapshot invalid");
        const auto transitions = left->DrainTransitions();
        Require(!transitions.empty(), "no state transitions");
        left->Tick(.02);
        Require(left->DrainTransitions().empty(), "unchanged tree spammed transitions");
        left->Halt();
        right->Halt();
        Require(!left->Inspect().idling && !right->Inspect().idling, "halted brain still reported running Idle");
        Require(a.cancellations == 0 && b.cancellations == 0, "terminal actions cancelled again");
    }

    void HaltAndValidation() {
        FakeActions a;
        auto running = NpcBrain::FromText(human, "Human", {}, {1, 200, 2}, a.Gateway());
        running->Tick(.02);
        running->Halt(); running->Halt();
        Require(a.cancellations == 1, "Halt not single-shot");
        Reject([&] { running->Tick(.02); }, "halted tree resumed");
        for (const auto value : {-1.0, .2, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
            Reject([&] { running->Tick(value); }, "invalid AI time accepted");
        const auto tree = [&](const std::string& node) {
            return "<root BTCPP_format=\"4\"><BehaviorTree ID=\"Main\">" + node + "</BehaviorTree></root>";
        };
        for (const auto& node : {std::string("<Unknown/>"), std::string("<Sleep msec=\"1\"/>"),
            std::string("<Harvest target_x=\"bad\" target_y=\"200\" target_z=\"0\"/>"),
            std::string("<Harvest target_x=\"0\" target_y=\"256\" target_z=\"0\"/>"),
            std::string("<SubTree ID=\"Missing\"/>"), std::string("<SubTree ID=\"Main\"/>"),
            std::string("<Script code=\"foo:=1 &amp; 3\"/>"), std::string("<Idle _onSuccess=\"@@@\"/>"),
            std::string("<Script code=\"foo:=1 .. 3\"/>"), std::string("<Script code=\"foo:=true ? 1 : 2\"/>"),
            std::string("<Idle unknown=\"1\"/>")}) {
            Reject([&] { static_cast<void>(NpcBrain::FromText(tree(node), "Main", {}, {0, 200, 0}, a.Gateway())); },
                "bad tree asset accepted");
        }
        try {
            static_cast<void>(NpcBrain::FromText(tree("<Harvest target_x=\"bad\" target_y=\"200\" target_z=\"0\"/>"),
                "Main", {}, {0, 200, 0}, a.Gateway()));
            throw std::runtime_error("invalid literal accepted");
        } catch (const std::invalid_argument& error) {
            Require(std::string(error.what()).find("Harvest.target_x") != std::string::npos,
                "invalid parameter diagnostic omitted node and port");
        }
        Reject([&] { static_cast<void>(NpcBrain::FromText(tree("<Idle/>"), "Other", {}, {}, a.Gateway())); }, "missing entry accepted");
        std::string deep = "<Idle/>";
        for (unsigned depth = 0; depth < 33; ++depth) deep = "<Inverter>" + deep + "</Inverter>";
        Reject([&] { static_cast<void>(NpcBrain::FromText(tree(deep), "Main", {}, {}, a.Gateway())); }, "depth budget ignored");
        std::string wide = "<Sequence>";
        for (unsigned count = 0; count < 256; ++count) wide += "<AlwaysSuccess/>";
        wide += "</Sequence>";
        Reject([&] { static_cast<void>(NpcBrain::FromText(tree(wide), "Main", {}, {}, a.Gateway())); }, "node budget ignored");
        const std::string deep_script = "<Script code=\"foo:=" + std::string(33, '(') + "1" + std::string(33, ')') + "\"/>";
        Reject([&] { static_cast<void>(NpcBrain::FromText(tree(deep_script), "Main", {}, {}, a.Gateway())); },
            "script recursion budget ignored");
        const std::string unary_script = "<Script code=\"foo:=" + std::string(33, '!') + "true\"/>";
        Reject([&] { static_cast<void>(NpcBrain::FromText(tree(unary_script), "Main", {}, {}, a.Gateway())); },
            "script prefix recursion budget ignored");
        Require(a.starts == 1, "loading invalid assets performed actions");
        const std::string script = "<Sequence><Script code=\"foo:=1; foo += 2\"/><ScriptCondition code=\"foo == 3\"/><Idle/></Sequence>";
        auto scripted = NpcBrain::FromText(tree(script), "Main", {}, {}, a.Gateway());
        scripted->Tick(.02);
        Require(scripted->Inspect().status == "RUNNING", "built-in arithmetic/comparison failed");
        const std::string subtree = "<root BTCPP_format=\"4\"><BehaviorTree ID=\"Main\"><SubTree ID=\"Child\"/></BehaviorTree><BehaviorTree ID=\"Child\"><Idle/></BehaviorTree></root>";
        auto child = NpcBrain::FromText(subtree, "Main", {}, {}, a.Gateway());
        child->Tick(.02);
        Require(child->Inspect().status == "RUNNING", "valid subtree failed");
    }
    void AssetFiles(const std::filesystem::path& root) {
        FakeActions a, b;
        auto human_tree = NpcBrain::FromFile(root / "ai/human_harvest.xml", "HumanHarvest", {}, {1, 200, 0}, a.Gateway());
        auto zombie = NpcBrain::FromFile(root / "ai/zombie_idle.xml", "ZombieIdle", {}, {}, b.Gateway());
        human_tree->Tick(.02); zombie->Tick(.02);
        Require(a.starts == 1 && b.starts == 0, "production trees did not bind actions");
        const auto model = NpcBrain::NodeModelsXml();
        Require(model.find("ID=\"Harvest\"") != std::string::npos && model.find("target_y") != std::string::npos &&
            model.find("ID=\"Idle\"") != std::string::npos, "editor node model missing");
        human_tree->Halt(); zombie->Halt();
    }
    void AbilityRegistration() {
        FakeActions fake;
        unsigned starts{}, polls{};
        AiAbilityDefinition capability{"Count", {{"amount", AiScalarType::Integer, "Validated amount"}},
            [&](const std::map<std::string, std::string>& inputs, bool starting) {
                Require(inputs.at("amount") == "7", "extension parameter");
                if (starting) { ++starts; return AiAbilityStatus::Running; }
                ++polls;
                return AiAbilityStatus::Succeeded;
            }, [] {}};
        const std::string xml = "<root BTCPP_format=\"4\"><BehaviorTree ID=\"Extension\"><Sequence><Count amount=\"7\"/><Idle/></Sequence></BehaviorTree></root>";
        auto extension = NpcBrain::FromText(xml, "Extension", {}, {}, fake.Gateway(), {capability});
        extension->Tick(.01);
        Require(extension->Inspect().current_node.find("Count") != std::string::npos, "extension diagnosis");
        extension->Tick(.01);
        Require(starts == 1 && polls == 1, "extension did not execute");
        const auto model = NpcBrain::NodeModelsXml({capability});
        Require(model.find("ID=\"Count\"") != std::string::npos && model.find("amount") != std::string::npos,
            "extension editor model");
        const auto invalid = xml.substr(0, xml.find("amount=\"7\"")) + "amount=\"bad\"" +
            xml.substr(xml.find("amount=\"7\"") + std::string("amount=\"7\"").size());
        Reject([&] { static_cast<void>(NpcBrain::FromText(invalid, "Extension", {}, {}, fake.Gateway(), {capability})); },
            "extension invalid parameter accepted");
        extension->Halt();
    }
    void IdleSemanticState() {
        FakeActions fake;
        for (const std::string name : {"idle_after_job", "settled_idle", ""}) {
            const auto attribute = name.empty() ? std::string{} : " name=\"" + name + "\"";
            auto brain = NpcBrain::FromText("<root BTCPP_format=\"4\"><BehaviorTree ID=\"Main\"><Idle" +
                attribute + "/></BehaviorTree></root>", "Main", {}, {}, fake.Gateway());
            Require(!brain->Inspect().idling, "unticked Idle reported running");
            brain->Tick(.02);
            Require(brain->Inspect().idling, "Idle semantics depended on its display name");
            brain->Halt();
            Require(!brain->Inspect().idling, "halt retained running Idle semantics");
        }
        // An unrelated running action must not impersonate Idle through its name.
        AiAbilityDefinition work{"Work", {},
            [](const auto&, bool) { return AiAbilityStatus::Running; }, [] {}};
        const auto tree = [&](const std::string& node) {
            return "<root BTCPP_format=\"4\"><BehaviorTree ID=\"Main\">" + node + "</BehaviorTree></root>";
        };
        auto misleading = NpcBrain::FromText(tree("<Work name=\"idle_after_job\"/>"),
            "Main", {}, {}, fake.Gateway(), {work});
        misleading->Tick(.02);
        Require(!misleading->Inspect().idling, "display name impersonated Idle semantics");
    }
    void ProductionActionBinding(const std::filesystem::path& root, bool contest, bool renamed_idle = false) {
        std::ifstream input(root / "configs/blockFormats.yaml", std::ios::binary);
        std::string definitions{std::istreambuf_iterator<char>(input), {}};
        const auto log = definitions.find("oak_log:");
        Require(log != std::string::npos, "Missing production block fixture");
        definitions.insert(log + 8, "\n  harvest: {seconds: 0.5, resource: wood, quantity: 2}");
        auto world = World::VoxelWorld::Create(World::BlockDefinition::FromConfig(definitions), {424242, 2, false}, nullptr, true);
        for (int x = -6; x <= 6; ++x) for (int z = -6; z <= 6; ++z)
            world->TryEdit({World::EditOperation::Set, {x, 199, z}, 5});
        world->TryEdit({World::EditOperation::Set, {0, 200, 0}, 6});
        const World::BlockCoord second_target = contest ? World::BlockCoord{0, 200, 0} : World::BlockCoord{2, 200, 0};
        if (!contest) world->TryEdit({World::EditOperation::Set, second_target, 6});
        ECS::Registry registry;
        registry.RegisterComponent<Transform>("Transform");
        registry.RegisterComponent<PhysicsStepOrder>("PhysicsStepOrder");
        registry.RegisterComponent<Physics::RigidBody>("RigidBody");
        registry.RegisterComponent<Physics::HitBox>("HitBox");
        registry.RegisterComponent<Character::CharacterComponent>("Character");
        registry.RegisterComponent<Character::PlayerComponent>("Player");
        auto session = SimulationSession::Create(registry, *world, {{-2.5f, 200.9f, .5f}, 0, -20});
        NpcConfig config;
        config.position = {-.5f, 200.9f, .5f};
        const auto first = session->AddNpc(config);
        config.order_key = 2;
        config.position = {contest ? 1.5f : 3.5f, 200.9f, .5f};
        const auto second = session->AddNpc(config);
        config.order_key = 3;
        config.kind = NpcKind::Zombie;
        const auto zombie = session->AddNpc(config);
        const auto create_human = [&](ECS::EntityId entity, World::BlockCoord target) {
            const auto path = root / "ai/human_harvest.xml";
            if (!renamed_idle) return NpcBrain::FromFile(path, "HumanHarvest", entity,
                target, SessionNpcGateway(*session, entity));
            std::ifstream source(path, std::ios::binary);
            std::string xml{std::istreambuf_iterator<char>(source), {}};
            const std::string original = "name=\"idle_after_job\"";
            const auto position = xml.find(original);
            Require(position != std::string::npos, "Missing Idle rename fixture");
            xml.replace(position, original.size(), "name=\"settled_idle\"");
            return NpcBrain::FromText(xml, "HumanHarvest", entity, target, SessionNpcGateway(*session, entity));
        };
        auto left = create_human(first, {0, 200, 0});
        auto right = create_human(second, second_target);
        auto idle = NpcBrain::FromFile(root / "ai/zombie_idle.xml", "ZombieIdle", zombie,
            {}, SessionNpcGateway(*session, zombie));
        for (unsigned frame = 0; frame < 120; ++frame) {
            left->Tick(static_cast<double>(.1f));
            right->Tick(static_cast<double>(.1f));
            idle->Tick(static_cast<double>(.1f));
            const auto result = session->Advance(.1, {});
            session->CommitFrame(*world, result);
        }
        const auto a = left->Inspect();
        const auto b = right->Inspect();
        Require(a.idling && b.idling, "production terminal brains did not stay Idle");
        Require(a.npc.action.phase == ActionPhase::Succeeded &&
            b.npc.action.phase == (contest ? ActionPhase::Failed : ActionPhase::Succeeded),
            "production scene did not preserve expected terminal outcomes");
        Require(a.npc.resources.size() == 1 && a.npc.resources[0].resource_id == "wood" &&
            a.npc.resources[0].quantity == 2,
            "production tree credited more than once or leaked resources");
        if (contest) Require(b.npc.resources.empty(), "contest loser received resources");
        else Require(b.npc.resources.size() == 1 && b.npc.resources[0].resource_id == "wood" &&
            b.npc.resources[0].quantity == 2, "independent NPC resources were not credited exactly once");
        Require(world->QueryBlock({0, 200, 0}).block.block_id == 1 &&
            world->QueryBlock(second_target).block.block_id == 1,
            "production scene left a successfully harvested target in the world");
        Require(idle->Inspect().npc.action.phase == ActionPhase::Idle, "production zombie started an action");
        left->Halt(); right->Halt(); idle->Halt();
    }
}

int main(int argc, char** argv) {
    try {
        if (argc != 2 && argc != 3) throw std::invalid_argument("Expected asset root and optional editor-model export path");
        IsolationAndTerminalState();
        HaltAndValidation();
        AssetFiles(argv[1]);
        AbilityRegistration();
        IdleSemanticState();
        ProductionActionBinding(argv[1], true);
        ProductionActionBinding(argv[1], false);
        ProductionActionBinding(argv[1], true, true);
        ProductionActionBinding(argv[1], false, true);
        if (argc == 3) {
            std::ofstream output(argv[2], std::ios::binary);
            output << NpcBrain::NodeModelsXml();
            if (!output) throw std::runtime_error("Could not export editor models");
        }
        std::cout << "NPC AI isolated state, terminal behavior, assets, scripts, cancellation and editor model passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "NPC AI: " << error.what() << '\n';
        return 1;
    }
}
