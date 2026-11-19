#include <symocraft/assets/character.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace SymoCraft::Assets {
    namespace {
        constexpr std::size_t MaximumFileBytes = 64 * 1024 * 1024;
        constexpr std::size_t MaximumVertices = 1024 * 1024;
        constexpr std::size_t MaximumNodes = 128;
        constexpr std::array<std::string_view, 6> PartNames{
            "body", "head", "left_arm", "right_arm", "left_leg", "right_leg"};
        bool Finite(const glm::vec3& value) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        }
        void Require(bool condition, const char* reason) {
            if (!condition) throw std::runtime_error(reason);
        }
        void CheckAccessors(const cgltf_data& data) {
            for (std::size_t i = 0; i < data.buffer_views_count; ++i) {
                const auto& view = data.buffer_views[i];
                Require(view.buffer && view.offset <= view.buffer->size &&
                    view.size <= view.buffer->size - view.offset, "buffer view is out of range");
                Require(!view.has_meshopt_compression, "compressed buffer views are unsupported");
            }
            for (std::size_t i = 0; i < data.accessors_count; ++i) {
                const auto& accessor = data.accessors[i];
                Require(!accessor.is_sparse && accessor.buffer_view && accessor.count > 0 &&
                    accessor.count <= MaximumVertices, "unsupported sparse, empty or oversized accessor");
                const auto size = cgltf_calc_size(accessor.type, accessor.component_type);
                const auto stride = accessor.stride;
                const auto available = accessor.buffer_view->size;
                Require(size > 0 && stride >= size && accessor.offset <= available &&
                    size <= available - accessor.offset, "accessor layout is invalid");
                Require(accessor.count - 1 <= (available - accessor.offset - size) / stride,
                    "accessor elements exceed buffer view");
            }
        }
        void CheckTransform(const glm::mat4& transform) {
            for (int column = 0; column < 4; ++column)
                for (int row = 0; row < 4; ++row)
                    Require(std::isfinite(transform[column][row]), "nonfinite node transform");
            Require(transform[0][3] == 0 && transform[1][3] == 0 && transform[2][3] == 0 &&
                transform[3][3] == 1, "only affine node transforms are supported");
            const auto determinant = glm::determinant(glm::mat3(transform));
            Require(std::isfinite(determinant) && std::abs(determinant) > 1e-8f,
                "node transform is singular or nonfinite");
        }
        glm::mat4 NodeTransform(const cgltf_node& node, bool world) {
            glm::mat4 transform;
            if (world) cgltf_node_transform_world(&node, &transform[0][0]);
            else cgltf_node_transform_local(&node, &transform[0][0]);
            CheckTransform(transform);
            return transform;
        }
        // Check all nodes, including non-mesh ancestors, before recursive world transforms.
        void CheckNodes(const cgltf_data& data) {
            Require(data.nodes_count > 0 && data.nodes_count <= MaximumNodes,
                "character node count exceeds limit");
            for (std::size_t i = 0; i < data.nodes_count; ++i) {
                const auto& node = data.nodes[i];
                NodeTransform(node, false);
                auto* parent = node.parent;
                for (std::size_t depth = 0; parent; ++depth, parent = parent->parent)
                    Require(depth < data.nodes_count, "cyclic node hierarchy");
            }
        }
        std::vector<int> BuildParts(const cgltf_data& data, CharacterMeshData& output) {
            std::vector<int> node_parts(data.nodes_count, -1);
            const cgltf_node* root = nullptr;
            for (std::size_t i = 0; i < data.nodes_count; ++i) {
                const auto& node = data.nodes[i];
                if (node.name && std::string_view(node.name) == "symocraft_rigid_v1") {
                    Require(!root && !node.mesh && !node.parent, "invalid or duplicate rigid character root");
                    root = &node;
                }
            }
            if (!root) return node_parts;
            for (const auto name : PartNames) {
                const cgltf_node* found = nullptr;
                for (std::size_t i = 0; i < data.nodes_count; ++i)
                    if (data.nodes[i].name && name == data.nodes[i].name) {
                        Require(!found, "duplicate rigid body part name");
                        found = &data.nodes[i];
                    }
                Require(found && found->mesh, "missing required rigid body part");
                auto* ancestor = found->parent;
                while (ancestor && ancestor != root) ancestor = ancestor->parent;
                Require(ancestor == root, "rigid body part is outside character root");
                node_parts[static_cast<std::size_t>(found - data.nodes)] = static_cast<int>(output.parts.size());
                CharacterPart part;
                part.name = name;
                output.parts.push_back(std::move(part));
            }
            for (std::size_t i = 0; i < data.nodes_count; ++i) {
                const auto& node = data.nodes[i];
                Require(!node.mesh || node_parts[i] >= 0, "unassigned mesh in rigid character");
                if (node_parts[i] < 0) continue;
                auto& part = output.parts[static_cast<std::size_t>(node_parts[i])];
                const auto world = NodeTransform(node, true);
                auto* ancestor = node.parent;
                while (ancestor && ancestor != root && node_parts[static_cast<std::size_t>(ancestor - data.nodes)] < 0)
                    ancestor = ancestor->parent;
                if (ancestor && ancestor != root) {
                    part.parent = node_parts[static_cast<std::size_t>(ancestor - data.nodes)];
                    part.local_transform = glm::inverse(NodeTransform(*ancestor, true)) * world;
                } else part.local_transform = world;
                CheckTransform(part.local_transform);
                // Reflecting a rigid hierarchy is outside the supported pose representation.
                Require(glm::determinant(glm::mat3(world)) > 1e-8f,
                    "reflected rigid part transforms are unsupported");
                const auto axis_name = part.name + "_axis";
                const cgltf_node* axis = nullptr;
                for (std::size_t j = 0; j < data.nodes_count; ++j)
                    if (data.nodes[j].name && axis_name == data.nodes[j].name) {
                        Require(!axis, "duplicate rigid joint axis");
                        axis = &data.nodes[j];
                    }
                Require(axis && axis->parent == &node && !axis->mesh && axis->children_count == 0,
                    "missing or invalid explicit rigid joint axis");
                const auto axis_transform = NodeTransform(*axis, false);
                Require(glm::mat3(axis_transform) == glm::mat3(1), "joint axis marker must not rotate or scale");
                const auto direction = glm::vec3(axis_transform[3]);
                const auto length = glm::length(direction);
                Require(Finite(direction) && std::isfinite(length) && length > 1e-8f,
                    "invalid rigid joint axis direction");
                part.rotation_axis = direction / length;
            }
            return node_parts;
        }
        const cgltf_accessor* Attribute(const cgltf_primitive& primitive, cgltf_attribute_type type) {
            const cgltf_accessor* result = nullptr;
            for (std::size_t i = 0; i < primitive.attributes_count; ++i) {
                const auto& attribute = primitive.attributes[i];
                if (attribute.type == type && attribute.index == 0) {
                    Require(!result, "duplicate vertex attribute");
                    result = attribute.data;
                }
            }
            return result;
        }
        CharacterMeshData Import(const std::filesystem::path& path) {
            std::ifstream stream(path, std::ios::binary | std::ios::ate);
            Require(static_cast<bool>(stream), "cannot open GLB");
            const auto length = stream.tellg();
            Require(length > 0 && length <= static_cast<std::streamoff>(MaximumFileBytes),
                "GLB is empty or exceeds 64 MiB");
            std::vector<char> bytes(static_cast<std::size_t>(length));
            stream.seekg(0);
            stream.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            Require(static_cast<bool>(stream), "cannot read complete GLB");
            cgltf_options options{};
            options.type = cgltf_file_type_glb;
            cgltf_data* parsed = nullptr;
            Require(cgltf_parse(&options, bytes.data(), bytes.size(), &parsed) == cgltf_result_success,
                "GLB parsing failed");
            const std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(parsed, cgltf_free);
            Require(data->buffers_count == 1 && !data->buffers[0].uri && data->bin &&
                data->buffers[0].size <= data->bin_size, "only a self-contained GLB buffer is supported");
            Require(data->animations_count == 0 && data->skins_count == 0 &&
                data->extensions_required_count == 0, "animation, skins or required extensions are unsupported");
            Require(data->textures_count == 0 && data->materials_count == 0,
                "only vertex-color characters without materials or textures are supported");
            CheckAccessors(*data);
            CheckNodes(*data);
            Require(cgltf_load_buffers(&options, data.get(), nullptr) == cgltf_result_success &&
                cgltf_validate(data.get()) == cgltf_result_success, "GLB validation failed");
            CharacterMeshData result;
            const auto node_parts = BuildParts(*data, result);
            for (std::size_t node_index = 0; node_index < data->nodes_count; ++node_index) {
                const auto& node = data->nodes[node_index];
                if (!node.mesh) continue;
                const auto transform = NodeTransform(node, true);
                const auto determinant = glm::determinant(glm::mat3(transform));
                CharacterPart* part = node_parts[node_index] >= 0 ?
                    &result.parts[static_cast<std::size_t>(node_parts[node_index])] : nullptr;
                const auto normals = glm::transpose(glm::inverse(glm::mat3(transform)));
                for (std::size_t primitive_index = 0; primitive_index < node.mesh->primitives_count; ++primitive_index) {
                    const auto& primitive = node.mesh->primitives[primitive_index];
                    Require(primitive.type == cgltf_primitive_type_triangles && primitive.targets_count == 0 &&
                        !primitive.has_draco_mesh_compression, "only uncompressed static triangles are supported");
                    const auto* positions = Attribute(primitive, cgltf_attribute_type_position);
                    const auto* normal = Attribute(primitive, cgltf_attribute_type_normal);
                    const auto* color = Attribute(primitive, cgltf_attribute_type_color);
                    Require(positions && normal && color && positions->type == cgltf_type_vec3 &&
                        normal->type == cgltf_type_vec3 && (color->type == cgltf_type_vec3 || color->type == cgltf_type_vec4) &&
                        positions->count == normal->count && positions->count == color->count,
                        "matching position, normal and COLOR_0 attributes are required");
                    const auto count = primitive.indices ? primitive.indices->count : positions->count;
                    Require(count % 3 == 0 && count <= MaximumVertices - result.vertices.size(),
                        "invalid triangle count or character exceeds vertex limit");
                    if (primitive.indices)
                        Require(primitive.indices->type == cgltf_type_scalar &&
                            (primitive.indices->component_type == cgltf_component_type_r_8u ||
                             primitive.indices->component_type == cgltf_component_type_r_16u ||
                             primitive.indices->component_type == cgltf_component_type_r_32u), "invalid index accessor");
                    result.vertices.reserve(result.vertices.size() + count);
                    for (std::size_t triangle = 0; triangle < count; triangle += 3) {
                        for (std::size_t corner = 0; corner < 3; ++corner) {
                            const auto offset = triangle + (determinant < 0 && corner > 0 ? 3 - corner : corner);
                            const auto index = primitive.indices ? cgltf_accessor_read_index(primitive.indices, offset) : offset;
                            Require(index < positions->count, "triangle index exceeds vertex count");
                            glm::vec3 position, direction;
                            std::array<float, 4> tint{1, 1, 1, 1};
                            Require(cgltf_accessor_read_float(positions, index, &position.x, 3) &&
                                cgltf_accessor_read_float(normal, index, &direction.x, 3) &&
                                cgltf_accessor_read_float(color, index, tint.data(), 4), "cannot read vertex attributes");
                            const auto local_position = position;
                            const auto local_normal = direction;
                            position = glm::vec3(transform * glm::vec4(position, 1));
                            direction = normals * direction;
                            const glm::vec3 rgb{tint[0], tint[1], tint[2]};
                            const auto magnitude = glm::length(direction);
                            Require(Finite(position) && Finite(direction) && Finite(rgb) &&
                                std::isfinite(magnitude) && magnitude > 1e-8f &&
                                std::isfinite(tint[3]) && tint[3] == 1.0f &&
                                glm::all(glm::greaterThanEqual(rgb, glm::vec3(0))) &&
                                glm::all(glm::lessThanEqual(rgb, glm::vec3(1))), "invalid vertex position, normal or color");
                            result.vertices.push_back({position, direction / magnitude, rgb});
                            if (part) {
                                const auto local_length = glm::length(local_normal);
                                Require(Finite(local_position) && Finite(local_normal) &&
                                    std::isfinite(local_length) && local_length > 1e-8f,
                                    "invalid local part vertex");
                                part->vertices.push_back({local_position, local_normal / local_length, rgb});
                            }
                        }
                    }
                }
            }
            Require(!result.vertices.empty(), "GLB contains no character triangles");
            for (const auto& part : result.parts)
                Require(!part.vertices.empty(), "rigid body part contains no triangles");
            return result;
        }
    }
    CharacterMeshData LoadCharacter(const std::filesystem::path& path) {
        try { return Import(path); }
        catch (const std::exception& error) {
            const auto location = path.u8string();
            throw std::runtime_error("Character asset " + std::string(location.begin(), location.end()) + ": " + error.what());
        }
    }
}
