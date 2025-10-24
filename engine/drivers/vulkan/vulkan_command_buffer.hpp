#ifndef engine_drivers_vulkan_VULKAN_COMMAND_BUFFER_HPP
#define engine_drivers_vulkan_VULKAN_COMMAND_BUFFER_HPP

#include "engine/core/graphics/command_buffer.hpp"

#include <wk/wulkan.hpp>

namespace engine::drivers::vulkan {

class VulkanCommandBuffer final : public core::graphics::CommandBuffer {
public:
    VulkanCommandBuffer(const wk::Device& device, const wk::CommandPool& command_pool)
    : _device(device), _command_pool(command_pool)
    {
        VkCommandBufferAllocateInfo ai = wk::CommandBufferAllocateInfo{}
            .set_command_pool(command_pool.handle())
            .to_vk();
        _command_buffer = wk::CommandBuffer(
            device.handle(),
            ai
        );
    }

    void reset() override {
        vkResetCommandBuffer(_command_buffer.handle(), 0);
    }

    void begin() override {
        VkCommandBufferBeginInfo begin_info = wk::CommandBufferBeginInfo{}.to_vk();
        VkResult result = vkBeginCommandBuffer(_command_buffer.handle(), &begin_info);
        if (result != VK_SUCCESS) {
            core::debug::Logger::get_singleton().error("Failed to begin command buffer");
        }
    }

    void end() override {
        VkResult result = vkEndCommandBuffer(_command_buffer.handle());
        if (result != VK_SUCCESS) {
            core::debug::Logger::get_singleton().error("Failed to end command buffer");
        }
    }

    void set_viewport(
        float x, float y,
        float width, float height,
        float min_depth, float max_depth
    ) override {
        VkViewport viewport = wk::Viewport{}
            .set_x(x).set_y(y)
            .set_width(width)
            .set_height(height)
            .set_min_depth(min_depth).set_max_depth(max_depth)
            .to_vk();
        vkCmdSetViewport(_command_buffer.handle(), 0, 1, &viewport);
    }

    void set_scissor(
        float width, float height,
        float min_depth, float max_depth
    ) override {
        VkRect2D scissor = wk::Rect2D{}
            .set_offset({0, 0})
            .set_extent(wk::Extent{}.set_width(width).set_height(height).to_vk_extent_2d())
            .to_vk();
        vkCmdSetScissor(_command_buffer.handle(), 0, 1, &scissor);
    }

    void* native_command_buffer() const override {
        return static_cast<void*>(_command_buffer.handle());
    };

    std::string backend_name() const override { return "Vulkan"; };

private:
    const wk::Device& _device;
    const wk::CommandPool& _command_pool;

    wk::CommandBuffer _command_buffer;
};

}

#endif