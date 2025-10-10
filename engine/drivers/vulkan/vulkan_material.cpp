#include "vulkan_material.hpp"

#include "vulkan_pipeline.hpp"
#include "vulkan_descriptor_set_layout.hpp"

#include <cstring>

namespace engine::drivers::vulkan {

VulkanMaterial::VulkanMaterial(
    const VulkanPipeline& pipeline,
    VkDescriptorSetLayout layout,
    uint32_t uniform_buffer_size
)
    : _device(pipeline.device()),
      _allocator(pipeline.allocator()),
      _descriptor_pool(pipeline.descriptor_pool()),
      _pipeline_layout(pipeline.pipeline_layout()),
      _uniform_buffer_size(uniform_buffer_size)
{
    // uniform buffer
    _uniform_buffer = wk::Buffer(
        _allocator.handle(),
        wk::BufferCreateInfo{}
            .set_size(uniform_buffer_size)
            .set_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
            .to_vk(),
        wk::AllocationCreateInfo{}
            .set_usage(VMA_MEMORY_USAGE_CPU_TO_GPU)
            .to_vk()
    );

    // descriptor set
    _descriptor_set = wk::DescriptorSet(
        _device.handle(),
        wk::DescriptorSetAllocateInfo{}
            .set_descriptor_pool(_descriptor_pool.handle())
            .set_set_layouts(1, &layout)
            .to_vk()
    );

    // write uniform descriptor
    VkDescriptorBufferInfo buffer_info = wk::DescriptorBufferInfo{}
        .set_buffer(_uniform_buffer.handle())
        .set_offset(0)
        .set_range(VK_WHOLE_SIZE)
        .to_vk();

    VkWriteDescriptorSet ubo_write = wk::WriteDescriptorSet{}
        .set_dst_set(_descriptor_set.handle())
        .set_dst_binding(0)
        .set_descriptor_type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        .set_descriptor_count(1)
        .set_p_buffer_info(&buffer_info)
        .to_vk();

    vkUpdateDescriptorSets(_device.handle(), 1, &ubo_write, 0, nullptr);
}

void VulkanMaterial::update_uniform_buffer(const void* data) {
    void* mapped = nullptr;
    vmaMapMemory(_allocator.handle(), _uniform_buffer.allocation(), &mapped);
    std::memcpy(mapped, data, _uniform_buffer_size);
    vmaUnmapMemory(_allocator.handle(), _uniform_buffer.allocation());
}

void VulkanMaterial::bind(void* cb) const {
    vkCmdBindDescriptorSets(static_cast<VkCommandBuffer>(cb),
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _pipeline_layout.handle(),
        0,
        1, &_descriptor_set.handle(),
        0, nullptr);
}

} // namespace engine::drivers::vulkan
