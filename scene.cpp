#include "scene.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

namespace broken {

namespace fs = std::filesystem;

void scene::load_glTF(const fs::path& path) {
    broken_info("[glTF] Loading glTF scene from: {}", path.string());

    if (!fs::exists(path)) {
        broken_error("[glTF] File does not exist: {}", path.string());
        return;
    }

    fastgltf::GltfFileStream fileStream(path);
    if (!fileStream.isOpen()) {
        broken_error("[glTF] Failed to open glTF file stream: {}", path.string());
        return;
    }

    fastgltf::Parser parser;
    constexpr auto options = fastgltf::Options::LoadExternalBuffers;

    auto data = parser.loadGltfJson(fileStream, path.parent_path(), options);
    if (!data) {
        broken_error("[glTF] Failed to parse glTF JSON. Error code: {}", static_cast<int>(data.error()));
        return;
    }

    auto& asset = data.get();
    if (asset.scenes.empty()) {
        broken_error("[glTF] No scenes found in glTF file.");
        return;
    }

    const fastgltf::Scene& gltfScene = asset.scenes[asset.defaultScene.value_or(0)];
    broken_info("[glTF] Asset parsed successfully. Found {} scene.", gltfScene.name);

    for (auto i = 0u; i < asset.meshes.size(); ++i) {
        const auto& mesh = asset.meshes[i];
        broken_trace("[glTF] Processing mesh {}...", mesh.name);

        std::vector<broken::mesh::vertex> vertices;
        std::vector<broken::mesh::index> indices;

        for (auto j = 0; j < mesh.primitives.size(); ++j) {
            const auto verticesBefore = vertices.size();
            const auto indicesBefore = indices.size();

            const auto& primitive = mesh.primitives[j];
            if (primitive.type != fastgltf::PrimitiveType::Triangles) {
                broken_warn("[glTF] Primitive {} in mesh {} is not a triangle soup. Skipping.", j, i);
                continue;
            }

            if (primitive.indicesAccessor.has_value()) {
                const fastgltf::Accessor& indexAccessor = asset.accessors[primitive.indicesAccessor.value()];
                indices.reserve(indices.size() + indexAccessor.count);

                if (indexAccessor.count > 0xFFFFFFFF) {
                    broken_warn("[glTF] Primitive has {} indices, which exceeds uint32_t capacity!",
                                indexAccessor.count);
                }

                fastgltf::iterateAccessorWithIndex<uint32_t>(asset, indexAccessor, [&](uint32_t idx, size_t) {
                    indices.push_back(static_cast<broken::mesh::index>(idx));
                });
            }

            auto posIt = primitive.findAttribute("POSITION");
            if (posIt != primitive.attributes.end()) {
                const fastgltf::Accessor& posAccessor = asset.accessors[posIt->accessorIndex];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset, posAccessor, [&](fastgltf::math::fvec3 pos, size_t index) {
                        vertices[index].position = glm::vec3(pos.x(), pos.y(), pos.z());
                        vertices[index].color = glm::vec4(1.f);
                    });
            } else {
                broken_error("[glTF] Primitive {} is missing POSITION attribute!", j);
                continue;
            }

            auto normIt = primitive.findAttribute("NORMAL");
            if (normIt != primitive.attributes.end()) {
                const fastgltf::Accessor& normAccessor = asset.accessors[normIt->accessorIndex];

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    asset, normAccessor, [&](fastgltf::math::fvec3 norm, size_t index) {
                        vertices[index].normal = glm::vec3(norm.x(), norm.y(), norm.z());
                    });
            }

            broken_debug("[glTF] Loaded primitive {} with {} vertices {} indices",
                         j,
                         vertices.size() - verticesBefore,
                         indices.size() - indicesBefore);
        }

        objects.emplace_back(broken::mesh{vertices, indices}, glm::mat4(1.f));
    }

    broken_info("[glTF] Scene loaded. Total objects in scene: {}", objects.size());
}

} // namespace broken
