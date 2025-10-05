#include "vk_renderer.hpp"

#include "vk_pipeline.hpp"

#include <wk/ext/glfw/surface.hpp>

#include <algorithm>
#include <iostream>

namespace core::graphics::vulkan {

VulkanRenderer::VulkanRenderer(const VulkanDevice& device, const wk::ext::glfw::Surface& surface, uint32_t width, uint32_t height)
    : _physical_device(device.physical_device()), _device(device.device()),
      _surface(surface),
      _allocator(device.allocator()),
      _command_pool(device.command_pool()),
      _descriptor_pool(device.descriptor_pool()),
      _width(width), _height(height)
{
    // ---------- swapchain ----------
    wk::PhysicalDeviceSurfaceSupport surface_support = wk::GetPhysicalDeviceSurfaceSupport(_physical_device.handle(), _surface.handle());
    VkPresentModeKHR present_mode = wk::ChooseSurfacePresentationMode(surface_support.present_modes);
    VkSurfaceFormatKHR surface_format = wk::ChooseSurfaceFormat(surface_support.formats);
    uint32_t min_image_count = std::clamp(
        surface_support.capabilities.minImageCount + 1,
        surface_support.capabilities.minImageCount,
        surface_support.capabilities.maxImageCount
    );

    std::vector<uint32_t> queue_family_indices_vec;
    VkSharingMode image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    if (device.queue_families().is_unique()) {
        queue_family_indices_vec = device.queue_families().to_vec(); // {graphics, present}
        image_sharing_mode = VK_SHARING_MODE_CONCURRENT;
    }

    _swapchain = wk::Swapchain(_device.handle(),
        wk::SwapchainCreateInfo{}
            .set_surface(_surface.handle())
            .set_present_mode(present_mode)
            .set_min_image_count(min_image_count)
            .set_image_extent(wk::ChooseSurfaceExtent(_width, _height, surface_support.capabilities))
            .set_image_format(surface_format.format)
            .set_image_color_space(surface_format.colorSpace)
            .set_image_array_layers(1)
            .set_image_usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            .set_image_sharing_mode(image_sharing_mode)
            .set_queue_family_indices(static_cast<uint32_t>(queue_family_indices_vec.size()),
                                      queue_family_indices_vec.data())
            .to_vk()
    );

    // ---------- render pass ----------
    _depth_format = wk::ChooseDepthFormat(device.physical_device().handle(), _DEPTH_FORMATS); // TODO: this shouldn't need physical device

    VkAttachmentDescription attachments[2] = {
        wk::AttachmentDescription{}
            .set_flags(0)
            .set_format(surface_format.format)
            .set_samples(VK_SAMPLE_COUNT_1_BIT)
            .set_load_op(VK_ATTACHMENT_LOAD_OP_CLEAR)
            .set_store_op(VK_ATTACHMENT_STORE_OP_STORE)
            .set_stencil_load_op(VK_ATTACHMENT_LOAD_OP_DONT_CARE)
            .set_stencil_store_op(VK_ATTACHMENT_STORE_OP_DONT_CARE)
            .set_initial_layout(VK_IMAGE_LAYOUT_UNDEFINED)
            .set_final_layout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            .to_vk(),
        wk::AttachmentDescription{}
            .set_flags(0)
            .set_format(_depth_format)
            .set_samples(VK_SAMPLE_COUNT_1_BIT)
            .set_load_op(VK_ATTACHMENT_LOAD_OP_CLEAR)
            .set_store_op(VK_ATTACHMENT_STORE_OP_DONT_CARE)
            .set_stencil_load_op(VK_ATTACHMENT_LOAD_OP_CLEAR)
            .set_stencil_store_op(VK_ATTACHMENT_STORE_OP_DONT_CARE)
            .set_initial_layout(VK_IMAGE_LAYOUT_UNDEFINED)
            .set_final_layout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            .to_vk()
    };
    
    VkAttachmentReference color_attachment_ref = wk::AttachmentReference{}
        .set_attachment(0)
        .set_layout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .to_vk();

    VkAttachmentReference depth_attachment_ref = wk::AttachmentReference{}
        .set_attachment(1)
        .set_layout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        .to_vk();

    VkSubpassDescription subpass = wk::SubpassDescription{}
        .set_pipeline_bind_point(VK_PIPELINE_BIND_POINT_GRAPHICS)
        .set_color_attachments(1, &color_attachment_ref)
        .set_depth_stencil_attachment(&depth_attachment_ref)
        .to_vk();

    VkSubpassDependency dependency = wk::SubpassDependency{}
        .set_src_subpass(VK_SUBPASS_EXTERNAL)
        .set_dst_subpass(0)
        .set_src_stage_mask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        .set_dst_stage_mask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        .set_src_access_mask(0)
        .set_dst_access_mask(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        .to_vk();

    _render_pass = wk::RenderPass(_device.handle(),
        wk::RenderPassCreateInfo{}
            .set_attachments(2, attachments)
            .set_subpasses(1, &subpass)
            .set_dependencies(1, &dependency)
            .to_vk()
    );

    _depth_images.clear();
    _depth_image_views.clear();
    _framebuffers.clear();
    _depth_images.reserve(_swapchain.image_views().size());
    _depth_image_views.reserve(_swapchain.image_views().size());
    _framebuffers.reserve(_swapchain.image_views().size());

    for (size_t i = 0; i < _swapchain.image_views().size(); ++i) {
        _depth_images.emplace_back(
            _allocator.handle(),
            wk::ImageCreateInfo{}
                .set_image_type(VK_IMAGE_TYPE_2D)
                .set_format(_depth_format)
                .set_extent(wk::Extent(_swapchain.extent()).to_vk())
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
                .set_image(_depth_images.back().handle())
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

        // framebuffer (color + depth)
        std::vector<VkImageView> attachments = {
            _swapchain.image_views()[i],
            _depth_image_views.back().handle()
        };

        _framebuffers.emplace_back(
            _device.handle(),
            wk::FramebufferCreateInfo{}
                .set_render_pass(_render_pass.handle())
                .set_attachments(static_cast<uint32_t>(attachments.size()), attachments.data())
                .set_extent(_swapchain.extent())
                .set_layers(1)
                .to_vk()
        );
    }

    // ---------- sync & command buffers ----------
    _command_buffers.clear();
    _image_available_semaphores.clear();
    _render_finished_semaphores.clear();
    _frame_in_flight_fences.clear();
    _command_buffers.reserve(_MAX_FRAMES_IN_FLIGHT);
    _image_available_semaphores.reserve(_MAX_FRAMES_IN_FLIGHT);
    _render_finished_semaphores.reserve(_MAX_FRAMES_IN_FLIGHT);
    _frame_in_flight_fences.reserve(_MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < _MAX_FRAMES_IN_FLIGHT; ++i) {
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
        _frame_in_flight_fences.emplace_back(_device.handle(),
            wk::FenceCreateInfo{}
                .set_flags(VK_FENCE_CREATE_SIGNALED_BIT)
                .to_vk());
    }
}

void* VulkanRenderer::begin_frame() {
    vkWaitForFences(_device.handle(), 1, &_frame_in_flight_fences[_current_frame].handle(), VK_TRUE, UINT64_MAX);

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

    vkResetFences(_device.handle(), 1, &_frame_in_flight_fences[_current_frame].handle());
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
        .set_render_pass(_render_pass.handle())
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
        .set_width(static_cast<float>(_swapchain.extent().width))
        .set_height(static_cast<float>(_swapchain.extent().height))
        .set_min_depth(0.0f).set_max_depth(1.0f)
        .to_vk();
    VkRect2D scissor = wk::Rect2D{}
        .set_offset({0,0})
        .set_extent(_swapchain.extent())
        .to_vk();
    vkCmdSetViewport(_command_buffers[_current_frame].handle(), 0, 1, &viewport);
    vkCmdSetScissor(_command_buffers[_current_frame].handle(), 0, 1, &scissor);

    return static_cast<void*>(_command_buffers[_current_frame].handle());
}

void VulkanRenderer::submit_draws(uint32_t index_count) {
    vkCmdDrawIndexed(_command_buffers[_current_frame].handle(), index_count, 1, 0, 0, 0);
}

void VulkanRenderer::end_frame() {
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
    result = vkQueueSubmit(_device.graphics_queue().handle(), 1, &gq_submit_info, _frame_in_flight_fences[_current_frame].handle());
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
    result = vkQueuePresentKHR(_device.present_queue().handle(), &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        rebuild();
    } else if (result != VK_SUCCESS) {
        std::cerr << "failed to present" << std::endl;
    } else {
        _current_frame = (_current_frame + 1) % _MAX_FRAMES_IN_FLIGHT;
    }
}

void VulkanRenderer::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return; // window minimized
    _width = width;
    _height = height;
    rebuild();
}

void VulkanRenderer::rebuild() {
    vkDeviceWaitIdle(_device.handle());

    wk::PhysicalDeviceSurfaceSupport surface_support = wk::GetPhysicalDeviceSurfaceSupport(_physical_device.handle(), _surface.handle());
    VkPresentModeKHR present_mode = wk::ChooseSurfacePresentationMode(surface_support.present_modes);
    VkSurfaceFormatKHR surface_format = wk::ChooseSurfaceFormat(surface_support.formats);
    uint32_t min_image_count = std::clamp(
        surface_support.capabilities.minImageCount + 1,
        surface_support.capabilities.minImageCount,
        surface_support.capabilities.maxImageCount
    );

    wk::Swapchain new_swapchain = wk::Swapchain(_device.handle(),
        wk::SwapchainCreateInfo{}
            .set_surface(_surface.handle())
            .set_present_mode(present_mode)
            .set_min_image_count(min_image_count)
            .set_image_extent(wk::ChooseSurfaceExtent(_width, _height, surface_support.capabilities))
            .set_image_format(surface_format.format)
            .set_image_color_space(surface_format.colorSpace)
            .set_image_array_layers(1)
            .set_image_usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            .set_image_sharing_mode(_swapchain.image_sharing_mode())
            .set_queue_family_indices(_swapchain.queue_family_indices().size(),
                                      _swapchain.queue_family_indices().data())
            .set_old_swapchain(_swapchain.handle())
            .to_vk()
    );
    _swapchain = std::move(new_swapchain);

    _depth_images.clear();
    _depth_image_views.clear();
    _framebuffers.clear();
    _depth_images.reserve(_swapchain.image_views().size());
    _depth_image_views.reserve(_swapchain.image_views().size());
    _framebuffers.reserve(_swapchain.image_views().size());

    for (size_t i = 0; i < _swapchain.image_views().size(); ++i) {
        // depth image per framebuffer
        _depth_images.emplace_back(
            _allocator.handle(),
            wk::ImageCreateInfo{}
                .set_image_type(VK_IMAGE_TYPE_2D)
                .set_format(_depth_format)
                .set_extent(wk::Extent(_swapchain.extent()).to_vk())
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
        _depth_image_views.emplace_back(
            _device.handle(),
            wk::ImageViewCreateInfo{}
                .set_image(_depth_images.back().handle())
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

        // Framebuffer (color + depth)
        std::vector<VkImageView> attachments = {
            _swapchain.image_views()[i],
            _depth_image_views.back().handle()
        };

        _framebuffers.emplace_back(
            _device.handle(),
            wk::FramebufferCreateInfo{}
                .set_render_pass(_render_pass.handle())
                .set_attachments(static_cast<uint32_t>(attachments.size()), attachments.data())
                .set_extent(_swapchain.extent())
                .set_layers(1)
                .to_vk()
        );
    }
}

std::unique_ptr<Pipeline> VulkanRenderer::create_pipeline(const VertexLayout& layout) const {
    return std::make_unique<VulkanPipeline>(
        *this, _device, layout
    );
};

} // core::graphics::vulkan