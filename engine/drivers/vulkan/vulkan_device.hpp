#ifndef engine_drivers_vulkan_VULKAN_DEVICE_HPP
#define engine_drivers_vulkan_VULKAN_DEVICE_HPP

#include "engine/core/graphics/device.hpp"
#include "engine/core/graphics/instance.hpp"
#include "engine/core/graphics/shader.hpp"
#include "engine/core/graphics/pipeline.hpp"
#include "engine/core/graphics/mesh_buffer.hpp"
#include "engine/core/graphics/material.hpp"
#include "engine/core/graphics/descriptor_set_layout.hpp"

#include "engine/core/window/window.hpp"

#include <wk/wulkan.hpp>
#include <wk/ext/glfw/surface.hpp>

namespace engine::drivers::vulkan {

class VulkanInstance;

class VulkanDevice final : public core::graphics::Device {
public:
    VulkanDevice(const VulkanInstance& instance, const core::window::Window& window);
    ~VulkanDevice() override = default;

    void wait_idle() override;

    std::unique_ptr<core::graphics::Shader> create_shader(engine::core::graphics::ShaderStageFlags stage, const std::string& filepath) const override;
    std::unique_ptr<core::graphics::MeshBuffer> create_mesh_buffer(
        const void* vertex_data, uint32_t vertex_size, uint32_t vertex_count,
        const void* index_data, uint32_t index_size, uint32_t index_count) const override;
    std::unique_ptr<core::graphics::Pipeline> create_pipeline(
        const core::graphics::Shader& vert, const core::graphics::Shader& frag,
        const core::graphics::DescriptorSetLayout& layout,
        const core::graphics::VertexBinding& vertex_binding, 
        const std::vector<core::graphics::ImageAttachmentInfo>& attachment_info
    ) const override;
    std::unique_ptr<core::graphics::RenderTarget> create_viewport(const core::window::Window& window,
        const core::graphics::Pipeline& pipeline,
        uint32_t width, uint32_t height
    ) const override;
    std::unique_ptr<core::graphics::DescriptorSetLayout> create_descriptor_set_layout(
        const core::graphics::DescriptorLayoutDescription& description
    ) const override;

    void* begin_command_buffer() const override;
    void end_command_buffer(void* command_buffer) const override;

    const wk::PhysicalDevice& physical_device() const { return _physical_device; }

    const wk::Device& device() const { return _device; }
    const wk::Allocator& allocator() const { return _allocator; }
    const wk::CommandPool& command_pool() const { return _command_pool; }
    const wk::DescriptorPool& descriptor_pool() const { return _descriptor_pool; }
    const wk::Queue& graphics_queue() const { return _graphics_queue; }
    uint32_t present_family() const { return _present_family; }
    core::graphics::ImageFormat present_format() const { return _present_format; }
    core::graphics::ColorSpace present_color_space() const { return _present_color_space; }
    core::graphics::ImageFormat depth_format() const { return _depth_format; }

    const wk::DeviceQueueFamilyIndices& queue_families() const { return _queue_families; }

    void* native_device() const override { return static_cast<void*>(_device.handle()); }
    void* native_physical_device() const override { return static_cast<void*>(_physical_device.handle()); }
    void* native_descriptor_pool() const override { return static_cast<void*>(_descriptor_pool.handle()); }
    void* native_graphics_queue() const override { return static_cast<void*>(_graphics_queue.handle()); }
    uint32_t native_graphics_queue_family() const override { return _graphics_queue.family_index(); }
    std::string backend_name() const override { return "Vulkan"; }

private:
    wk::PhysicalDevice _physical_device;
    wk::Device _device;

    wk::DeviceQueueFamilyIndices _queue_families;

    wk::Queue _graphics_queue;

    wk::Allocator _allocator;
    wk::CommandPool _command_pool;
    wk::DescriptorPool _descriptor_pool;

    uint32_t _present_family;
    core::graphics::ImageFormat _present_format;
    core::graphics::ColorSpace _present_color_space;
    core::graphics::ImageFormat _depth_format;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_RENDER_TARGET_HPP
