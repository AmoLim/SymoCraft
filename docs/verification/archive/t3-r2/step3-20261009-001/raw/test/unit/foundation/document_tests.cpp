#include "symocraft/foundation/document.h"
#include "symocraft/telemetry/document_io.h"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace { void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }
int main() {
    using namespace SymoCraft::Data;
    try {
        Value source;
        source["integer"] = std::numeric_limits<std::uint64_t>::max();
        source["float"] = 1.6f;
        source["real"] = 1.25;
        source["enabled"] = true;
        source["text"] = "texture_layer";
        source["numeric_text"] = "123";
        source["boolean_text"] = "true";
        source["items"].push_back(12);
        source["items"].push_back(42);
        source["items"].SetFlowStyle();
        source["empty"] = Value::Sequence{};
        auto copy = source;
        copy["items"][std::size_t{0}] = 99;
        Require(source["items"][std::size_t{0}].as<int>() == 12, "Document copies alias each other");
        const auto parsed = LoadYaml(DumpYaml(source));
        Require(parsed["integer"].as<std::uint64_t>() == std::numeric_limits<std::uint64_t>::max(), "Unsigned precision lost");
        Require(parsed["real"].as<double>() == 1.25 && parsed["enabled"].as<bool>(), "Scalar type lost");
        Require(parsed["text"].as<std::string>() == "texture_layer", "String changed");
        Require(parsed["numeric_text"].as<std::string>() == "123" &&
                parsed["boolean_text"].as<std::string>() == "true", "Ambiguous string changed type");
        Require(LoadYaml("value: !!str 123")["value"].as<std::string>() == "123", "Explicit string tag ignored");
        Require(LoadYaml("value: 'true'")["value"].as<std::string>() == "true", "Quoted string reinterpreted");
        Require(parsed["items"].size() == 2 && parsed["items"].FlowStyle(), "Sequence changed");
        Require(parsed["empty"].IsSequence() && parsed["empty"].size() == 0, "Empty collection changed");
        Require(!parsed["absent"].IsDefined(), "Missing value fabricated");
        bool rejected = false;
        try { source["enabled"].push_back(1); } catch (const std::invalid_argument&) { rejected = true; }
        Require(rejected, "Invalid document mutation accepted");
        std::cout << "Owned metadata and private YAML adapter contracts passed.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
