#ifndef engine_core_graphics_DEVICE_HPP
#define engine_core_graphics_DEVICE_HPP

#include "vertex_types.hpp"
#include "shader.hpp"
#include "mesh_buffer.hpp"
#include "pipeline.hpp"
#include "render_target.hpp"
#include "descriptor_set_layout.hpp"
#include "descriptor_types.hpp"

#include "engine/core/window/window.hpp"

#include <string>

namespace engine::core::graphics {

class Device {
public:
    virtual ~Device() = default;

    virtual void wait_idle() = 0;

    virtual std::unique_ptr<Shader> create_shader(ShaderStageFlags stage, const std::string& filepath) const = 0;
    virtual std::unique_ptr<MeshBuffer> create_mesh_buffer(
        const void* vertex_data, uint32_t vertex_size, uint32_t vertex_count,
        const void* index_data, uint32_t index_size, uint32_t index_count
    ) const = 0;
    virtual std::unique_ptr<Pipeline> create_pipeline(
        const Shader& vert, const Shader& frag,
        const DescriptorSetLayout& layout,
        const VertexBinding& vertex_binding, 
        const std::vector<ImageAttachmentInfo>& attachment_info
    ) const = 0;
    virtual std::unique_ptr<RenderTarget> create_viewport(
        const window::Window& window, 
        const Pipeline& render_pass,
        uint32_t width, uint32_t height
    ) const = 0;
    virtual std::unique_ptr<core::graphics::RenderTarget> create_texture_render_target(
        const core::graphics::Pipeline& pipeline,
        uint32_t width, uint32_t height
    ) const = 0;
    virtual std::unique_ptr<DescriptorSetLayout> create_descriptor_set_layout(
        const DescriptorLayoutDescription& description
    ) const = 0;

    virtual void* begin_command_buffer() const = 0;
    virtual void end_command_buffer(void* cb) const = 0;

    virtual void* native_device() const = 0;
    virtual void* native_physical_device() const = 0;
    virtual void* native_descriptor_pool() const = 0;
    virtual void* native_graphics_queue() const = 0;
    virtual uint32_t native_graphics_queue_family() const = 0;
    virtual std::string backend_name() const = 0;

protected:
    Device() = default;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_DEVICE_HPP
