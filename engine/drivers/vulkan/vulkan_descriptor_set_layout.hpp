#ifndef engine_drivers_vulkan_VULKAN_DESCRIPTOR_SET_LAYOUT_HPP
#define engine_drivers_vulkan_VULKAN_DESCRIPTOR_SET_LAYOUT_HPP

#include "engine/core/graphics/descriptor_set_layout.hpp"
#include "engine/core/graphics/descriptor_types.hpp"

#include <wk/wulkan.hpp>

namespace engine::drivers::vulkan {

class VulkanDevice;

class VulkanDescriptorSetLayout final : public engine::core::graphics::DescriptorSetLayout {
public:
    VulkanDescriptorSetLayout(const VulkanDevice& device,
        const engine::core::graphics::DescriptorLayoutDescription& description
    );

    VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& other) = default;
    VulkanDescriptorSetLayout& operator=(VulkanDescriptorSetLayout&& other) = default;

    VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
    VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

    ~VulkanDescriptorSetLayout() override = default;

    void* native_descriptor_set_layout() const override { return static_cast<void*>(_layout.handle()); }
    std::string backend_name() const { return "Vulkan"; }

private:
    const wk::Device& _device;

    wk::DescriptorSetLayout _layout;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_DESCRIPTOR_SET_LAYOUT_HPP
