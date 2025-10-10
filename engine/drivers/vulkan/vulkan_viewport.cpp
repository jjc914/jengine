#include "vulkan_viewport.hpp"

#include "vulkan_device.hpp"
#include "convert_vulkan.hpp"

#include <wk/extent.hpp>
#include <wk/ext/glfw/surface.hpp>
#include <algorithm>
#include <iostream>

namespace engine::drivers::vulkan {

VulkanViewport::VulkanViewport(
    const VulkanDevice& device,
    const core::window::Window& window,
    const core::graphics::Pipeline& pipeline,
    uint32_t width,
    uint32_t height,
    uint32_t max_in_flight)
    : VulkanRenderTarget(device.device()), _physical_device(device.physical_device()), _surface(window.surface()),
      _allocator(device.allocator()), _command_pool(device.command_pool()), _descriptor_pool(device.descriptor_pool()),
      _graphics_queue(device.graphics_queue()), _render_pass(static_cast<VkRenderPass>(pipeline.native_render_pass())),
      _current_frame(0)
{
    _width = width;
    _height = height;
    _max_in_flight = max_in_flight;

    // get present queue
    uint32_t present_family;
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device.handle(), &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device.handle(), &queue_family_count, queue_families.data());

    for (uint32_t i = 0; i < queue_family_count; ++i) {
        VkBool32 supports_present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device.handle(), i, window.surface().handle(), &supports_present);
        if (supports_present) {
            present_family = i;
            break;
        }
    }

    _present_queue = wk::Queue(device.device().handle(), present_family);

    wk::PhysicalDeviceSurfaceSupport support = wk::GetPhysicalDeviceSurfaceSupport(_physical_device.handle(), _surface.handle());
    _present_mode = wk::ChooseSurfacePresentationMode(support.present_modes);
    _image_extent = wk::ChooseSurfaceExtent(_width, _height, support.capabilities);
    _color_format = ToVkFormat(pipeline.present_color_format());
    _color_space = ToVkColorSpace(device.present_color_space());
    _depth_format = ToVkFormat(device.depth_format());

    uint32_t image_count = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount != 0 &&
        image_count > support.capabilities.maxImageCount) {
        image_count = support.capabilities.maxImageCount;
    }
    _min_image_count = image_count;

    // swapchain
    _queue_family_indices.clear();
    _queue_family_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    if (_graphics_queue.family_index() != present_family) {
        _queue_family_indices = { _graphics_queue.family_index(), present_family };
        _queue_family_sharing_mode = VK_SHARING_MODE_CONCURRENT;
    }

    _swapchain = wk::Swapchain(_device.handle(),
        wk::SwapchainCreateInfo{}
            .set_surface(_surface.handle())
            .set_present_mode(_present_mode)
            .set_min_image_count(_min_image_count)
            .set_image_extent(_image_extent)
            .set_image_format(_color_format)
            .set_image_color_space(_color_space)
            .set_image_array_layers(1)
            .set_image_usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            .set_image_sharing_mode(_queue_family_sharing_mode)
            .set_queue_family_indices(static_cast<uint32_t>(_queue_family_indices.size()), _queue_family_indices.empty() ? nullptr : _queue_family_indices.data())
            .to_vk()
    );

    _images.clear();
    _image_views.clear();
    _framebuffers.clear();
    _images.reserve(_swapchain.image_views().size());
    _image_views.reserve(_swapchain.image_views().size());
    _framebuffers.reserve(_swapchain.image_views().size());

    for (size_t i = 0; i < _swapchain.image_views().size(); ++i) {
        // depth image
        _images.emplace_back(
            _allocator.handle(),
            wk::ImageCreateInfo{}
                .set_image_type(VK_IMAGE_TYPE_2D)
                .set_format(_depth_format)
                .set_extent(wk::Extent(_image_extent).to_vk())
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

        // depth view
        _image_views.emplace_back(
            _device.handle(),
            wk::ImageViewCreateInfo{}
                .set_image(_images.back().handle())
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

        // framebuffer (color and depth)
        std::vector<VkImageView> attachments = {
            _swapchain.image_views()[i],
            _image_views.back().handle()
        };

        _framebuffers.emplace_back(
            _device.handle(),
            wk::FramebufferCreateInfo{}
                .set_render_pass(static_cast<VkRenderPass>(pipeline.native_render_pass()))
                .set_attachments(static_cast<uint32_t>(attachments.size()), attachments.data())
                .set_extent(_swapchain.extent())
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
                .set_command_pool(_command_pool.handle())
                .to_vk());
        _image_available_semaphores.emplace_back(_device.handle(), 
            wk::SemaphoreCreateInfo{}
                .to_vk());
        _render_finished_semaphores.emplace_back(_device.handle(), 
            wk::SemaphoreCreateInfo{}
                .to_vk());
        _in_flight_fences.emplace_back(_device.handle(),
            wk::FenceCreateInfo{}
                .set_flags(VK_FENCE_CREATE_SIGNALED_BIT)
                .to_vk());
    }
}

void* VulkanViewport::begin_frame(const core::graphics::Pipeline& pipeline) {
    vkWaitForFences(_device.handle(), 1, &_in_flight_fences[_current_frame].handle(), VK_TRUE, UINT64_MAX);

    // acquire next image
    VkResult result;
    result = vkAcquireNextImageKHR(_device.handle(), _swapchain.handle(), UINT32_MAX, 
        _image_available_semaphores[_current_frame].handle(), VK_NULL_HANDLE, &_available_image_index
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        rebuild();
        return nullptr;
    } else if (result != VK_SUCCESS) {
        std::cerr << "failed to acquire swapchain image" << std::endl;
        return nullptr;
    }

    vkResetFences(_device.handle(), 1, &_in_flight_fences[_current_frame].handle());
    vkResetCommandBuffer(_command_buffers[_current_frame].handle(), 0);

    VkCommandBufferBeginInfo cb_begin_info = wk::CommandBufferBeginInfo{}.to_vk();
    result = vkBeginCommandBuffer(_command_buffers[_current_frame].handle(), &cb_begin_info);
    if (result != VK_SUCCESS) {
        std::cerr << "could not begin command buffer" << std::endl;
        return nullptr;
    }

    // clear screen
    VkClearValue color_clear = wk::ClearValue{}.set_color(0.0f, 0.0f, 0.0f).to_vk();
    VkClearValue depth_clear = wk::ClearValue{}.set_depth_stencil(1.0f, 0).to_vk();
    
    std::vector<VkClearValue> clear_values = { color_clear, depth_clear };

    VkRenderPassBeginInfo rp_begin_info = wk::RenderPassBeginInfo{}
        .set_render_pass(static_cast<VkRenderPass>(pipeline.native_render_pass()))
        .set_framebuffer(_framebuffers[_available_image_index].handle())
        .set_render_area({ { 0, 0 }, _swapchain.extent() })
        .set_clear_values(static_cast<uint32_t>(clear_values.size()), clear_values.data())
        .to_vk();

    vkCmdBeginRenderPass(_command_buffers[_current_frame].handle(), 
        &rp_begin_info,
        VK_SUBPASS_CONTENTS_INLINE
    );

    // build viewport/scissor
    VkViewport viewport = wk::Viewport{}
        .set_x(0.0f).set_y(0.0f)
        .set_width(static_cast<float>(_width))
        .set_height(static_cast<float>(_height))
        .set_min_depth(0.0f).set_max_depth(1.0f)
        .to_vk();
    VkRect2D scissor = wk::Rect2D{}
        .set_offset({0,0})
        .set_extent(_swapchain.extent())
        .to_vk();
    vkCmdSetViewport(_command_buffers[_current_frame].handle(), 0, 1, &viewport);
    vkCmdSetScissor(_command_buffers[_current_frame].handle(), 0, 1, &scissor);

    vkCmdBindPipeline(_command_buffers[_current_frame].handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<VkPipeline>(pipeline.native_pipeline()));

    return static_cast<void*>(_command_buffers[_current_frame].handle());
}

void VulkanViewport::submit_draws(uint32_t index_count) {
    vkCmdDrawIndexed(_command_buffers[_current_frame].handle(), index_count, 1, 0, 0, 0);
}

void VulkanViewport::end_frame() {
    vkCmdEndRenderPass(_command_buffers[_current_frame].handle());
    VkResult result = vkEndCommandBuffer(_command_buffers[_current_frame].handle());
    if (result != VK_SUCCESS) {
        std::cerr << "failed to end command buffer" << std::endl;
    }

    std::vector<VkCommandBuffer> gq_command_buffers = { _command_buffers[_current_frame].handle() };
    std::vector<VkSemaphore> gq_wait_semaphores = { _image_available_semaphores[_current_frame].handle() };
    std::vector<VkPipelineStageFlags> gq_wait_stage_flags = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    std::vector<VkSemaphore> gq_signal_semaphores = { _render_finished_semaphores[_current_frame].handle() };
    VkSubmitInfo gq_submit_info = wk::GraphicsQueueSubmitInfo{}
        .set_command_buffers(static_cast<uint32_t>(gq_command_buffers.size()), gq_command_buffers.data())
        .set_wait_semaphores(static_cast<uint32_t>(gq_wait_semaphores.size()), gq_wait_semaphores.data())
        .set_wait_dst_stage_masks(static_cast<uint32_t>(gq_wait_stage_flags.size()), gq_wait_stage_flags.data())
        .set_signal_semaphores(static_cast<uint32_t>(gq_signal_semaphores.size()), gq_signal_semaphores.data())
        .to_vk();
    result = vkQueueSubmit(_device.graphics_queue().handle(), 1, &gq_submit_info, _in_flight_fences[_current_frame].handle());
    if (result != VK_SUCCESS) {
        std::cerr << "failed to submit queue" << std::endl;
    }

    std::vector<VkSwapchainKHR> pq_swapchains = { _swapchain.handle() };
    std::vector<uint32_t> pq_image_indices = { _available_image_index };
    VkPresentInfoKHR present_info = wk::PresentInfo{}
        .set_swapchains(static_cast<uint32_t>(pq_swapchains.size()), pq_swapchains.data())
        .set_image_indices(pq_image_indices.data())
        .set_wait_semaphores(static_cast<uint32_t>(gq_signal_semaphores.size()), gq_signal_semaphores.data())
        .to_vk();
    result = vkQueuePresentKHR(_present_queue.handle(), &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        rebuild();
    } else if (result != VK_SUCCESS) {
        std::cerr << "failed to present" << std::endl;
    } else {
        _current_frame = (_current_frame + 1) % _max_in_flight;
    }
}

void VulkanViewport::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return;
    }

    _width = width;
    _height = height;
    rebuild();
}

