#include "scene.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

namespace broken {

static glm::mat4 compute_node_transform(const fastgltf::Node& node) {
    auto matrixData = fastgltf::getTransformMatrix(node);
    return glm::make_mat4(matrixData.data());
}

static void
parse_node(const fastgltf::Asset& asset, size_t nodeIndex, const glm::mat4& parentTransform, scene& outScene) {
    const fastgltf::Node& node = asset.nodes[nodeIndex];
    glm::mat4 globalTransform = parentTransform * compute_node_transform(node);

    std::string nodeName = node.name.empty() ? "Unnamed Node" : std::string(node.name);
    broken_trace("Processing node: {}", nodeName);

    if (node.meshIndex.has_value()) {
        const fastgltf::Mesh& gltfMesh = asset.meshes[node.meshIndex.value()];
        broken_trace("Node '{}' has mesh index {}", nodeName, node.meshIndex.value());

        for (size_t primIndex = 0; primIndex < gltfMesh.primitives.size(); ++primIndex) {
            const auto& primitive = gltfMesh.primitives[primIndex];

            if (primitive.type != fastgltf::PrimitiveType::Triangles) {
                broken_warn(
                    "Primitive {} in mesh {} is not a triangle soup. Skipping.", primIndex, node.meshIndex.value());
                continue;
            }

            scene_object obj{};
            obj.transform = globalTransform;

            if (primitive.indicesAccessor.has_value()) {
                const fastgltf::Accessor& indexAccessor = asset.accessors[primitive.indicesAccessor.value()];
                obj.mesh.indices.reserve(indexAccessor.count);

                if (indexAccessor.count > 65535) {
                    broken_warn("Primitive has {} indices, which exceeds uint16_t capacity!", indexAccessor.count);
                }

                fastgltf::iterateAccessorWithIndex<uint32_t>(asset, indexAccessor, [&](uint32_t idx, size_t) {
                    obj.mesh.indices.push_back(static_cast<mesh::index>(idx));
                });
            }

            auto posIt = primitive.findAttribute("POSITION");
            if (posIt != primitive.attributes.end()) {
                const fastgltf::Accessor& posAccessor = asset.accessors[posIt->accessorIndex];
                obj.mesh.vertices.resize(posAccessor.count);

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset, posAccessor, [&](fastgltf::math::fvec3 pos, size_t index) {
                        obj.mesh.vertices[index].position = glm::vec3(pos.x(), pos.y(), pos.z());
                    });
            } else {
                broken_error("Primitive {} is missing POSITION attribute!", primIndex);
                continue;
            }

            auto normIt = primitive.findAttribute("NORMAL");
            if (normIt != primitive.attributes.end()) {
                const fastgltf::Accessor& normAccessor = asset.accessors[normIt->accessorIndex];

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset, normAccessor, [&](fastgltf::math::fvec3 norm, size_t index) {
                        obj.mesh.vertices[index].normal = glm::vec3(norm.x(), norm.y(), norm.z());
                    });
            }

            broken_debug("Loaded primitive {} with {} vertices and {} indices",
                         primIndex,
                         obj.mesh.vertices.size(),
                         obj.mesh.indices.size());

            outScene.objects.push_back(std::move(obj));
        }
    }

    for (size_t childIndex : node.children) {
        parse_node(asset, childIndex, globalTransform, outScene);
    }
}

std::optional<scene> scene::load_from_file(const fs::path& path) {
    broken_info("Loading glTF scene from: {}", path.string());

    if (!fs::exists(path)) {
        broken_error("File does not exist: {}", path.string());
        return std::nullopt;
    }

    fastgltf::GltfFileStream fileStream(path);
    if (!fileStream.isOpen()) {
        broken_error("Failed to open glTF file stream: {}", path.string());
        return std::nullopt;
    }

    fastgltf::Parser parser;
    constexpr auto options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::LoadExternalBuffers;

    auto data = parser.loadGltfJson(fileStream, path.parent_path(), options);
    if (!data) {
        broken_error("Failed to parse glTF JSON. Error code: {}", static_cast<int>(data.error()));
        return std::nullopt;
    }

    const fastgltf::Asset& asset = data.get();

    scene outScene{};

    if (asset.scenes.empty()) {
        broken_error("No scenes found in glTF file.");
        return std::nullopt;
    }

    const fastgltf::Scene& gltfScene = asset.scenes[asset.defaultScene.value_or(0)];
    broken_info("glTF asset parsed successfully. Found {} root nodes.", gltfScene.nodeIndices.size());

    for (size_t rootNodeIndex : gltfScene.nodeIndices) {
        parse_node(asset, rootNodeIndex, glm::mat4(1.0f), outScene);
    }

    broken_info("Scene loaded. Total objects in scene: {}", outScene.objects.size());
    return outScene;
}

} // namespace broken
