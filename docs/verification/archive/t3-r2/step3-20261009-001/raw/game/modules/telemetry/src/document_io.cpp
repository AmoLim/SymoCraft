#include "symocraft/telemetry/document_io.h"
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace SymoCraft::Data {
    namespace {
        YAML::Node Encode(const Value& value) {
            auto node = std::visit([](const auto& item) -> YAML::Node {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, std::monostate>) return YAML::Node{};
                else if constexpr (std::is_same_v<T, Value::Sequence>) {
                    YAML::Node result(YAML::NodeType::Sequence);
                    for (const auto& child : item) result.push_back(Encode(child));
                    return result;
                } else if constexpr (std::is_same_v<T, Value::Mapping>) {
                    YAML::Node result(YAML::NodeType::Map);
                    for (const auto& [name, child] : item) result[name] = Encode(child);
                    return result;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    YAML::Node result(item);
                    bool boolean;
                    std::int64_t signed_number;
                    std::uint64_t unsigned_number;
                    double real;
                    if (YAML::convert<bool>::decode(result, boolean) ||
                        YAML::convert<std::int64_t>::decode(result, signed_number) ||
                        YAML::convert<std::uint64_t>::decode(result, unsigned_number) ||
                        YAML::convert<double>::decode(result, real))
                        result.SetTag("tag:yaml.org,2002:str");
                    return result;
                } else return YAML::Node(item);
            }, value.storage());
            if (value.FlowStyle()) node.SetStyle(YAML::EmitterStyle::Flow);
            return node;
        }
        Value Decode(const YAML::Node& node) {
            Value value;
            if (node.IsMap()) {
                value = Value::Mapping{};
                for (const auto& item : node) value[item.first.as<std::string>()] = Decode(item.second);
            } else if (node.IsSequence()) {
                value = Value::Sequence{};
                for (const auto& item : node) value.push_back(Decode(item));
            } else if (node.IsScalar()) {
                // Explicitly quoted scalar strings must not be reinterpreted as numbers.
                if (node.Tag() == "!" || node.Tag() == "tag:yaml.org,2002:str") return node.Scalar();
                bool boolean;
                std::int64_t signed_number;
                std::uint64_t unsigned_number;
                double real;
                if (YAML::convert<bool>::decode(node, boolean)) value = boolean;
                else if (YAML::convert<std::int64_t>::decode(node, signed_number)) value = signed_number;
                else if (YAML::convert<std::uint64_t>::decode(node, unsigned_number)) value = unsigned_number;
                else if (YAML::convert<double>::decode(node, real)) value = real;
                else value = node.Scalar();
            }
            if (node.Style() == YAML::EmitterStyle::Flow) value.SetFlowStyle();
            return value;
        }
    }
    std::string DumpYaml(const Value& value) { return YAML::Dump(Encode(value)); }
    Value LoadYaml(std::string_view text) { return Decode(YAML::Load(std::string(text))); }
    Value LoadYamlFile(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot read document: " + path.string());
        return Decode(YAML::Load(file));
    }
}
