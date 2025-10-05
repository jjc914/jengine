#ifndef CORE_GRAPHICS_MESH_BUFFER_HPP
#define CORE_GRAPHICS_MESH_BUFFER_HPP

#include <cstdint>
#include <memory>

namespace core::graphics {

class MeshBuffer {
public:
    virtual ~MeshBuffer() = default;

    virtual void bind(void* command_buffer) const = 0;

    virtual uint32_t vertex_count() const = 0;
    virtual uint32_t index_count() const = 0;

    virtual void* vertex_buffer_handle() const = 0;
    virtual void* index_buffer_handle() const = 0;
};

} // namespace core::graphics

#endif // CORE_GRAPHICS_MESH_BUFFER_HPP
