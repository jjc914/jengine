#ifndef engine_drivers_vulkan_VULKAN_VIEWPORT_HPP
#define engine_drivers_vulkan_VULKAN_VIEWPORT_HPP

#include "vulkan_render_target.hpp"

#include "engine/core/graphics/pipeline.hpp"
#include "engine/core/window/window.hpp"

#include <wk/ext/glfw/surface.hpp>

namespace engine::drivers::vulkan {

class VulkanDevice;

class VulkanViewport final : public VulkanRenderTarget {
public:
    VulkanViewport(
        const VulkanDevice& device,
        const core::window::Window& window,
        const core::graphics::Pipeline& pipeline,
        uint32_t width,
        uint32_t height,
        uint32_t max_in_flight = 1
    );
    ~VulkanViewport() = default;

    void* begin_frame(const core::graphics::Pipeline& pipeline) override;
    void submit_draws(uint32_t index_count) override;
    void end_frame() override;

    void resize(uint32_t width, uint32_t height) override;
    
    void* native_frame_image_view(uint32_t i) const override { return static_cast<void*>(_depth_image_views[i].handle()); }

private:
    void rebuild();

    const wk::PhysicalDevice& _physical_device;
    const wk::Allocator& _allocator;
    const wk::CommandPool& _command_pool;
    const wk::DescriptorPool& _descriptor_pool;
    const wk::Queue& _graphics_queue;
    VkRenderPass _render_pass;
    std::vector<uint32_t> _queue_family_indices;
    VkSharingMode _queue_family_sharing_mode;
    uint32_t _min_image_count;

    wk::ext::glfw::Surface _surface;
    std::vector<wk::Image> _depth_images;
    std::vector<wk::ImageView> _depth_image_views;

    wk::Swapchain _swapchain;
    wk::Queue _present_queue;

    VkPresentModeKHR _present_mode;
    VkExtent2D _image_extent;
    VkFormat _color_format;
    VkColorSpaceKHR _color_space;
    VkFormat _depth_format;
    uint32_t _current_index;
    uint32_t _available_image_index;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_VIEWPORT_HPP
