#ifndef engine_drivers_vulkan_VULKAN_SWAPCHAIN_RENDER_TARGET_HPP
#define engine_drivers_vulkan_VULKAN_SWAPCHAIN_RENDER_TARGET_HPP

#include "engine/core/graphics/swapchain_render_target.hpp"
#include "engine/core/graphics/pipeline.hpp"
#include "engine/core/window/window.hpp"

#include "vulkan_device.hpp"
#include "vulkan_texture.hpp"
#include "vulkan_command_buffer.hpp"
#include "convert_vulkan.hpp"

#include "engine/core/debug/logger.hpp"

#include <wk/ext/glfw/surface.hpp>
#include <wk/swapchain.hpp>
#include <wk/semaphore.hpp>
#include <wk/fence.hpp>
#include <wk/framebuffer.hpp>

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace engine::drivers::vulkan {

class VulkanSwapchainRenderTarget final : public core::graphics::SwapchainRenderTarget {
public:
    VulkanSwapchainRenderTarget(
        const VulkanDevice& device,
        const core::window::Window& window,
        const core::graphics::Pipeline& pipeline,
        uint32_t max_in_flight,
        bool has_depth
    );

    VulkanSwapchainRenderTarget(VulkanSwapchainRenderTarget&&) = default;
    VulkanSwapchainRenderTarget& operator=(VulkanSwapchainRenderTarget&&) = default;

    VulkanSwapchainRenderTarget(const VulkanSwapchainRenderTarget&) = delete;
    VulkanSwapchainRenderTarget& operator=(const VulkanSwapchainRenderTarget&) = delete;

    ~VulkanSwapchainRenderTarget() override = default;

    core::graphics::CommandBuffer* begin_frame(
        const core::graphics::Pipeline& pipeline,
        glm::vec4 color_clear,
        glm::vec2 depth_clear
    ) override;
    void end_frame() override;
    void resize(uint32_t width, uint32_t height) override;

    void present() override;

    void push_constants(
        core::graphics::CommandBuffer* command_buffer,
        void* pipeline_layout,
        const void* data, size_t size,
        core::graphics::ShaderStageFlags stage_flags
    ) override;

    uint32_t width() const override { return _extent.width; }
    uint32_t height() const override { return _extent.height; }
    uint32_t frame_index() const override { return _frame_index; }
    uint32_t frame_count() const override { return _frame_count; }

    core::graphics::Texture* frame_color_texture(uint32_t i) const override {
        return _color_textures[i].get();
    }
    
    void* native_frame_image_view(uint32_t i) const override {
        return reinterpret_cast<void*>(_color_textures[i]->native_image_view());
    }

    std::string backend_name() const override { return "Vulkan"; }

private:
    void rebuild();

private:
    // Core Vulkan handles
    const VulkanDevice& _vulkan_device;
    const wk::Device& _device;
    const wk::PhysicalDevice& _physical_device;
    const wk::Allocator& _allocator;
    const wk::CommandPool& _command_pool;
    const wk::Queue& _graphics_queue;
    wk::Queue _present_queue;

    // swapchain and surface
    wk::ext::glfw::Surface _surface;
    wk::Swapchain _swapchain;
    VkRenderPass _render_pass;

    // image and framebuffer resources
    std::vector<std::unique_ptr<core::graphics::Texture>> _color_textures;
    std::vector<std::unique_ptr<core::graphics::Texture>> _depth_textures;
    std::vector<wk::Framebuffer> _framebuffers;
    bool _has_depth = false;

    // command and sync
    std::vector<std::unique_ptr<core::graphics::CommandBuffer>> _command_buffers;
    std::vector<wk::Semaphore> _image_available_semaphores;
    std::vector<wk::Semaphore> _render_finished_semaphores;
    std::vector<wk::Fence> _in_flight_fences;

    // swapchain settings settings
    VkExtent2D _extent;
    VkFormat _color_format;
    VkFormat _depth_format;
    VkColorSpaceKHR _color_space;
    VkPresentModeKHR _present_mode;

    // frame tracking
    uint32_t _max_in_flight = 2;
    uint32_t _frame_index = 0;
    uint32_t _frame_count = 0;
    uint32_t _acquired_image_index = 0;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_SWAPCHAIN_RENDER_TARGET_HPP
