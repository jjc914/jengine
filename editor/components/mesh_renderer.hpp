#ifndef editor_components_MESH_RENDERER_HPP
#define editor_components_MESH_RENDERER_HPP

#include "engine/core/renderer/cache/mesh_cache.hpp"
#include "engine/core/renderer/cache/material_cache.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace editor::components {

struct MeshRenderer {
    engine::core::renderer::cache::MeshCacheId mesh_id;
    engine::core::renderer::cache::MaterialCacheId material_id;
    bool visible = true;
};

} // namespace editor::components

#endif //editor_components_MESH_RENDERER_HPP