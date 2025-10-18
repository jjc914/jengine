#include "vulkan_texture_render_target.hpp"

#include "vulkan_device.hpp"
#include "convert_vulkan.hpp"

#include "engine/core/debug/logger.hpp"

#include <wk/extent.hpp>
#include <algorithm>
#include <iostream>

namespace engine::drivers::vulkan {

VulkanTextureRenderTarget::VulkanTextureRenderTarget(
    const VulkanDevice& device,
    const core::graphics::Pipeline& pipeline,
    uint32_t width,
    uint32_t height,
    bool use_depth,
    uint32_t max_in_flight)
    : VulkanRenderTarget(device.device()),
      _command_pool(device.command_pool()),
      _allocator(device.allocator())
{
    _width = width;
    _height = height;
    _max_in_flight = max_in_flight;
    _frame_count = max_in_flight;

    _render_pass = static_cast<VkRenderPass>(pipeline.native_render_pass());

    // formats
    _color_format = ToVkFormat(pipeline.color_format());
    _depth_format = use_depth ? ToVkFormat(pipeline.depth_format()) : VK_FORMAT_UNDEFINED;
    
    _color_images.clear();
    _color_image_views.clear();
    _depth_images.clear();
    _depth_image_views.clear();
    _framebuffers.clear();
    _color_images.reserve(_max_in_flight);
    _color_image_views.reserve(_max_in_flight);
    _depth_images.reserve(_max_in_flight);
    _depth_image_views.reserve(_max_in_flight);
    _framebuffers.reserve(_max_in_flight);

    for (size_t i = 0; i < _max_in_flight; ++i) {
        // color image
        _color_images.emplace_back(
            _allocator.handle(),
            wk::ImageCreateInfo{}
                .set_image_type(VK_IMAGE_TYPE_2D)
                .set_format(_color_format)
                .set_extent(wk::Extent{_width, _height, 1}.to_vk())
                .set_mip_levels(1)
                .set_array_layers(1)
                .set_samples(VK_SAMPLE_COUNT_1_BIT)
                .set_tiling(VK_IMAGE_TILING_OPTIMAL)
                .set_usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
                .set_initial_layout(VK_IMAGE_LAYOUT_UNDEFINED)
                .to_vk(),
            wk::AllocationCreateInfo{}
                .set_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                .to_vk()
        );
        
        // color view
        _color_image_views.emplace_back(
            _device.handle(),
            wk::ImageViewCreateInfo{}
                .set_image(_color_images[_color_images.size() - 1].handle())
                .set_view_type(VK_IMAGE_VIEW_TYPE_2D)
                .set_format(_color_format)
                .set_components(wk::ComponentMapping::identity().to_vk())
                .set_subresource_range(
                    wk::ImageSubresourceRange{}
                        .set_aspect_mask(VK_IMAGE_ASPECT_COLOR_BIT)
                        .set_base_mip_level(0)
                        .set_level_count(1)
                        .set_base_array_layer(0)
                        .set_layer_count(1)
                        .to_vk()
                )
                .to_vk()
        );
    }

    // depth image
    if (use_depth) {
        for (size_t i = 0; i < _max_in_flight; ++i) {
            _depth_images.emplace_back(
                _allocator.handle(),
                wk::ImageCreateInfo{}
                    .set_image_type(VK_IMAGE_TYPE_2D)
                    .set_format(_depth_format)
                    .set_extent(wk::Extent{_width, _height, 1}.to_vk())
                    .set_mip_levels(1)
                    .set_array_layers(1)
                    .set_samples(VK_SAMPLE_COUNT_1_BIT)
                    .set_tiling(VK_IMAGE_TILING_OPTIMAL)
                    .set_usage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
                    .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
                    .set_initial_layout(VK_IMAGE_LAYOUT_UNDEFINED)
                    .to_vk(),
                wk::AllocationCreateInfo{}
                    .set_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                    .to_vk()
            );

            _depth_image_views.emplace_back(
                _device.handle(),
                wk::ImageViewCreateInfo{}
                    .set_image(_depth_images[_depth_images.size() - 1].handle())
                    .set_view_type(VK_IMAGE_VIEW_TYPE_2D)
                    .set_format(_depth_format)
                    .set_components(wk::ComponentMapping::identity().to_vk())
                    .set_subresource_range(
                        wk::ImageSubresourceRange{}
                            .set_aspect_mask(wk::GetAspectFlags(_depth_format))
                            .set_base_mip_level(0)
                            .set_level_count(1)
                            .set_base_array_layer(0)
                            .set_layer_count(1)
                            .to_vk()
                    )
                    .to_vk()
            );
        }
    }

    // framebuffer
    for (size_t i = 0; i < _max_in_flight; ++i) {
        std::vector<VkImageView> attachments;
        attachments.push_back(_color_image_views[i].handle());
        if (use_depth)
            attachments.push_back(_depth_image_views[i].handle());

        _framebuffers.emplace_back(
            _device.handle(),
            wk::FramebufferCreateInfo{}
                .set_render_pass(_render_pass)
                .set_attachments(static_cast<uint32_t>(attachments.size()), attachments.data())
                .set_extent({_width, _height})
                .set_layers(1)
                .to_vk()
        );
    }

    // sync and render command buffers
    _command_buffers.clear();
    _image_available_semaphores.clear();
    _render_finished_semaphores.clear();
    _in_flight_fences.clear();

    _command_buffers.reserve(_max_in_flight);
    _image_available_semaphores.reserve(_max_in_flight);
    _render_finished_semaphores.reserve(_max_in_flight);
    _in_flight_fences.reserve(_max_in_flight);

    for (size_t i = 0; i < _max_in_flight; ++i) {
        _command_buffers.emplace_back(_device.handle(),
            wk::CommandBufferAllocateInfo{}
                .set_command_pool(device.command_pool().handle())
                .to_vk());

        _image_available_semaphores.emplace_back(_device.handle(), wk::SemaphoreCreateInfo{}.to_vk());
        _render_finished_semaphores.emplace_back(_device.handle(), wk::SemaphoreCreateInfo{}.to_vk());
        _in_flight_fences.emplace_back(_device.handle(),
            wk::FenceCreateInfo{}.set_flags(VK_FENCE_CREATE_SIGNALED_BIT).to_vk());
    }
}

void* VulkanTextureRenderTarget::begin_frame(const core::graphics::Pipeline& pipeline) {
    vkWaitForFences(_device.handle(), 1, &_in_flight_fences[_frame_index].handle(), VK_TRUE, UINT64_MAX);
    vkResetFences(_device.handle(), 1, &_in_flight_fences[_frame_index].handle());
    vkResetCommandBuffer(_command_buffers[_frame_index].handle(), 0);

    VkCommandBufferBeginInfo cb_begin_info = wk::CommandBufferBeginInfo{}.to_vk();
    VkResult result = vkBeginCommandBuffer(_command_buffers[_frame_index].handle(), &cb_begin_info);
    if (result != VK_SUCCESS) {
        core::debug::Logger::get_singleton().error("Could not begin command buffer");
        return nullptr;
    }

    VkClearValue color_clear = wk::ClearValue{}.set_color(0, 0, 0).to_vk();
    VkClearValue depth_clear = wk::ClearValue{}.set_depth_stencil(1.0f, 0).to_vk();

    std::vector<VkClearValue> clear_values = { color_clear };
    if (_depth_image_views[_frame_index].handle() != VK_NULL_HANDLE) clear_values.emplace_back(depth_clear);

    VkRenderPassBeginInfo rp_begin_info = wk::RenderPassBeginInfo{}
        .set_render_pass(_render_pass)
        .set_framebuffer(_framebuffers[_frame_index].handle())
        .set_render_area({ { 0, 0 }, { _width, _height } })
        .set_clear_values(static_cast<uint32_t>(clear_values.size()), clear_values.data())
        .to_vk();

    vkCmdBeginRenderPass(_command_buffers[_frame_index].handle(), &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = wk::Viewport{}
        .set_x(0.0f).set_y(0.0f)
        .set_width(static_cast<float>(_width))
        .set_height(static_cast<float>(_height))
        .set_min_depth(0.0f).set_max_depth(1.0f)
        .to_vk();

    VkRect2D scissor = wk::Rect2D{}
        .set_offset({0, 0})
        .set_extent({_width, _height})
        .to_vk();

    vkCmdSetViewport(_command_buffers[_frame_index].handle(), 0, 1, &viewport);
    vkCmdSetScissor(_command_buffers[_frame_index].handle(), 0, 1, &scissor);

    vkCmdBindPipeline(_command_buffers[_frame_index].handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<VkPipeline>(pipeline.native_pipeline()));

    return static_cast<void*>(_command_buffers[_frame_index].handle());
}

void VulkanTextureRenderTarget::submit_draws(uint32_t index_count) {
    vkCmdDrawIndexed(_command_buffers[_frame_index].handle(), index_count, 1, 0, 0, 0);
}

void VulkanTextureRenderTarget::end_frame() {
    vkCmdEndRenderPass(_command_buffers[_frame_index].handle());
    VkResult result = vkEndCommandBuffer(_command_buffers[_frame_index].handle());
    if (result != VK_SUCCESS) {
        core::debug::Logger::get_singleton().error("Failed to end command buffer");
    }

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    VkCommandBuffer cmd = _command_buffers[_frame_index].handle();
    submit_info.pCommandBuffers = &cmd;

    result = vkQueueSubmit(_device.graphics_queue().handle(), 1, &submit_info, _in_flight_fences[_frame_index].handle());
    if (result != VK_SUCCESS) {
        core::debug::Logger::get_singleton().error("Failed to submit queue");
    }

    _frame_index = (_frame_index + 1) % _max_in_flight;
}

void VulkanTextureRenderTarget::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    _width = width;
    _height = height;
    rebuild();
}

std::vector<uint32_t> VulkanTextureRenderTarget::copy_color_to_cpu(uint32_t frame_index) {
    ENGINE_ASSERT(frame_index < _color_images.size(), "Invalid frame index");

    vkWaitForFences(_device.handle(), 1, &_in_flight_fences[frame_index].handle(), VK_TRUE, UINT64_MAX);

    const uint32_t bpp = BytesPerPixel(_color_format);
    const VkDeviceSize image_size = static_cast<VkDeviceSize>(_width) * _height * bpp;

    // create staging buffer
    wk::Buffer staging_buffer(_allocator.handle(),
        wk::BufferCreateInfo{}
            .set_size(image_size)
            .set_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
            .to_vk(),
        wk::AllocationCreateInfo{}
            .set_usage(VMA_MEMORY_USAGE_CPU_ONLY)
            .to_vk()
    );

    // allocate one time command buffer
    wk::CommandBuffer command_buffer(_device.handle(),
        wk::CommandBufferAllocateInfo{}
            .set_command_pool(_command_pool.handle())
            .set_level(VK_COMMAND_BUFFER_LEVEL_PRIMARY)
            .set_command_buffer_count(1)
            .to_vk()
    );

    VkCommandBufferBeginInfo begin_info = wk::CommandBufferBeginInfo{}
        .set_flags(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
        .to_vk();
    vkBeginCommandBuffer(command_buffer.handle(), &begin_info);

    VkImage src_image = _color_images[frame_index].handle();

    // transition layout
    VkImageMemoryBarrier to_transfer = wk::ImageMemoryBarrier{}
        .set_old_layout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .set_new_layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        .set_src_queue_family_index(VK_QUEUE_FAMILY_IGNORED)
        .set_dst_queue_family_index(VK_QUEUE_FAMILY_IGNORED)
        .set_image(src_image)
        .set_aspect(VK_IMAGE_ASPECT_COLOR_BIT)
        .set_levels(0, 1)
        .set_layers(0, 1)
        .set_src_access(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        .set_dst_access(VK_ACCESS_TRANSFER_READ_BIT)
        .to_vk();

    vkCmdPipelineBarrier(command_buffer.handle(),
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &to_transfer
    );

    // TODO: add this to wulkan
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;   // tightly packed
    region.bufferImageHeight = 0; // tightly packed
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { _width, _height, 1 };

    vkCmdCopyImageToBuffer(command_buffer.handle(),
        src_image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        staging_buffer.handle(),
        1, &region
    );

    // transfer image layout to restore
    VkImageMemoryBarrier back_to_color = to_transfer;
    back_to_color.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    back_to_color.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    back_to_color.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    back_to_color.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(command_buffer.handle(),
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &back_to_color
    );

    vkEndCommandBuffer(command_buffer.handle());

    // submit and wait
    wk::Fence fence(_device.handle(), wk::FenceCreateInfo{}.to_vk());

    VkSubmitInfo submit = wk::SubmitInfo{}
        .set_command_buffers(1, &command_buffer.handle())
        .to_vk();

    vkQueueSubmit(_device.graphics_queue().handle(), 1, &submit, fence.handle());
    vkWaitForFences(_device.handle(), 1, &fence.handle(), VK_TRUE, UINT64_MAX);

    // map and copy to cpu
    std::vector<uint32_t> out;
    out.resize(static_cast<size_t>(_width) * _height);

    void* mapped = nullptr;
    vmaMapMemory(_allocator.handle(), staging_buffer.allocation(), &mapped);
    std::memcpy(out.data(), mapped, static_cast<size_t>(image_size));
    vmaUnmapMemory(_allocator.handle(), staging_buffer.allocation());

    return out;
}

void VulkanTextureRenderTarget::rebuild() {
    vkDeviceWaitIdle(_device.handle());

    bool use_depth = _depth_images.size() > 0;

    _color_images.clear();
    _color_image_views.clear();
    _depth_images.clear();
    _depth_image_views.clear();
    _framebuffers.clear();
    _color_images.reserve(_max_in_flight);
    _color_image_views.reserve(_max_in_flight);
    _depth_images.reserve(_max_in_flight);
    _depth_image_views.reserve(_max_in_flight);
    _framebuffers.reserve(_max_in_flight);

    for (size_t i = 0; i < _max_in_flight; ++i) {
        // color image
        _color_images.emplace_back(
            _allocator.handle(),
            wk::ImageCreateInfo{}
                .set_image_type(VK_IMAGE_TYPE_2D)
                .set_format(_color_format)
                .set_extent(wk::Extent{_width, _height, 1}.to_vk())
                .set_mip_levels(1)
                .set_array_layers(1)
                .set_samples(VK_SAMPLE_COUNT_1_BIT)
                .set_tiling(VK_IMAGE_TILING_OPTIMAL)
                .set_usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
                .set_initial_layout(VK_IMAGE_LAYOUT_UNDEFINED)
                .to_vk(),
            wk::AllocationCreateInfo{}
                .set_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                .to_vk()
        );
        
        // color view
        _color_image_views.emplace_back(
            _device.handle(),
            wk::ImageViewCreateInfo{}
                .set_image(_color_images[_color_images.size() - 1].handle())
                .set_view_type(VK_IMAGE_VIEW_TYPE_2D)
                .set_format(_color_format)
                .set_components(wk::ComponentMapping::identity().to_vk())
                .set_subresource_range(
                    wk::ImageSubresourceRange{}
                        .set_aspect_mask(VK_IMAGE_ASPECT_COLOR_BIT)
                        .set_base_mip_level(0)
                        .set_level_count(1)
                        .set_base_array_layer(0)
                        .set_layer_count(1)
                        .to_vk()
                )
                .to_vk()
        );
    }

    // depth image
    if (use_depth) {
        for (size_t i = 0; i < _max_in_flight; ++i) {
            _depth_images.emplace_back(
                _allocator.handle(),
                wk::ImageCreateInfo{}
                    .set_image_type(VK_IMAGE_TYPE_2D)
                    .set_format(_depth_format)
                    .set_extent(wk::Extent{_width, _height, 1}.to_vk())
                    .set_mip_levels(1)
                    .set_array_layers(1)
                    .set_samples(VK_SAMPLE_COUNT_1_BIT)
                    .set_tiling(VK_IMAGE_TILING_OPTIMAL)
                    .set_usage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
                    .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
                    .set_initial_layout(VK_IMAGE_LAYOUT_UNDEFINED)
                    .to_vk(),
                wk::AllocationCreateInfo{}
                    .set_usage(VMA_MEMORY_USAGE_GPU_ONLY)
                    .to_vk()
            );

            _depth_image_views.emplace_back(
                _device.handle(),
                wk::ImageViewCreateInfo{}
                    .set_image(_depth_images[_depth_images.size() - 1].handle())
                    .set_view_type(VK_IMAGE_VIEW_TYPE_2D)
                    .set_format(_depth_format)
                    .set_components(wk::ComponentMapping::identity().to_vk())
                    .set_subresource_range(
                        wk::ImageSubresourceRange{}
                            .set_aspect_mask(wk::GetAspectFlags(_depth_format))
                            .set_base_mip_level(0)
                            .set_level_count(1)
                            .set_base_array_layer(0)
                            .set_layer_count(1)
                            .to_vk()
                    )
                    .to_vk()
            );
        }
    }

    // framebuffer
    for (size_t i = 0; i < _max_in_flight; ++i) {
        std::vector<VkImageView> attachments;
        attachments.push_back(_color_image_views[i].handle());
        if (use_depth)
            attachments.push_back(_depth_image_views[i].handle());

        _framebuffers.emplace_back(
            _device.handle(),
            wk::FramebufferCreateInfo{}
                .set_render_pass(_render_pass)
                .set_attachments(static_cast<uint32_t>(attachments.size()), attachments.data())
                .set_extent({_width, _height})
                .set_layers(1)
                .to_vk()
        );
    }
}

} // namespace engine::drivers::vulkan
