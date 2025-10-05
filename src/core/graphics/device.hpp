#ifndef CORE_GRAPHICS_DEVICE_HPP
#define CORE_GRAPHICS_DEVICE_HPP

#include "renderer.hpp"
#include "mesh_buffer.hpp"

#include <string>

namespace core::graphics {

class Device {
public:
    virtual ~Device() = default;

    virtual void wait_idle() = 0;

    virtual std::unique_ptr<Renderer> create_renderer(uint32_t width, uint32_t height) const = 0;
    virtual std::unique_ptr<Renderer> create_renderer(void* surface, uint32_t width, uint32_t height) const = 0;
    virtual std::unique_ptr<MeshBuffer> create_mesh_buffer(
        const void* vertex_data, uint32_t vertex_size, uint32_t vertex_count,
        const void* index_data, uint32_t index_size, uint32_t index_count) const = 0;

    virtual void* native_handle() const = 0;
    virtual std::string backend_name() const = 0;

protected:
    Device() = default;
};

} // namespace core::graphics

#endif // CORE_GRAPHICS_DEVICE_HPP
