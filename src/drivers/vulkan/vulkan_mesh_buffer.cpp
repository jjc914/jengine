#include "vulkan_mesh_buffer.hpp"
#include "vulkan_device.hpp"

#include <cstring>

namespace drivers::vulkan {

VulkanMeshBuffer::VulkanMeshBuffer(
    const VulkanDevice& device,
    const void* vertex_data, uint32_t vertex_size, uint32_t vertex_count,
    const void* index_data, uint32_t index_size, uint32_t index_count)
    : _device(device.device()),
      _allocator(device.allocator()), _command_pool(device.command_pool()),
      _vertex_count(vertex_count), _index_count(index_count)
{
    // ---------- geometry buffers ----------
    _vertex_buffer = wk::Buffer(
        _allocator.handle(),
        wk::BufferCreateInfo{}
            .set_size(_vertex_count * vertex_size)
            .set_usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
            .to_vk(),
        wk::AllocationCreateInfo{}.set_usage(VMA_MEMORY_USAGE_GPU_ONLY).to_vk()
    );

    _index_buffer = wk::Buffer(
        _allocator.handle(),
        wk::BufferCreateInfo{}
            .set_size(_index_count * index_size)
            .set_usage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
            .to_vk(),
        wk::AllocationCreateInfo{}.set_usage(VMA_MEMORY_USAGE_GPU_ONLY).to_vk()
    );

    _index_type = (index_size == 4) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

    // ---------- staging buffers ----------
    wk::Buffer vertex_staging(
        _allocator.handle(),
        wk::BufferCreateInfo{}
            .set_size(_vertex_count * vertex_size)
            .set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
            .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
            .to_vk(),
        wk::AllocationCreateInfo{}.set_usage(VMA_MEMORY_USAGE_CPU_ONLY).to_vk()
    );

    wk::Buffer index_staging(
        _allocator.handle(),
        wk::BufferCreateInfo{}
            .set_size(_index_count * index_size)
            .set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
            .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
            .to_vk(),
        wk::AllocationCreateInfo{}.set_usage(VMA_MEMORY_USAGE_CPU_ONLY).to_vk()
    );

    // ---------- upload ----------
    {
        void* mapped_vertex = nullptr;
        vmaMapMemory(_allocator.handle(), vertex_staging.allocation(), &mapped_vertex);
        std::memcpy(mapped_vertex, vertex_data, _vertex_count * vertex_size);
        vmaUnmapMemory(_allocator.handle(), vertex_staging.allocation());

        void* mapped_index = nullptr;
        vmaMapMemory(_allocator.handle(), index_staging.allocation(), &mapped_index);
        std::memcpy(mapped_index, index_data, _index_count * index_size);
        vmaUnmapMemory(_allocator.handle(), index_staging.allocation());

        // ---------- record copy command ----------
        wk::CommandBuffer upload_cb(
            _device.handle(),
            wk::CommandBufferAllocateInfo{}.set_command_pool(_command_pool.handle()).to_vk()
        );

        VkCommandBufferBeginInfo begin_info = wk::CommandBufferBeginInfo{}
            .set_flags(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
            .to_vk();
        vkBeginCommandBuffer(upload_cb.handle(), &begin_info);

        VkBufferCopy vertex_copy = {0, 0, _vertex_count * vertex_size};
        vkCmdCopyBuffer(upload_cb.handle(), vertex_staging.handle(), _vertex_buffer.handle(), 1, &vertex_copy);

        VkBufferCopy index_copy = {0, 0, _index_count * index_size};
        vkCmdCopyBuffer(upload_cb.handle(), index_staging.handle(), _index_buffer.handle(), 1, &index_copy);

        vkEndCommandBuffer(upload_cb.handle());

        // ---------- submit & wait ----------
        VkSubmitInfo submit_info = wk::GraphicsQueueSubmitInfo{}
            .set_command_buffers(1, &upload_cb.handle())
            .to_vk();

        vkQueueSubmit(_device.graphics_queue().handle(), 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(_device.graphics_queue().handle());
    }
}

void VulkanMeshBuffer::bind(void* command_buffer) const {
    VkDeviceSize offset = 0;
    VkCommandBuffer cb = static_cast<VkCommandBuffer>(command_buffer);
    vkCmdBindVertexBuffers(cb, 0, 1, &_vertex_buffer.handle(), &offset);
    vkCmdBindIndexBuffer(cb, _index_buffer.handle(), 0, _index_type);
}

}