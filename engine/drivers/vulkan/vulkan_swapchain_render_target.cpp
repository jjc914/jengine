#include "vulkan_swapchain_render_target.hpp"

#include "vulkan_device.hpp"
#include "vulkan_texture.hpp"
#include "vulkan_command_buffer.hpp"
#include "convert_vulkan.hpp"

#include "engine/core/graphics/image_types.hpp"
#include "engine/core/debug/logger.hpp"

#include <wk/wulkan.hpp>
#include <wk/ext/glfw/surface.hpp>

#include <algorithm>
#include <vector>

namespace engine::drivers::vulkan {

VulkanSwapchainRenderTarget::VulkanSwapchainRenderTarget(
    const VulkanDevice& device,
    const core::window::Window& window,
    const core::graphics::Pipeline& pipeline,
    uint32_t max_in_flight,
    bool has_depth
)
    : _vulkan_device(device),
      _device(device.device()),
      _physical_device(device.physical_device()),
      _allocator(device.allocator()),
      _command_pool(device.command_pool()),
      _graphics_queue(device.graphics_queue()),
      _render_pass(static_cast<VkRenderPass>(pipeline.native_render_pass())),
      _extent{0,0},
      _color_format(ToVkFormat(pipeline.color_format())),
      _depth_format(ToVkFormat(device.depth_format())),
      _has_depth(has_depth),
      _color_space(ToVkColorSpace(device.present_color_space())),
      _present_mode(VK_PRESENT_MODE_FIFO_KHR),
      _max_in_flight(max_in_flight),
      _frame_index(0),
      _acquired_image_index(0)
{
    // create surface from window // TODO: this is platform dependent (?)
    _surface = wk::ext::glfw::Surface(device.instance().handle(),
        static_cast<GLFWwindow*>(window.native_window())
    );

    // find present queue family
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device.handle(), &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device.handle(), &queue_family_count, queue_families.data());

    uint32_t present_family = _graphics_queue.family_index();
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        VkBool32 supports_present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device.handle(), i, _surface.handle(), &supports_present);
        if (supports_present) { present_family = i; break; }
    }
    _present_queue = wk::Queue(_device.handle(), present_family);

    // initial swapchain build
    rebuild();
}

core::graphics::CommandBuffer* VulkanSwapchainRenderTarget::begin_frame(
    const core::graphics::Pipeline& pipeline,
    glm::vec4 color_clear,
    glm::vec2 depth_clear
) {
    // Wait for previous work on this frame slot
    vkWaitForFences(_device.handle(), 1, &_in_flight_fences[_frame_index].handle(), VK_TRUE, UINT64_MAX);

    // Acquire next image
    VkResult result = vkAcquireNextImageKHR(
        _device.handle(),
        _swapchain.handle(),
        UINT32_MAX,
        _image_available_semaphores[_frame_index].handle(),
        VK_NULL_HANDLE,
        &_acquired_image_index
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        rebuild();
        return nullptr;
    }
    if (result != VK_SUCCESS) {
        core::debug::Logger::get_singleton().error("Failed to acquire swapchain image");
        return nullptr;
    }

    // reset
    vkResetFences(_device.handle(), 1, &_in_flight_fences[_frame_index].handle());
    _command_buffers[_frame_index]->reset();

    _command_buffers[_frame_index]->begin();

    // clear values
    std::vector<VkClearValue> clear_values;
    clear_values.push_back(wk::ClearValue{}.set_color(color_clear.r, color_clear.g, color_clear.b, color_clear.a).to_vk());
    if (_has_depth) {
        clear_values.push_back(wk::ClearValue{}.set_depth_stencil(depth_clear.x, depth_clear.y).to_vk());
    }

    VkRenderPassBeginInfo rp_begin_info = wk::RenderPassBeginInfo{}
        .set_render_pass(static_cast<VkRenderPass>(pipeline.native_render_pass()))
        .set_framebuffer(_framebuffers[_acquired_image_index].handle())
        .set_render_area({ {0,0}, _extent })
        .set_clear_values(static_cast<uint32_t>(clear_values.size()), clear_values.data())
        .to_vk();

    vkCmdBeginRenderPass(
        static_cast<VkCommandBuffer>(_command_buffers[_frame_index]->native_command_buffer()),
        &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE
    );

    return _command_buffers[_frame_index].get();
}

