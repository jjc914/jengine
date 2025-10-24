#ifndef engine_import_MESH_HPP
#define engine_import_MESH_HPP

#include "engine/core/debug/logger.hpp"

#include <tiny_obj_loader.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

namespace engine::import {

struct ObjVertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 texcoord{};
};

struct ObjMesh {
    std::string name;
    std::vector<ObjVertex> vertices;
    std::vector<uint32_t> indices;
};

struct ObjModel {
    std::vector<ObjMesh> meshes;
};

ObjModel ReadObj(const std::string& filepath);

} // namespace engine::import

#endif // engine_import_MESH_HPP
