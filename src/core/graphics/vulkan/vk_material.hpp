#ifndef CORE_GRAPHICS_VULKAN_MATERIAL_HPP
#define CORE_GRAPHICS_VULKAN_MATERIAL_HPP

#include "../material.hpp"
#include "vk_device.hpp"
#include "vk_pipeline.hpp"

#include <wk/wulkan.hpp>
#include <string>

namespace core::graphics::vulkan {

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

} // namespace core::graphics::vulkan

#endif // CORE_GRAPHICS_VULKAN_MATERIAL_HPP