void VulkanSwapchainRenderTarget::end_frame() {
    vkCmdEndRenderPass(static_cast<VkCommandBuffer>(_command_buffers[_frame_index]->native_command_buffer()));

    _command_buffers[_frame_index]->end();

    // submit
    VkCommandBuffer cmd = static_cast<VkCommandBuffer>(_command_buffers[_frame_index]->native_command_buffer());
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = wk::SubmitInfo{}
        .set_wait_semaphores(1, &_image_available_semaphores[_frame_index].handle())
        .set_wait_dst_stage_masks(1, &wait_stage)
        .set_command_buffers(1, &cmd)
        .set_signal_semaphores(1, &_render_finished_semaphores[_frame_index].handle())
        .to_vk();

    if (vkQueueSubmit(_graphics_queue.handle(), 1, &submit_info, _in_flight_fences[_frame_index].handle()) != VK_SUCCESS) {
        core::debug::Logger::get_singleton().error("Failed to submit draw queue");
    }
}

void VulkanSwapchainRenderTarget::present() {
    // present acquired image
    VkSemaphore signal_sema = _render_finished_semaphores[_frame_index].handle();

    VkPresentInfoKHR present_info = wk::PresentInfo{}
        .set_swapchains(1, &_swapchain.handle())
        .set_image_indices(&_acquired_image_index)
        .set_wait_semaphores(1, &signal_sema)
        .to_vk();

    VkResult result = vkQueuePresentKHR(_present_queue.handle(), &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        rebuild();
    } else if (result != VK_SUCCESS) {
        core::debug::Logger::get_singleton().error("Failed to present");
    } else {
        _frame_index = (_frame_index + 1) % _max_in_flight;
    }
}

void VulkanSwapchainRenderTarget::push_constants(
    core::graphics::CommandBuffer* command_buffer,
    void* pipeline_layout,
    const void* data, size_t size,
    core::graphics::ShaderStageFlags stage_flags
) {
    ENGINE_ASSERT(command_buffer, "Invalid command buffer");
    ENGINE_ASSERT(size <= 128, "Push constants exceed Vulkan limit");

    VkCommandBuffer cmd = static_cast<VkCommandBuffer>(command_buffer->native_command_buffer());
    vkCmdPushConstants(
        cmd, static_cast<VkPipelineLayout>(pipeline_layout),
        ToVkShaderStageFlags(stage_flags),
        0,
        static_cast<uint32_t>(size), data
    );
}

void VulkanSwapchainRenderTarget::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    _extent = { width, height };
    rebuild();
}

