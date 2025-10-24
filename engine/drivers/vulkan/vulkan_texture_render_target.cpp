#include "vulkan_texture_render_target.hpp"

#include "vulkan_device.hpp"
#include "vulkan_command_buffer.hpp"
#include "convert_vulkan.hpp"

#include "engine/core/debug/logger.hpp"

#include <wk/extent.hpp>
#include <algorithm>
#include <iostream>

namespace engine::drivers::vulkan {

VulkanTextureRenderTarget::VulkanTextureRenderTarget(
    const VulkanDevice& device,
    const core::graphics::Pipeline& pipeline,
    const core::graphics::AttachmentInfo& attachments,
    uint32_t max_in_flight)
    : VulkanRenderTarget(device.device()),
      _device(device.device()),
      _command_pool(device.command_pool()),
      _allocator(device.allocator()),
      _graphics_queue(device.graphics_queue()),
      _color_textures(attachments.color_textures),
      _depth_texture(attachments.depth_texture)
{
    _max_in_flight = max_in_flight;
    _frame_count = max_in_flight;

    _render_pass = static_cast<VkRenderPass>(pipeline.native_render_pass());

    // extract formats from first attachments (assumed consistent)
    if (!_color_textures.empty())
        _color_format = ToVkFormat(_color_textures.front()->format());
    _depth_format = _depth_texture ? ToVkFormat(_depth_texture->format()) : VK_FORMAT_UNDEFINED;

    // create framebuffer
    rebuild();

    // sync and command buffers
    _command_buffers.clear();
    _image_available_semaphores.clear();
    _render_finished_semaphores.clear();
    _in_flight_fences.clear();

    _command_buffers.reserve(_max_in_flight);
    _image_available_semaphores.reserve(_max_in_flight);
    _render_finished_semaphores.reserve(_max_in_flight);
    _in_flight_fences.reserve(_max_in_flight);

    for (size_t i = 0; i < _max_in_flight; ++i) {
        _command_buffers.emplace_back(_device, _command_pool);

        _image_available_semaphores.emplace_back(_device.handle(), wk::SemaphoreCreateInfo{}.to_vk());
        _render_finished_semaphores.emplace_back(_device.handle(), wk::SemaphoreCreateInfo{}.to_vk());
        _in_flight_fences.emplace_back(_device.handle(),
            wk::FenceCreateInfo{}.set_flags(VK_FENCE_CREATE_SIGNALED_BIT).to_vk());
    }
}

core::graphics::CommandBuffer* VulkanTextureRenderTarget::begin_frame(const core::graphics::Pipeline& pipeline,
    glm::vec4 color_clear,
    glm::vec2 depth_clear)
{
    vkWaitForFences(_device.handle(), 1, &_in_flight_fences[_frame_index].handle(), VK_TRUE, UINT64_MAX);
    vkResetFences(_device.handle(), 1, &_in_flight_fences[_frame_index].handle());
    _command_buffers[_frame_index].reset();

    _command_buffers[_frame_index].begin();

    // clear values (color + optional depth)
    std::vector<VkClearValue> clear_values;
    clear_values.push_back(wk::ClearValue{}.set_color(color_clear.r, color_clear.g, color_clear.b, color_clear.a).to_vk());
    if (_depth_texture)
        clear_values.push_back(wk::ClearValue{}.set_depth_stencil(depth_clear.r, depth_clear.g).to_vk());

    VkRenderPassBeginInfo rp_begin_info = wk::RenderPassBeginInfo{}
        .set_render_pass(static_cast<VkRenderPass>(pipeline.native_render_pass()))
        .set_framebuffer(_framebuffers[_frame_index].handle())
        .set_render_area({ { 0, 0 }, _extent })
        .set_clear_values(static_cast<uint32_t>(clear_values.size()), clear_values.data())
        .to_vk();

    vkCmdBeginRenderPass(static_cast<VkCommandBuffer>(_command_buffers[_frame_index].native_command_buffer()),
        &rp_begin_info,
        VK_SUBPASS_CONTENTS_INLINE
    );

    return &_command_buffers[_frame_index];
}

void VulkanTextureRenderTarget::end_frame() {
    vkCmdEndRenderPass(static_cast<VkCommandBuffer>(_command_buffers[_frame_index].native_command_buffer()));

    _command_buffers[_frame_index].end();

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    VkCommandBuffer cmd = static_cast<VkCommandBuffer>(_command_buffers[_frame_index].native_command_buffer());
    submit_info.pCommandBuffers = &cmd;

    if (vkQueueSubmit(_graphics_queue.handle(), 1, &submit_info, _in_flight_fences[_frame_index].handle()) != VK_SUCCESS)
        core::debug::Logger::get_singleton().error("Failed to submit draw queue");

    _frame_index = (_frame_index + 1) % _max_in_flight;
}

void VulkanTextureRenderTarget::rebuild() {
    vkDeviceWaitIdle(_device.handle());

    // update extent
    if (!_color_textures.empty()) {
        _extent = { _color_textures.front()->width(), _color_textures.front()->height() };
    } else if (_depth_texture) {
        _extent = { _depth_texture->width(), _depth_texture->height() };
    } else {
        _extent = { 1, 1 };
    }

    _framebuffers.clear();
    _framebuffers.reserve(_max_in_flight);

    for (size_t i = 0; i < _max_in_flight; ++i) {
        std::vector<VkImageView> attachments;
        for (const core::graphics::Texture* tex : _color_textures)
            attachments.push_back(static_cast<VkImageView>(tex->native_image_view()));
        if (_depth_texture)
            attachments.push_back(static_cast<VkImageView>(_depth_texture->native_image_view()));

        _framebuffers.emplace_back(
            _device.handle(),
            wk::FramebufferCreateInfo{}
                .set_render_pass(_render_pass)
                .set_attachments(static_cast<uint32_t>(attachments.size()), attachments.data())
                .set_extent(_extent)
                .set_layers(1)
                .to_vk()
        );
    }
}

} // namespace engine::drivers::vulkan
