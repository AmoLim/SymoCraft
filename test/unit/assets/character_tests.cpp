#include <symocraft/assets/character.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string_view>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    struct TemporaryFile {
        std::filesystem::path path;
        ~TemporaryFile() { std::error_code error; std::filesystem::remove(path, error); }
    };
    void Check(const SymoCraft::CharacterMeshData& mesh) {
        Require(!mesh.vertices.empty() && mesh.vertices.size() % 3 == 0, "Missing complete character triangles");
        glm::vec3 minimum{100}, maximum{-100};
        for (const auto& vertex : mesh.vertices) {
            minimum = glm::min(minimum, vertex.position);
            maximum = glm::max(maximum, vertex.position);
            Require(std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y) &&
                std::isfinite(vertex.position.z), "Nonfinite imported position");
            Require(std::abs(glm::length(vertex.normal) - 1.0f) < 1e-4f, "Nonunit imported normal");
            Require(glm::all(glm::greaterThanEqual(vertex.color, glm::vec3{0})) &&
                glm::all(glm::lessThanEqual(vertex.color, glm::vec3{1})), "Invalid imported color");
        }
        for (std::size_t index = 0; index < mesh.vertices.size(); index += 3) {
            const auto& a = mesh.vertices[index];
            const auto& b = mesh.vertices[index + 1];
            const auto& c = mesh.vertices[index + 2];
            Require(glm::length(a.normal - b.normal) < 1e-4f && glm::length(a.normal - c.normal) < 1e-4f &&
                std::abs(glm::dot(a.normal, b.position - a.position)) < 1e-4f &&
                std::abs(glm::dot(a.normal, c.position - a.position)) < 1e-4f,
                "Blender hard face normals were lost");
        }
        Require(std::abs(minimum.y) < 1e-4f && maximum.y > 1.7f && maximum.y < 2.0f,
            "GLB must be Y-up, human scale and rooted at its feet");
        Require(minimum.x < -0.3f && maximum.x > 0.3f, "Character dimensions were lost");
        Require(std::any_of(mesh.vertices.begin(), mesh.vertices.end(), [&](const auto& vertex) {
            return glm::length(vertex.color - mesh.vertices.front().color) > 0.1f;
        }), "Distinct body-part vertex colors were lost");
    }
    std::string ReadFile(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }
    void Replace(std::string& text, const std::string& old_value, const std::string& new_value) {
        const auto offset = text.find(old_value);
        Require(offset != std::string::npos, "Expected fixture JSON field is missing");
        text.replace(offset, old_value.size(), new_value);
    }
    void Mutate(const std::filesystem::path& source, const std::filesystem::path& destination,
                const std::function<void(std::string&)>& edit) {
        const auto bytes = ReadFile(source);
        std::uint32_t json_length{};
        std::memcpy(&json_length, bytes.data() + 12, sizeof(json_length));
        Require(bytes.size() >= 20 + json_length, "Invalid fixture chunk layout");
        std::string json = bytes.substr(20, json_length);
        edit(json);
        while (json.size() % 4) json += ' ';
        std::string output = bytes.substr(0, 20) + json + bytes.substr(20 + json_length);
        const auto total = static_cast<std::uint32_t>(output.size());
        const auto revised_length = static_cast<std::uint32_t>(json.size());
        std::memcpy(output.data() + 8, &total, sizeof(total));
        std::memcpy(output.data() + 12, &revised_length, sizeof(revised_length));
        std::ofstream file(destination, std::ios::binary);
        file.write(output.data(), static_cast<std::streamsize>(output.size()));
    }
    void CheckParts(const SymoCraft::CharacterMeshData& mesh) {
        Require(mesh.parts.size() == 6, "Expected six owned rigid body parts");
        std::size_t count{};
        const auto world_transform = [&](int index) {
            auto result = mesh.parts[static_cast<std::size_t>(index)].local_transform;
            auto parent = mesh.parts[static_cast<std::size_t>(index)].parent;
            std::size_t depth{};
            while (parent >= 0) {
                Require(++depth < mesh.parts.size(), "Cyclic imported part hierarchy");
                result = mesh.parts[static_cast<std::size_t>(parent)].local_transform * result;
                parent = mesh.parts[static_cast<std::size_t>(parent)].parent;
            }
            return result;
        };
        glm::vec3 minimum{100}, maximum{-100};
        for (std::size_t index = 0; index < mesh.parts.size(); ++index) {
            const auto& part = mesh.parts[index];
            Require(!part.vertices.empty() && part.vertices.size() % 3 == 0,
                "Rigid part geometry was not retained");
            Require(part.pivot == glm::vec3(0) && part.rotation_axis == glm::vec3(1, 0, 0),
                "Explicit exported pivot/axis was lost");
            const auto world = world_transform(static_cast<int>(index));
            for (const auto& vertex : part.vertices) {
                const auto position = glm::vec3(world * glm::vec4(vertex.position, 1));
                minimum = glm::min(minimum, position); maximum = glm::max(maximum, position);
            }
            if (part.name == "left_arm") {
                Require(glm::length(glm::vec3(world[3]) - glm::vec3(-0.35f, 1.32f, 0)) < 1e-4f,
                    "Arm pivot is not located at its shoulder");
                Require(part.parent >= 0 && mesh.parts[static_cast<std::size_t>(part.parent)].name == "body",
                    "Local part hierarchy was not preserved");
                const auto changed = glm::rotate(glm::mat4(1), 0.5f, part.rotation_axis) *
                    glm::vec4(part.vertices.front().position, 1);
                Require(glm::length(glm::vec3(changed) - part.vertices.front().position) > 0.01f,
                    "Local limb geometry cannot rotate independently around its pivot");
            }
            count += part.vertices.size();
        }
        Require(count == mesh.vertices.size() && std::abs(minimum.y) < 1e-4f && maximum.y > 1.7f,
            "Part local transforms do not reconstruct the complete bind pose");
    }
    void ExpectFailure(const std::filesystem::path& path, std::string_view reason = {}) {
        try { SymoCraft::Assets::LoadCharacter(path); }
        catch (const std::runtime_error& error) {
            Require(std::string{error.what()}.find(path.filename().string()) != std::string::npos,
                "Import error must identify the character file");
            Require(reason.empty() || std::string_view(error.what()).find(reason) != std::string_view::npos,
                "Import failed at a different boundary than expected");
            return;
        }
        throw std::runtime_error("Invalid character unexpectedly imported");
    }
}
int main(int argc, char** argv) {
    try {
        Require(argc == 3, "Expected human and zombie GLB paths");
        const auto human = SymoCraft::Assets::LoadCharacter(argv[1]);
        const auto saved = human.vertices.front();
        const auto zombie = SymoCraft::Assets::LoadCharacter(argv[2]);
        Check(human); Check(zombie);
        CheckParts(human); CheckParts(zombie);
        Require(human.vertices.front().position == saved.position && human.vertices.front().color == saved.color,
            "Later import invalidated owned character data");
        Require(human.vertices.size() != zombie.vertices.size() ||
            !std::equal(human.vertices.begin(), human.vertices.end(), zombie.vertices.begin(),
                [](const auto& a, const auto& b) { return a.color == b.color; }),
            "Human and zombie assets must have distinct appearances");
        ExpectFailure(std::filesystem::path{argv[1]}.parent_path() / "missing-character.glb");
        TemporaryFile invalid{std::filesystem::current_path() / ("invalid-character-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".glb")};
        { std::ofstream file(invalid.path, std::ios::binary); file << "not a GLB"; }
        ExpectFailure(invalid.path);
        // The marker opts into a complete rigid schema; bad parts never fall back to static.
        const auto reject = [&](const std::function<void(std::string&)>& edit) {
            Mutate(argv[1], invalid.path, edit); ExpectFailure(invalid.path);
        };
        reject([](auto& json) { Replace(json, "\"name\":\"left_arm\"", "\"name\":\"missing_arm\""); });
        reject([](auto& json) { Replace(json, "\"name\":\"right_arm\"", "\"name\":\"left_arm\""); });
        reject([](auto& json) { Replace(json, "\"name\":\"left_arm_axis\",\"translation\":[1,0,0]",
            "\"name\":\"left_arm_axis\",\"translation\":[0,0,0]"); });
        reject([](auto& json) { Replace(json, "\"name\":\"head\"", "\"name\":\"head\",\"scale\":[0,1,1]"); });
        reject([](auto& json) { Replace(json, "\"name\":\"head\"", "\"name\":\"head\",\"scale\":[1e999,1,1]"); });
        reject([](auto& json) { Replace(json, "\"children\":[0,2,4,6,8,10]", "\"children\":[0,2,4,6,8,10,12]"); });
        Mutate(argv[1], invalid.path, [](auto& json) {
            std::string nodes; for (int i = 0; i < 128; ++i) nodes += "{},";
            const auto position = json.rfind("\"nodes\":[");
            Require(position != std::string::npos, "Fixture root node array is missing");
            json.insert(position + std::string_view("\"nodes\":[").size(), nodes);
        });
        ExpectFailure(invalid.path, "node count exceeds limit");
        // Whole-body static imports retain the original baked geometry path.
        Mutate(argv[1], invalid.path, [](auto& json) {
            Replace(json, "symocraft_rigid_v1", "legacy_static_model");
        });
        const auto legacy = SymoCraft::Assets::LoadCharacter(invalid.path);
        Require(legacy.parts.empty() && legacy.vertices.size() == human.vertices.size(),
            "Legacy whole-body static compatibility was lost");
        Check(legacy);
        // Unsupported color sets must not silently turn a character white.
        { std::ifstream source(argv[1], std::ios::binary);
          std::string bytes{std::istreambuf_iterator<char>{source}, std::istreambuf_iterator<char>{}};
          const auto attribute = bytes.find("COLOR_0");
          Require(attribute != std::string::npos, "Fixture has no color accessor");
          bytes[attribute + 6] = '1';
          std::ofstream file(invalid.path, std::ios::binary); file.write(bytes.data(), bytes.size()); }
        ExpectFailure(invalid.path);
        // A valid JSON envelope with a truncated binary payload must fail before attribute reads.
        { std::ifstream source(argv[1], std::ios::binary); std::ofstream file(invalid.path, std::ios::binary);
          char buffer[32]; source.read(buffer, sizeof(buffer)); file.write(buffer, source.gcount()); }
        ExpectFailure(invalid.path);
        std::cout << "CPU character GLB ownership, rigid parts, hierarchy, pivots, static compatibility and diagnostics passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
