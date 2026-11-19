#pragma once

#include <symocraft/simulation/npc.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <map>
#include <string>
#include <vector>

namespace SymoCraft::Simulation {
    // Each brain owns its tree and blackboard. The gateway borrows only an action
    // service and identity. It never provides the tree with Registry/World access.
    struct NpcActionGateway {
        std::function<ActionId(World::BlockCoord)> start;
        std::function<ActionSnapshot(ActionId)> inspect;
        std::function<void(ActionId)> cancel;
        std::function<NpcSnapshot()> snapshot;
    };
    struct AiNodeTransition {
        std::string node;
        std::string previous;
        std::string current;
    };
    struct NpcAiSnapshot {
        ECS::EntityId entity;
        std::string current_node;
        std::string status;
        NpcSnapshot npc;
        std::uint64_t dropped_transitions{};
        // At least one project Idle action is running, and no other action is
        // running. Derived from node types/statuses, never editable node names.
        bool idling = false;
    };
    enum class AiScalarType { Integer, Number, Boolean, Text };
    enum class AiAbilityStatus { Running, Succeeded, Failed };
    struct AiAbilityPort { std::string name; AiScalarType type; std::string description; };
    struct AiAbilityDefinition {
        std::string name;
        std::vector<AiAbilityPort> inputs;
        // Implementations route gameplay work through the validated action layer.
        // `starting` distinguishes the first call from polling a running ability.
        std::function<AiAbilityStatus(const std::map<std::string, std::string>&, bool starting)> tick;
        std::function<void()> halt;
    };

    NpcActionGateway SessionNpcGateway(SimulationSession& session, ECS::EntityId entity);

    class NpcBrain final {
    public:
        static std::unique_ptr<NpcBrain> FromFile(const std::filesystem::path& asset,
            const std::string& tree_id, ECS::EntityId entity, World::BlockCoord target,
            NpcActionGateway gateway, std::vector<AiAbilityDefinition> extensions = {});
        static std::unique_ptr<NpcBrain> FromText(const std::string& xml,
            const std::string& tree_id, ECS::EntityId entity, World::BlockCoord target,
            NpcActionGateway gateway, std::vector<AiAbilityDefinition> extensions = {});
        static std::string NodeModelsXml(std::vector<AiAbilityDefinition> extensions = {});
        ~NpcBrain() noexcept;
        NpcBrain(const NpcBrain&) = delete;
        NpcBrain& operator=(const NpcBrain&) = delete;

        // One bounded traversal per positive effective simulation interval.
        // Call before Advance and outside any World/resource commit boundary.
        void Tick(double effective_seconds);
        void Halt();
        NpcAiSnapshot Inspect() const;
        std::vector<AiNodeTransition> DrainTransitions();

    private:
        struct Impl;
        explicit NpcBrain(std::unique_ptr<Impl> impl);
        std::unique_ptr<Impl> impl_;
    };
}
