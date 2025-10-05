#ifndef CORE_GRAPHICS_VULKAN_PIPELINE_HPP
#define CORE_GRAPHICS_VULKAN_PIPELINE_HPP

#include "../pipeline.hpp"
#include "../vertex_layout.hpp"
#include "vk_renderer.hpp"

#include <wk/wulkan.hpp>

namespace core::graphics::vulkan {

class VulkanPipeline : public Pipeline {
public:
    VulkanPipeline(const VulkanRenderer& renderer, const wk::Device& device, const VertexLayout& layout);
    ~VulkanPipeline() override = default;

    void bind(void* command_buffer) const override;

    std::unique_ptr<Material> create_material(uint32_t uniform_buffer_size) const override;

    const wk::PipelineLayout& layout() const { return _pipeline_layout; }

private:
    const wk::Device& _device;
    const wk::RenderPass& _render_pass;
    const wk::Allocator& _allocator;
    const wk::DescriptorPool& _descriptor_pool;

    wk::PipelineLayout _pipeline_layout;
    wk::Pipeline _pipeline;
    wk::DescriptorSetLayout _descriptor_set_layout;
};

} // namespace core::graphics::vulkan

#endif // CORE_GRAPHICS_VULKAN_PIPELINE_HPP
