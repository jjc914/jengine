#include "vulkan_descriptor_set_layout.hpp"

#include "vulkan_device.hpp"
#include "convert_vulkan.hpp"

namespace engine::drivers::vulkan {

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
    const VulkanDevice& device,
    const engine::core::graphics::DescriptorLayoutDescription& description)
    : _device(device.device())
{
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    layout_bindings.reserve(description.bindings.size());

    for (const engine::core::graphics::DescriptorLayoutBinding& b : description.bindings) {
        VkDescriptorSetLayoutBinding vk_binding{};
        vk_binding.binding = b.binding;
        vk_binding.descriptorType = ToVkDescriptorType(b.type);
        vk_binding.descriptorCount = b.count;
        vk_binding.stageFlags = ToVkShaderStageFlags(b.visibility);
        vk_binding.pImmutableSamplers = nullptr;
        layout_bindings.push_back(vk_binding);
    }

    _layout = wk::DescriptorSetLayout(
        _device.handle(),
        wk::DescriptorSetLayoutCreateInfo{}
            .set_bindings(static_cast<uint32_t>(layout_bindings.size()), layout_bindings.data())
            .to_vk()
    );
}

} // namespace engine::drivers::vulkan
