#ifndef DRIVERS_VULKAN_VULKAN_MATERIAL_HPP
#define DRIVERS_VULKAN_VULKAN_MATERIAL_HPP

#include "core/graphics/material.hpp"

#include <wk/wulkan.hpp>
#include <string>

namespace drivers::vulkan {

class VulkanPipeline;

class VulkanMaterial final : public core::graphics::Material {
public:
    VulkanMaterial(
        const VulkanPipeline& pipeline,
        const wk::Device& device,
        const wk::Allocator& allocator,
        const wk::DescriptorPool& descriptor_pool,
        const wk::DescriptorSetLayout& descriptor_layout,
        uint32_t uniform_buffer_size
    );
    ~VulkanMaterial() override = default;

    void bind(void* command_buffer) const override;
    void update_uniform_buffer(const void* data) override;

    std::string backend_name() const override { return "Vulkan"; }

private:
    const wk::Device& _device;
    const wk::Allocator& _allocator;
    const wk::DescriptorPool& _descriptor_pool;
    const wk::PipelineLayout& _pipeline_layout;
    const wk::DescriptorSetLayout& _descriptor_layout;

    wk::DescriptorSet _descriptor_set;
    wk::Buffer _uniform_buffer;

    uint32_t _uniform_buffer_size;
};

} // namespace drivers::vulkan

#endif // DRIVERS_VULKAN_VULKAN_MATERIAL_HPP
