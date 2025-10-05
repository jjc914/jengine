#ifndef DRIVERS_VULKAN_VULKAN_PIPELINE_HPP
#define DRIVERS_VULKAN_VULKAN_PIPELINE_HPP

#include "core/graphics/pipeline.hpp"
#include "core/graphics/material.hpp"
#include "core/graphics/vertex_layout.hpp"

#include <wk/wulkan.hpp>

namespace drivers::vulkan {

class VulkanRenderer;
class VulkanMaterial;

class VulkanPipeline : public core::graphics::Pipeline {
public:
    VulkanPipeline(const VulkanRenderer& renderer, const wk::Device& device, const core::graphics::VertexLayout& layout);
    ~VulkanPipeline() override = default;

    void bind(void* command_buffer) const override;

    std::unique_ptr<core::graphics::Material> create_material(uint32_t uniform_buffer_size) const override;

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

} // namespace drivers::vulkan

#endif // DRIVERS_VULKAN_VULKAN_PIPELINE_HPP
