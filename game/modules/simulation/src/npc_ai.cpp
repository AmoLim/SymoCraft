#include <symocraft/simulation/npc_ai.h>
#include <symocraft/simulation/session.h>
#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/scripting/any_types.hpp>
#include <behaviortree_cpp/scripting/script_parser.hpp>
#include <behaviortree_cpp/xml_parsing.h>
#include <tinyxml2.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace SymoCraft::Simulation {
    namespace {
        constexpr std::size_t max_asset_bytes = 65536;
        constexpr std::size_t max_tree_nodes = 256;
        constexpr unsigned max_tree_depth = 32;
        constexpr std::size_t max_transitions = 1024;

        struct ActionBinding {
            NpcActionGateway gateway;
            bool callbacks_enabled = true;
        };

        class HarvestNode final : public BT::StatefulActionNode {
        public:
            HarvestNode(const std::string& name, const BT::NodeConfig& config, ActionBinding* binding)
                : StatefulActionNode(name, config), binding_(*binding) {}
            static BT::PortsList providedPorts() {
                return {BT::InputPort<int>("target_x", "Target block X"),
                    BT::InputPort<int>("target_y", "Target block Y"),
                    BT::InputPort<int>("target_z", "Target block Z")};
            }
            BT::NodeStatus onStart() override {
                action_ = binding_.gateway.start(Target());
                return Poll();
            }
            BT::NodeStatus onRunning() override { return Poll(); }
            void onHalted() override {
                if (action_ != 0 && binding_.callbacks_enabled) binding_.gateway.cancel(action_);
                action_ = 0;
            }
            World::BlockCoord Target() const {
                const auto x = getInput<int>("target_x");
                const auto y = getInput<int>("target_y");
                const auto z = getInput<int>("target_z");
                if (!x || !y || !z) throw std::invalid_argument("Harvest target ports require three integer values");
                if (y.value() < 0 || y.value() >= 256) throw std::invalid_argument("Harvest target_y must be in 0..255");
                return {x.value(), y.value(), z.value()};
            }
        private:
            BT::NodeStatus Poll() {
                const auto state = binding_.gateway.inspect(action_);
                if (state.id != action_) throw std::runtime_error("Harvest action identity no longer matches");
                if (state.phase == ActionPhase::Succeeded) return BT::NodeStatus::SUCCESS;
                if (state.phase == ActionPhase::Failed || state.phase == ActionPhase::Cancelled)
                    return BT::NodeStatus::FAILURE;
                return BT::NodeStatus::RUNNING;
            }
            ActionBinding& binding_;
            ActionId action_{};
        };

        class IdleNode final : public BT::StatefulActionNode {
        public:
            using StatefulActionNode::StatefulActionNode;
            static BT::PortsList providedPorts() { return {}; }
            BT::NodeStatus onStart() override { return BT::NodeStatus::RUNNING; }
            BT::NodeStatus onRunning() override { return BT::NodeStatus::RUNNING; }
            void onHalted() override {}
        };

        class AbilityNode final : public BT::StatefulActionNode {
        public:
            AbilityNode(const std::string& name, const BT::NodeConfig& config,
                const AiAbilityDefinition& definition, bool& callbacks_enabled)
                : StatefulActionNode(name, config), definition_(definition), callbacks_enabled_(callbacks_enabled) {}
            BT::NodeStatus onStart() override { return Invoke(true); }
            BT::NodeStatus onRunning() override { return Invoke(false); }
            void onHalted() override { if (callbacks_enabled_) definition_.halt(); }
            void CheckInputs() const { static_cast<void>(ReadInputs()); }
        private:
            std::map<std::string, std::string> ReadInputs() const {
                std::map<std::string, std::string> result;
                for (const auto& port : definition_.inputs) {
                    const auto value = getInput<std::string>(port.name);
                    if (!value) throw std::invalid_argument("Missing ability port: " + definition_.name + "." + port.name);
                    switch (port.type) {
                    case AiScalarType::Integer: static_cast<void>(BT::convertFromString<int>(value.value())); break;
                    case AiScalarType::Number:
                        if (!std::isfinite(BT::convertFromString<double>(value.value())))
                            throw std::invalid_argument("Non-finite ability port: " + port.name);
                        break;
                    case AiScalarType::Boolean: static_cast<void>(BT::convertFromString<bool>(value.value())); break;
                    case AiScalarType::Text: break;
                    }
                    result.emplace(port.name, value.value());
                }
                return result;
            }
            BT::NodeStatus Invoke(bool starting) {
                switch (definition_.tick(ReadInputs(), starting)) {
                case AiAbilityStatus::Running: return BT::NodeStatus::RUNNING;
                case AiAbilityStatus::Succeeded: return BT::NodeStatus::SUCCESS;
                case AiAbilityStatus::Failed: return BT::NodeStatus::FAILURE;
                }
                throw std::runtime_error("Ability returned an invalid state");
            }
            const AiAbilityDefinition& definition_;
            bool& callbacks_enabled_;
        };

        void RegisterNodes(BT::BehaviorTreeFactory& factory, ActionBinding& binding) {
            factory.registerNodeType<HarvestNode>("Harvest", &binding);
            factory.registerNodeType<IdleNode>("Idle");
        }

        void RegisterAbilities(BT::BehaviorTreeFactory& factory,
            std::vector<AiAbilityDefinition>& definitions, bool& callbacks_enabled) {
            for (auto& definition : definitions) {
                if (definition.name.empty() || !definition.tick || !definition.halt)
                    throw std::invalid_argument("Ability registration requires a name, tick and halt");
                BT::PortsList ports;
                for (const auto& input : definition.inputs) {
                    BT::PortInfo info;
                    switch (input.type) {
                    case AiScalarType::Integer: info = BT::InputPort<int>(input.name, input.description).second; break;
                    case AiScalarType::Number: info = BT::InputPort<double>(input.name, input.description).second; break;
                    case AiScalarType::Boolean: info = BT::InputPort<bool>(input.name, input.description).second; break;
                    case AiScalarType::Text: info = BT::InputPort<std::string>(input.name, input.description).second; break;
                    default: throw std::invalid_argument("Unknown ability port type");
                    }
                    if (!ports.emplace(input.name, info).second) throw std::invalid_argument("Duplicate ability port");
                }
                const BT::TreeNodeManifest manifest{BT::NodeType::ACTION, definition.name, ports, {}};
                factory.registerBuilder(manifest, [&definition, &callbacks_enabled](const std::string& name, const BT::NodeConfig& config) {
                    return std::make_unique<AbilityNode>(name, config, definition, callbacks_enabled);
                });
            }
        }

        void ValidateExpression(std::string_view source) {
            // Built-in scripts have no calls or gameplay object bindings. Keep the
            // authored subset to scalar arithmetic/comparison/logical assignment.
            if (source.empty() || source.size() > 1024)
                throw std::invalid_argument("Script is empty or exceeds 1024 bytes");
            for (std::size_t i = 0; i < source.size(); ++i) {
                const char c = source[i];
                if (c == '@' || c == '[' || c == ']' || c == '"' || c == '\'' || c == '^' || c == '~')
                    throw std::invalid_argument("Script contains unsupported operator or value");
                if ((c == '&' || c == '|') && (i + 1 == source.size() || source[i + 1] != c))
                    throw std::invalid_argument("Script bitwise operators are unsupported");
                if (c == '&' || c == '|') ++i;
            }
            const std::string script(source);
            const auto tokens = BT::Scripting::tokenize(script);
            if (tokens.size() > 256) throw std::invalid_argument("Script exceeds 256 tokens");
            unsigned depth = 0, prefixes = 0;
            for (const auto& token : tokens) {
                using Type = BT::Scripting::TokenType;
                switch (token.type) {
                case Type::String: case Type::DotDot: case Type::Ampersand: case Type::Pipe:
                case Type::Caret: case Type::Tilde: case Type::Question: case Type::Colon:
                case Type::Error:
                    throw std::invalid_argument("Script contains unsupported operator or value");
                default: break;
                }
                if (token.type == Type::LeftParen && ++depth > 32)
                    throw std::invalid_argument("Script exceeds 32 nested parentheses");
                if (token.type == Type::RightParen) {
                    if (depth == 0) throw std::invalid_argument("Script has unmatched parentheses");
                    --depth;
                }
                if (token.type == Type::Bang || token.type == Type::Minus) {
                    if (++prefixes > 32) throw std::invalid_argument("Script exceeds 32 prefix operators");
                } else { prefixes = 0; }
            }
            const auto valid = BT::ValidateScript(script);
            if (!valid) throw std::invalid_argument("Invalid script: " + valid.error());
        }

        const std::set<std::string>& AllowedNodes() {
            static const std::set<std::string> allowed{
                "Sequence", "Fallback", "ReactiveSequence", "ReactiveFallback", "IfThenElse",
                "ForceSuccess", "ForceFailure", "Inverter", "RunOnce", "Script", "ScriptCondition",
                "SubTree", "Harvest", "Idle", "AlwaysSuccess", "AlwaysFailure"};
            return allowed;
        }

        std::string NodeId(const tinyxml2::XMLElement& element) {
            const std::string tag = element.Name();
            if (tag == "Action" || tag == "Condition" || tag == "Decorator" || tag == "Control") {
                const char* id = element.Attribute("ID");
                if (!id) throw std::invalid_argument("Node is missing ID");
                return id;
            }
            return tag;
        }

        void ValidateNode(const tinyxml2::XMLElement& element,
            const std::map<std::string, const tinyxml2::XMLElement*>& trees,
            const BT::BehaviorTreeFactory& factory, std::set<std::string>& stack,
            std::size_t& count, unsigned depth) {
            if (++count > max_tree_nodes || depth > max_tree_depth)
                throw std::invalid_argument("Tree exceeds node or depth budget");
            const auto id = NodeId(element);
            if (!AllowedNodes().contains(id) && factory.builtinNodes().contains(id))
                throw std::invalid_argument("Unsupported node: " + id);
            const auto manifest = factory.manifests().find(id);
            if (manifest == factory.manifests().end()) throw std::invalid_argument("Unregistered node: " + id);
            for (auto attr = element.FirstAttribute(); attr; attr = attr->Next()) {
                const std::string name = attr->Name();
                const std::string value = attr->Value();
                if (name == "name" || name == "ID") continue;
                if (name == "_successIf" || name == "_failureIf" || name == "_skipIf" ||
                    name == "_while" || name == "_onSuccess" || name == "_onFailure" ||
                    name == "_post" || name == "_onHalted") {
                    try { ValidateExpression(value); }
                    catch (const std::exception& error) {
                        throw std::invalid_argument("Invalid attribute " + id + "." + name + ": " + error.what());
                    }
                    continue;
                }
                if (id == "SubTree") {
                    if (name.starts_with('_')) throw std::invalid_argument("Unsupported subtree attribute: " + name);
                    continue;
                }
                const auto port = manifest->second.ports.find(name);
                if (port == manifest->second.ports.end()) throw std::invalid_argument("Unknown port " + id + "." + name);
                try {
                    if ((id == "Script" || id == "ScriptCondition") && name == "code") {
                        ValidateExpression(value);
                    } else if (!BT::TreeNode::isBlackboardPointer(value) && port->second.converter()) {
                        static_cast<void>(port->second.parseString(value));
                    }
                } catch (const std::exception& error) {
                    throw std::invalid_argument("Invalid port " + id + "." + name + ": " + error.what());
                }
            }
            if (id == "SubTree") {
                const char* reference = element.Attribute("ID");
                if (!reference || !trees.contains(reference)) throw std::invalid_argument("Missing subtree reference");
                if (!stack.insert(reference).second) throw std::invalid_argument("Cyclic subtree reference: " + std::string(reference));
                const auto* child = trees.at(reference)->FirstChildElement();
                if (!child || child->NextSiblingElement()) throw std::invalid_argument("Subtree must have one root node");
                ValidateNode(*child, trees, factory, stack, count, depth + 1);
                stack.erase(reference);
            }
            for (auto child = element.FirstChildElement(); child; child = child->NextSiblingElement())
                ValidateNode(*child, trees, factory, stack, count, depth + 1);
        }

        void ValidateAsset(const std::string& xml, const std::string& main_id, const BT::BehaviorTreeFactory& factory) {
            if (xml.empty() || xml.size() > max_asset_bytes) throw std::invalid_argument("Tree asset exceeds 65536 bytes or is empty");
            tinyxml2::XMLDocument document;
            if (document.Parse(xml.data(), xml.size()) != tinyxml2::XML_SUCCESS)
                throw std::invalid_argument(std::string("Tree XML: ") + document.ErrorStr());
            const auto* root = document.RootElement();
            if (!root || std::string_view(root->Name()) != "root" || root->NextSiblingElement())
                throw std::invalid_argument("Tree asset needs one root element");
            std::map<std::string, const tinyxml2::XMLElement*> trees;
            for (auto tree = root->FirstChildElement(); tree; tree = tree->NextSiblingElement()) {
                const std::string_view kind = tree->Name();
                if (kind == "TreeNodesModel") continue;
                if (kind != "BehaviorTree") throw std::invalid_argument("Tree asset contains unsupported element: " + std::string(kind));
                const char* id = tree->Attribute("ID");
                if (!id || !trees.emplace(id, tree).second) throw std::invalid_argument("Missing or duplicate BehaviorTree ID");
            }
            if (!trees.contains(main_id)) throw std::invalid_argument("Tree entry does not exist: " + main_id);
            for (const auto& [id, tree] : trees) {
                const auto* first = tree->FirstChildElement();
                if (!first || first->NextSiblingElement()) throw std::invalid_argument("BehaviorTree requires exactly one root: " + id);
                std::size_t count = 0;
                std::set<std::string> stack{id};
                ValidateNode(*first, trees, factory, stack, count, 1);
            }
        }
    }

    struct NpcBrain::Impl {
        ECS::EntityId entity;
        ActionBinding binding;
        std::vector<AiAbilityDefinition> abilities;
        BT::BehaviorTreeFactory factory;
        BT::Tree tree;
        std::vector<BT::TreeNode::StatusChangeSubscriber> subscriptions;
        std::vector<AiNodeTransition> transitions;
        std::string current_node = "NotStarted";
        std::string status = "IDLE";
        std::uint64_t dropped{};
        bool halted = false;

        Impl(ECS::EntityId identity, NpcActionGateway gateway, std::vector<AiAbilityDefinition> extensions)
            : entity(identity), binding{std::move(gateway)}, abilities(std::move(extensions)) {
            if (!binding.gateway.start || !binding.gateway.inspect || !binding.gateway.cancel || !binding.gateway.snapshot)
                throw std::invalid_argument("NPC AI action gateway is incomplete");
            RegisterNodes(factory, binding);
            RegisterAbilities(factory, abilities, binding.callbacks_enabled);
            transitions.reserve(max_transitions);
        }
        ~Impl() noexcept {
            binding.callbacks_enabled = false;
            subscriptions.clear();
            try { tree.haltTree(); } catch (...) {}
        }
        void Load(const std::string& xml, const std::string& tree_id, World::BlockCoord target) {
            ValidateAsset(xml, tree_id, factory);
            auto blackboard = BT::Blackboard::create();
            blackboard->set<int>("target_x", target.x);
            blackboard->set<int>("target_y", target.y);
            blackboard->set<int>("target_z", target.z);
            factory.registerBehaviorTreeFromText(xml);
            tree = factory.createTree(tree_id, blackboard);
            tree.applyVisitor([&](BT::TreeNode* node) {
                if (auto* harvest = dynamic_cast<HarvestNode*>(node)) static_cast<void>(harvest->Target());
                if (auto* ability = dynamic_cast<AbilityNode*>(node)) ability->CheckInputs();
                subscriptions.push_back(node->subscribeToStatusChange(
                    [this](BT::TimePoint, const BT::TreeNode& changed, BT::NodeStatus before, BT::NodeStatus after) {
                        if (transitions.size() == max_transitions) {
                            transitions.erase(transitions.begin());
                            ++dropped;
                        }
                        transitions.push_back({changed.fullPath(), BT::toStr(before), BT::toStr(after)});
                        if (after == BT::NodeStatus::RUNNING && changed.type() == BT::NodeType::ACTION)
                            current_node = changed.fullPath();
                    }));
            });
        }
    };

    NpcBrain::NpcBrain(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
    NpcBrain::~NpcBrain() noexcept = default;
    std::unique_ptr<NpcBrain> NpcBrain::FromText(const std::string& xml, const std::string& tree_id,
        ECS::EntityId entity, World::BlockCoord target, NpcActionGateway gateway, std::vector<AiAbilityDefinition> extensions) {
        auto implementation = std::make_unique<Impl>(entity, std::move(gateway), std::move(extensions));
        implementation->Load(xml, tree_id, target);
        return std::unique_ptr<NpcBrain>(new NpcBrain(std::move(implementation)));
    }
    std::unique_ptr<NpcBrain> NpcBrain::FromFile(const std::filesystem::path& asset, const std::string& tree_id,
        ECS::EntityId entity, World::BlockCoord target, NpcActionGateway gateway, std::vector<AiAbilityDefinition> extensions) {
        try {
            const auto size = std::filesystem::file_size(asset);
            if (size == 0 || size > max_asset_bytes) throw std::invalid_argument("Tree asset size is invalid");
            std::ifstream stream(asset, std::ios::binary);
            if (!stream) throw std::runtime_error("Cannot open tree asset");
            std::string xml(static_cast<std::size_t>(size), '\0');
            stream.read(xml.data(), static_cast<std::streamsize>(xml.size()));
            if (!stream || stream.peek() != std::char_traits<char>::eof())
                throw std::runtime_error("Tree asset changed during load");
            return FromText(xml, tree_id, entity, target, std::move(gateway), std::move(extensions));
        } catch (const std::exception& error) {
            throw std::runtime_error("NPC tree asset " + asset.string() + ": " + error.what());
        }
    }
    std::string NpcBrain::NodeModelsXml(std::vector<AiAbilityDefinition> extensions) {
        ActionBinding unused;
        BT::BehaviorTreeFactory factory;
        RegisterNodes(factory, unused);
        RegisterAbilities(factory, extensions, unused.callbacks_enabled);
        return BT::writeTreeNodesModelXML(factory, false);
    }
    void NpcBrain::Tick(double effective_seconds) {
        if (!std::isfinite(effective_seconds) || effective_seconds < 0 || effective_seconds > static_cast<double>(0.1f))
            throw std::invalid_argument("NPC AI effective rule seconds must be finite and in 0..0.1");
        if (impl_->halted) throw std::logic_error("NPC AI has been halted");
        if (effective_seconds == 0) return;
        impl_->status = BT::toStr(impl_->tree.tickExactlyOnce());
    }
    void NpcBrain::Halt() {
        if (impl_->halted) return;
        impl_->tree.haltTree();
        impl_->halted = true;
        impl_->current_node = "Halted";
        impl_->status = "IDLE";
    }
    NpcAiSnapshot NpcBrain::Inspect() const {
        bool idle_running = false, other_action_running = false;
        std::as_const(impl_->tree).applyVisitor([&](const BT::TreeNode* node) {
            if (node->type() != BT::NodeType::ACTION || node->status() != BT::NodeStatus::RUNNING) return;
            if (dynamic_cast<const IdleNode*>(node)) idle_running = true;
            else other_action_running = true;
        });
        return {impl_->entity, impl_->current_node, impl_->status, impl_->binding.gateway.snapshot(), impl_->dropped,
                idle_running && !other_action_running};
    }
    std::vector<AiNodeTransition> NpcBrain::DrainTransitions() {
        auto result = std::move(impl_->transitions);
        impl_->transitions.clear();
        impl_->transitions.reserve(max_transitions);
        return result;
    }
    NpcActionGateway SessionNpcGateway(SimulationSession& session, ECS::EntityId entity) {
        return {
            [&session, entity](World::BlockCoord target) { return session.StartHarvest(entity, target); },
            [&session, entity](ActionId id) {
                const auto result = session.InspectNpc(entity).action;
                if (result.id != id) throw std::runtime_error("NPC action was replaced");
                return result;
            },
            [&session, entity](ActionId id) { session.CancelAction(entity, id); },
            [&session, entity] { return session.InspectNpc(entity); }
        };
    }
}
