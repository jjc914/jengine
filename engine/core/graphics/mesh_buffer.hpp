#ifndef engine_core_graphics_MESH_BUFFER_HPP
#define engine_core_graphics_MESH_BUFFER_HPP

#include <cstdint>
#include <memory>

namespace engine::core::graphics {

class MeshBuffer {
public:
    virtual ~MeshBuffer() = default;

    virtual void bind(void* command_buffer) const = 0;

    virtual uint32_t vertex_count() const = 0;
    virtual uint32_t index_count() const = 0;

    virtual void* vertex_buffer_handle() const = 0;
    virtual void* index_buffer_handle() const = 0;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_MESH_BUFFER_HPP
