#ifndef engine_core_graphics_DEVICE_HPP
#define engine_core_graphics_DEVICE_HPP

#include "vertex_types.hpp"
#include "image_types.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "mesh_buffer.hpp"
#include "pipeline.hpp"
#include "render_target.hpp"
#include "swapchain_render_target.hpp"
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
    virtual std::unique_ptr<core::graphics::Texture> create_texture(
        uint32_t width,
        uint32_t height,
        core::graphics::ImageFormat format,
        uint32_t layers = 1,
        uint32_t mip_levels = 1,
        core::graphics::TextureUsage usage = core::graphics::TextureUsage::COLOR_ATTACHMENT
    ) const = 0;
    virtual std::unique_ptr<core::graphics::Texture> create_texture_from_native(
        void* image,
        void* view,
        uint32_t width,
        uint32_t height,
        core::graphics::ImageFormat format,
        uint32_t layers,
        uint32_t mip_levels,
        core::graphics::TextureUsage usage
    ) const = 0;
    virtual std::unique_ptr<Pipeline> create_pipeline(
        const Shader& vert, const Shader& frag,
        const DescriptorSetLayout& layout,
        const VertexBindingDescription& vertex_binding, 
        const std::vector<ImageAttachmentInfo>& attachment_info,
        const core::graphics::PipelineConfig& config
    ) const = 0;
    virtual std::unique_ptr<SwapchainRenderTarget> create_swapchain_render_target(
        const window::Window& window, 
        const Pipeline& pipeline,
        uint32_t max_in_flight = 3, bool has_depth = true
    ) const = 0;
    virtual std::unique_ptr<core::graphics::RenderTarget> create_texture_render_target(
        const core::graphics::Pipeline& pipeline,
        const core::graphics::AttachmentInfo& attachments,
        uint32_t max_in_flight = 1
    ) const = 0;
    virtual std::unique_ptr<DescriptorSetLayout> create_descriptor_set_layout(
        const DescriptorLayoutDescription& description
    ) const = 0;

    virtual ImageFormat present_format() const = 0;
    virtual ColorSpace present_color_space() const = 0;
    virtual ImageFormat depth_format() const = 0;
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