void VulkanSwapchainRenderTarget::rebuild() {
    vkDeviceWaitIdle(_device.handle());

    // destroy resources
    _framebuffers.clear();
    _color_textures.clear();
    _depth_textures.clear();

    // query surface support
    wk::PhysicalDeviceSurfaceSupport support = wk::GetPhysicalDeviceSurfaceSupport(_physical_device.handle(), _surface.handle());
    _present_mode = wk::ChooseSurfacePresentationMode(support.present_modes);
    _extent = wk::ChooseSurfaceExtent(_extent.width, _extent.height, support.capabilities);

    // choose image count
    uint32_t min_image_count = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount != 0 &&
        min_image_count > support.capabilities.maxImageCount) {
        min_image_count = support.capabilities.maxImageCount;
    }

    // set sharing mode
    std::vector<uint32_t> queue_family_indices;
    VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    if (_graphics_queue.family_index() != _present_queue.family_index()) {
        queue_family_indices = { _graphics_queue.family_index(), _present_queue.family_index() };
        sharing_mode = VK_SHARING_MODE_CONCURRENT;
    }

    // rebuild swapchain
    wk::Swapchain new_swapchain = wk::Swapchain(_device.handle(),
        wk::SwapchainCreateInfo{}
            .set_surface(_surface.handle())
            .set_present_mode(_present_mode)
            .set_min_image_count(min_image_count)
            .set_image_extent(_extent)
            .set_image_format(_color_format)
            .set_image_color_space(_color_space)
            .set_image_array_layers(1)
            .set_image_usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            .set_image_sharing_mode(sharing_mode)
            .set_queue_family_indices(static_cast<uint32_t>(queue_family_indices.size()),
                                      queue_family_indices.empty() ? nullptr : queue_family_indices.data())
            .set_old_swapchain(_swapchain.handle())
            .to_vk()
    );
    _swapchain = std::move(new_swapchain);

    _frame_count = _swapchain.image_count();

    // resize vectors
    _framebuffers.clear();
    _framebuffers.reserve(_frame_count);

    // wrap into vulkan texture
    _color_textures.reserve(_frame_count);
    for (uint32_t i = 0; i < _frame_count; ++i) {
        VkImage image = _swapchain.images()[i];
        VkImageView view = _swapchain.image_views()[i];

        _color_textures.emplace_back(
            _vulkan_device.create_texture_from_native(
                static_cast<void*>(image),
                static_cast<void*>(view),
                _extent.width,
                _extent.height,
                FromImageVkFormat(_color_format),
                1, 1,
                core::graphics::TextureUsage::COLOR_ATTACHMENT
            )
        );
    }

    if (_has_depth) {
        _depth_textures.reserve(_frame_count);
        for (uint32_t i = 0; i < _frame_count; ++i) {
            _depth_textures.emplace_back(
                _vulkan_device.create_texture(
                    _extent.width,
                    _extent.height,
                    core::graphics::ImageFormat::D32_FLOAT_S8_UINT,
                    1, 1,
                    core::graphics::TextureUsage::DEPTH_ATTACHMENT
                )
            );
        }
    }

    // framebuffers
    for (uint32_t i = 0; i < _frame_count; ++i) {
        VkImageView attachments[2] = {
            static_cast<VkImageView>(_color_textures[i]->native_image_view())
        };
        if (_has_depth) {
            attachments[1] = static_cast<VkImageView>(_depth_textures[i]->native_image_view());
        }

        _framebuffers.emplace_back(
            _device.handle(),
            wk::FramebufferCreateInfo{}
                .set_render_pass(_render_pass)
                .set_attachments(_has_depth ? 2 : 1, attachments)
                .set_extent(_extent)
                .set_layers(1)
                .to_vk()
        );
    }

    // rebuild command buffer and sync
    if (_command_buffers.size() != _frame_count) {
        _command_buffers.clear();
        _image_available_semaphores.clear();
        _render_finished_semaphores.clear();
        _in_flight_fences.clear();

        _command_buffers.reserve(_frame_count);
        _image_available_semaphores.reserve(_frame_count);
        _render_finished_semaphores.reserve(_frame_count);
        _in_flight_fences.reserve(_frame_count);

        for (uint32_t i = 0; i < _frame_count; ++i) {
            _command_buffers.emplace_back(std::make_unique<VulkanCommandBuffer>(_device, _command_pool));

            _image_available_semaphores.emplace_back(_device.handle(), wk::SemaphoreCreateInfo{}.to_vk());
            _render_finished_semaphores.emplace_back(_device.handle(), wk::SemaphoreCreateInfo{}.to_vk());
            _in_flight_fences.emplace_back(
                _device.handle(),
                wk::FenceCreateInfo{}.set_flags(VK_FENCE_CREATE_SIGNALED_BIT).to_vk()
            );
        }
    }

    _frame_index = 0;
    _acquired_image_index = 0;
}

} // namespace engine::drivers::vulkan
