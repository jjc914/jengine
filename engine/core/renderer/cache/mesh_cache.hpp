#ifndef engine_core_renderer_cache_MESH_CACHE_HPP
#define engine_core_renderer_cache_MESH_CACHE_HPP

#include "engine/core/graphics/device.hpp"
#include "engine/core/graphics/mesh_buffer.hpp"
#include "engine/import/mesh.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include <vector>
#include <cstdint>

namespace engine::core::renderer::cache {

using MeshCacheId = uint32_t;

class MeshCache {
public:
    MeshCache(const graphics::Device& device) : _device(device), _next_id(0) {}
    MeshCache(const MeshCache&) = delete;
    MeshCache& operator=(const MeshCache&) = delete;
    ~MeshCache() = default;

    MeshCacheId register_mesh(const import::ObjModel& model) {
        if (model.meshes.empty()) return -1;

        const auto& mesh = model.meshes[0];
        std::unique_ptr<graphics::MeshBuffer> mesh_buffer = _device.create_mesh_buffer(
            mesh.vertices.data(), sizeof(import::ObjVertex), mesh.vertices.size(),
            mesh.indices.data(), sizeof(uint32_t), mesh.indices.size()
        );
        MeshCacheId id = _next_id++;
        _meshes.emplace_back(std::move(mesh_buffer));
        return id;
    }

    void clear() noexcept {
        _meshes.clear();
    }

    graphics::MeshBuffer* get(MeshCacheId id) {
        ENGINE_ASSERT(id < _next_id, "Invalid MeshCacheId for MeshCache");
        return _meshes.at(id).get();
    }

    const graphics::MeshBuffer* get(MeshCacheId id) const {
        ENGINE_ASSERT(id < _next_id, "Invalid MeshCacheId for MeshCache");
        return _meshes.at(id).get();
    }

private:
    const graphics::Device& _device;
    
    MeshCacheId _next_id;
    std::vector<std::unique_ptr<graphics::MeshBuffer>> _meshes;
};

} // namespace engine::core::renderer::cache

#endif // engine_core_renderer_cache_MESH_CACHE_HPP