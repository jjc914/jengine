#ifndef engine_drivers_vulkan_VULKAN_RENDER_PASS_HPP
#define engine_drivers_vulkan_VULKAN_RENDER_PASS_HPP

#include "engine/core/graphics/pipeline.hpp"

#include "engine/core/graphics/descriptor_set_layout.hpp"
#include "engine/core/graphics/image_types.hpp"
#include "engine/core/graphics/descriptor_types.hpp"
#include "engine/core/graphics/vertex_types.hpp"

#include <wk/wulkan.hpp>

namespace engine::drivers::vulkan {

class VulkanDevice;

class VulkanPipeline final : public core::graphics::Pipeline {
public:
    VulkanPipeline(const VulkanDevice& device,
        VkShaderModule vert, VkShaderModule frag,
        const core::graphics::VertexBinding& vertex_binding,
        const core::graphics::DescriptorSetLayout& layout,
        const std::vector<core::graphics::ImageAttachmentInfo>& attachment_info);
    ~VulkanPipeline() override = default;
    
    void bind(void* cb) const override;

    std::unique_ptr<core::graphics::Material> create_material(
        const core::graphics::DescriptorSetLayout& layout, 
        uint32_t uniform_buffer_size
    ) const override;

    const wk::Device& device() const { return _device; }
    const wk::Allocator& allocator() const { return _allocator; }
    const wk::DescriptorPool& descriptor_pool() const { return _descriptor_pool; }
    const wk::PipelineLayout& pipeline_layout() const { return _pipeline_layout; }

    core::graphics::ImageFormat color_format() const override { return _color_format; }
    core::graphics::ImageFormat depth_format() const override { return _depth_format; }

    void* native_pipeline() const override { return static_cast<void*>(_pipeline.handle()); }
    void* native_render_pass() const override { return static_cast<void*>(_render_pass.handle()); }
    std::string backend_name() const override { return "Vulkan"; }

private:
    const wk::Device& _device;
    const wk::Allocator& _allocator;
    const wk::DescriptorPool& _descriptor_pool;

    wk::RenderPass _render_pass;
    wk::PipelineLayout _pipeline_layout;
    wk::Pipeline _pipeline;

    std::vector<core::graphics::ImageAttachmentInfo> _attachment_info;
    core::graphics::ImageFormat _color_format;
    core::graphics::ImageFormat _depth_format;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_RENDER_PASS_HPP