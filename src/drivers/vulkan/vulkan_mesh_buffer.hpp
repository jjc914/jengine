#ifndef DRIVERS_VULKAN_VULKAN_MESH_BUFFER_HPP
#define DRIVERS_VULKAN_VULKAN_MESH_BUFFER_HPP

#include "core/graphics/mesh_buffer.hpp"

#include <wk/wulkan.hpp>

namespace drivers::vulkan {

class VulkanDevice;

class VulkanMeshBuffer : public core::graphics::MeshBuffer {
public:
    VulkanMeshBuffer(const VulkanDevice& device,
                     const void* vertex_data, uint32_t vertex_size, uint32_t vertex_count,
                     const void* index_data, uint32_t index_size, uint32_t index_count);
    ~VulkanMeshBuffer() override = default;

    void bind(void* command_buffer) const override;

    uint32_t vertex_count() const override { return _vertex_count; }
    uint32_t index_count() const override { return _index_count; }

    void* vertex_buffer_handle() const override { return (void*)_vertex_buffer.handle(); }
    void* index_buffer_handle() const override { return (void*)_index_buffer.handle(); }

private:
    const wk::Device& _device;
    const wk::Allocator& _allocator;
    const wk::CommandPool& _command_pool;

    wk::Buffer _vertex_buffer;
    wk::Buffer _index_buffer;

    uint32_t _vertex_count;
    uint32_t _index_count;
    VkIndexType _index_type;
};

} // namespace drivers::vulkan

#endif // DRIVERS_VULKAN_VULKAN_MESH_BUFFER_HPP
