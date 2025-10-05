#include "vk_device.hpp"

#include "vk_renderer.hpp"
#include "vk_mesh_buffer.hpp"

namespace core::graphics::vulkan {

VulkanDevice::VulkanDevice(const VulkanInstance& instance, const wk::ext::glfw::Surface& surface) {
    // ---------- device ----------
    VkPhysicalDeviceFeatures2 physical_device_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    _physical_device = wk::PhysicalDevice(static_cast<VkInstance>(instance.native_handle()), surface.handle(),
        wk::GetRequiredDeviceExtensions(), &physical_device_features,
        &wk::DefaultPhysicalDeviceFeatureScorer
    );
    _queue_families = _physical_device.queue_family_indices();

    const float QUEUE_PRIORITY = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos = {
        wk::DeviceQueueCreateInfo{}
            .set_queue_family_index(_queue_families.graphics_family.value())
            .set_queue_count(1)
            .set_p_queue_priorities(&QUEUE_PRIORITY)
            .to_vk()
    };
    if (_queue_families.is_unique()) {
        queue_create_infos.push_back(
            wk::DeviceQueueCreateInfo{}
                .set_queue_family_index(_queue_families.present_family.value())
                .set_queue_count(1)
                .set_p_queue_priorities(&QUEUE_PRIORITY)
                .to_vk());
    }

    _device = wk::Device(_physical_device.handle(), _queue_families,
        wk::DeviceCreateInfo{}
            .set_p_enabled_features(&_physical_device.features())
            .set_enabled_extensions(_physical_device.extensions().size(),
                                    _physical_device.extensions().data())
            .set_queue_create_infos(queue_create_infos.size(), queue_create_infos.data())
            .to_vk());

    // ---------- command pool ----------
    _command_pool = wk::CommandPool(_device.handle(),
        wk::CommandPoolCreateInfo{}
            .set_flags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                    VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)
            .set_queue_family_index(_queue_families.graphics_family.value())
            .to_vk()
    );

    // ---------- descriptor pool ----------
    VkDescriptorPoolSize pool_sizes[] = {
        wk::DescriptorPoolSize{}
            .set_type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            .set_descriptor_count(1024)
            .to_vk(),
        wk::DescriptorPoolSize{}
            .set_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            .set_descriptor_count(1024)
            .to_vk(),
        wk::DescriptorPoolSize{}
            .set_type(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .set_descriptor_count(256)
            .to_vk()
    };

    _descriptor_pool = wk::DescriptorPool(_device.handle(),
        wk::DescriptorPoolCreateInfo{}
            .set_max_sets(1024)  // Safe upper bound for all sets combined
            .set_flags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .set_pool_sizes(static_cast<uint32_t>(std::size(pool_sizes)), pool_sizes)
            .to_vk()
    );

    // ---------- allocator ----------
    VmaVulkanFunctions vulkan_functions{};
    _allocator = wk::Allocator(
        wk::AllocatorCreateInfo{}
            .set_vulkan_api_version(VK_API_VERSION_1_3)
            .set_instance(static_cast<VkInstance>(instance.native_handle()))
            .set_physical_device(_physical_device.handle())
            .set_device(_device.handle())
            .set_p_vulkan_functions(&vulkan_functions)
            .to_vk()
    );
}

void VulkanDevice::wait_idle() {
    vkDeviceWaitIdle(_device.handle());
}

std::unique_ptr<Renderer> VulkanDevice::create_renderer(void* surface, uint32_t width, uint32_t height) const {
    return std::make_unique<VulkanRenderer>(
        *this, *static_cast<wk::ext::glfw::Surface*>(surface),
        width, height
    );
}

std::unique_ptr<MeshBuffer> VulkanDevice::create_mesh_buffer(
    const void* vertex_data, uint32_t vertex_size, uint32_t vertex_count,
    const void* index_data, uint32_t index_size, uint32_t index_count) const 
{
    return std::make_unique<VulkanMeshBuffer>(
        *this,
        vertex_data, vertex_size, vertex_count,
        index_data, index_size, index_count
    );
};

}