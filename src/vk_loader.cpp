#include "stb_image.h"
#include <iostream>
#include <vk_loader.h>

#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_types.h"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

//needed for the fastgltf variants
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanEngine* engine, std::filesystem::path filePath)
{

    std::cout << "Loading GLTF: " << filePath << std::endl;

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser{};

    auto load = parser.loadBinaryGLTF(&data, filePath.parent_path(), gltfOptions);
    if (load) {
        gltf = std::move(load.get());
    }
    else {
        fmt::print("Failed to load glTF: {} \n", fastgltf::to_underlying(load.error()));
        return {};
    }

    std::vector<std::shared_ptr<MeshAsset>> meshes;

    // use the same vectors for all meshes so that the memory doesnt reallocate as
    // often
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    for (fastgltf::Mesh& mesh : gltf.meshes) {
        MeshAsset newmesh;

        newmesh.name = mesh.name;

        // clear the mesh arrays each mesh, we dont want to merge them by error
        indices.clear();
        vertices.clear();

        for (auto&& p : mesh.primitives) {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = vertices.size();
            {
                fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor, [&](std::uint32_t idx) {
                    indices.push_back(idx + initial_vtx);
                    });
            }

            fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];

            size_t vidx = initial_vtx;
            fastgltf::iterateAccessor<glm::vec3>(gltf, posAccessor,
                [&](glm::vec3 v) {
                    Vertex newvtx;
                    newvtx.position = v;
                    newvtx.normal = { 1,0,0 };
                    newvtx.color = glm::vec4{ 1.f };
                    newvtx.uv_x = 0;
                    newvtx.uv_y = 0;
                    vertices.push_back(newvtx);
                });

            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {
                vidx = initial_vtx;
                fastgltf::iterateAccessor<glm::vec3>(gltf, gltf.accessors[(*normals).second],
                    [&](glm::vec3 v) { vertices[vidx++].normal = v; });
            }

            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {
                vidx = initial_vtx;
                fastgltf::iterateAccessor<glm::vec2>(gltf, gltf.accessors[(*uv).second], [&](glm::vec2 v) {

                    vertices[vidx].uv_x = v.x;
                    vertices[vidx].uv_y = v.y;
                    vidx++;
                    });
            }

            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {
                vidx = initial_vtx;
                fastgltf::iterateAccessor<glm::vec4>(gltf, gltf.accessors[(*colors).second],
                    [&](glm::vec4 v) { vertices[vidx++].color = v; });
            }
            newmesh.surfaces.push_back(newSurface);
        }

        //display the vertex normals
        constexpr bool OverrideColors = false;
        if (OverrideColors) {
            for (Vertex& vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }
        newmesh.meshBuffers = engine->uploadMesh(indices, vertices);

        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
    }

    return meshes;

}