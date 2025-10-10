#ifndef engine_drivers_vulkan_VULKAN_MATERIAL_HPP
#define engine_drivers_vulkan_VULKAN_MATERIAL_HPP

#include "vulkan_pipeline.hpp"

#include "engine/core/graphics/material.hpp"

#include <wk/wulkan.hpp>
#include <string>

namespace engine::drivers::vulkan {

class VulkanDevice;

class VulkanMaterial final : public core::graphics::Material {
public:
    VulkanMaterial(
        const VulkanPipeline& device,
        VkDescriptorSetLayout layout,
        uint32_t uniform_buffer_size
    );
    ~VulkanMaterial() override = default;

    void bind(void* cb) const override;
    void update_uniform_buffer(const void* data) override;

    std::string backend_name() const override { return "Vulkan"; }

private:
    const wk::Device& _device;
    const wk::Allocator& _allocator;
    const wk::DescriptorPool& _descriptor_pool;
    const wk::PipelineLayout& _pipeline_layout;

    wk::DescriptorSet _descriptor_set;
    wk::Buffer _uniform_buffer;

    uint32_t _uniform_buffer_size;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_MATERIAL_HPP
