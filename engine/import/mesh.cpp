#include "mesh.hpp"

namespace import {

ObjModel ReadObj(const std::string& filepath) {
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.vertex_color = false;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filepath, config)) {
        if (!reader.Error().empty()) {
            engine::core::debug::Logger::get_singleton().error("OBJ parse error: {}", reader.Error());
        }
        return {};
    }

    if (!reader.Warning().empty()) {
        engine::core::debug::Logger::get_singleton().warn("OBJ warning: {}", reader.Warning());
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    ObjModel model;

    for (const tinyobj::shape_t& shape : shapes) {
        ObjMesh mesh;
        mesh.name = shape.name.empty() ? "Unnamed" : shape.name;

        for (const tinyobj::index_t& index : shape.mesh.indices) {
            ObjVertex vertex{};

            if (index.vertex_index >= 0) {
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
            }

            if (index.normal_index >= 0) {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }

            if (index.texcoord_index >= 0) {
                vertex.texcoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // flip V
                };
            }

            mesh.vertices.push_back(vertex);
            mesh.indices.push_back(static_cast<uint32_t>(mesh.indices.size()));
        }

        model.meshes.push_back(std::move(mesh));
    }

    return model;
}

}