void VulkanViewport::rebuild() {
    vkDeviceWaitIdle(_device.handle());

    _framebuffers.clear();
    _image_views.clear();
    _images.clear();

    wk::PhysicalDeviceSurfaceSupport support = wk::GetPhysicalDeviceSurfaceSupport(_physical_device.handle(), _surface.handle());
    _present_mode = wk::ChooseSurfacePresentationMode(support.present_modes);
    _image_extent = wk::ChooseSurfaceExtent(_width, _height, support.capabilities);

    // swapchain
    wk::Swapchain new_swapchain = wk::Swapchain(_device.handle(),
        wk::SwapchainCreateInfo{}
            .set_surface(_surface.handle())
            .set_present_mode(_present_mode)
            .set_min_image_count(_min_image_count)
            .set_image_extent(_image_extent)
            .set_image_format(_color_format)
            .set_image_color_space(_color_space)
            .set_image_array_layers(1)
            .set_image_usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            .set_image_sharing_mode(_queue_family_sharing_mode)
            .set_queue_family_indices(static_cast<uint32_t>(_queue_family_indices.size()), _queue_family_indices.empty() ? nullptr : _queue_family_indices.data())
            .set_old_swapchain(_swapchain.handle())
            .to_vk()
    );
    _swapchain = std::move(new_swapchain);

    _images.clear();
    _image_views.clear();
    _framebuffers.clear();
    _images.reserve(_swapchain.image_views().size());
    _image_views.reserve(_swapchain.image_views().size());
    _framebuffers.reserve(_swapchain.image_views().size());

    for (size_t i = 0; i < _swapchain.image_views().size(); ++i) {
        // depth image
        _images.emplace_back(
            _allocator.handle(),
            wk::ImageCreateInfo{}
                .set_image_type(VK_IMAGE_TYPE_2D)
                .set_format(_depth_format)
                .set_extent(wk::Extent(_image_extent).to_vk())
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

        // depth view
        _image_views.emplace_back(
            _device.handle(),
            wk::ImageViewCreateInfo{}
                .set_image(_images.back().handle())
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

        // framebuffer (color and depth)
        std::vector<VkImageView> attachments = {
            _swapchain.image_views()[i],
            _image_views.back().handle()
        };

        _framebuffers.emplace_back(
            _device.handle(),
            wk::FramebufferCreateInfo{}
                .set_render_pass(_render_pass)
                .set_attachments(static_cast<uint32_t>(attachments.size()), attachments.data())
                .set_extent(_swapchain.extent())
                .set_layers(1)
                .to_vk()
        );
    }
}

} // namespace engine::drivers::vulkan
