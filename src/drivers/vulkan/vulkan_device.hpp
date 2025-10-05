#ifndef DRIVERS_VULKAN_VULKAN_DEVICE_HPP
#define DRIVERS_VULKAN_VULKAN_DEVICE_HPP

#include "core/graphics/device.hpp"
#include "core/graphics/instance.hpp"
#include "core/graphics/renderer.hpp"
#include "core/graphics/mesh_buffer.hpp"

#include <wk/wulkan.hpp>
#include <wk/ext/glfw/surface.hpp>

namespace drivers::vulkan {

class VulkanInstance;

class VulkanDevice final : public core::graphics::Device {
public:
    VulkanDevice(const VulkanInstance& instance, const wk::ext::glfw::Surface& surface);
    ~VulkanDevice() override = default;

    void wait_idle() override;

    std::unique_ptr<core::graphics::Renderer> create_renderer(uint32_t width, uint32_t height) const override { return nullptr; } // TODO: implement this
    std::unique_ptr<core::graphics::Renderer> create_renderer(void* surface, uint32_t width, uint32_t height) const override;
    std::unique_ptr<core::graphics::MeshBuffer> create_mesh_buffer(
        const void* vertex_data, uint32_t vertex_size, uint32_t vertex_count,
        const void* index_data, uint32_t index_size, uint32_t index_count) const override;

    const wk::PhysicalDevice& physical_device() const { return _physical_device; }

    const wk::Device& device() const { return _device; }
    const wk::Allocator& allocator() const { return _allocator; }
    const wk::CommandPool& command_pool() const { return _command_pool; }
    const wk::DescriptorPool& descriptor_pool() const { return _descriptor_pool; }

    const wk::DeviceQueueFamilyIndices& queue_families() const { return _queue_families; }

    void* native_handle() const override { return static_cast<void*>(_device.handle()); }
    std::string backend_name() const override { return "Vulkan"; }

private:
    wk::PhysicalDevice _physical_device;
    wk::Device _device;

    wk::DeviceQueueFamilyIndices _queue_families;

    wk::Allocator _allocator;
    wk::CommandPool _command_pool;
    wk::DescriptorPool _descriptor_pool;
};

} // namespace drivers::vulkan

#endif // DRIVERS_VULKAN_VULKAN_RENDER_TARGET_HPP
