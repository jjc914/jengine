#ifndef DRIVERS_VULKAN_VULKAN_RENDERER_HPP
#define DRIVERS_VULKAN_VULKAN_RENDERER_HPP

#include "core/graphics/renderer.hpp"
#include "core/graphics/pipeline.hpp"
#include "core/graphics/vertex_layout.hpp"

#include <wk/wulkan.hpp>
#include <wk/ext/glfw/surface.hpp>
#include <vector>

namespace drivers::vulkan {

class VulkanDevice;

class VulkanRenderer : public core::graphics::Renderer {
public:
    VulkanRenderer(const VulkanDevice& device, const wk::ext::glfw::Surface& surface, uint32_t width, uint32_t height);
    ~VulkanRenderer() override = default;

    void* begin_frame() override;
    void submit_draws(uint32_t index_count) override;
    void end_frame() override;
    void resize(uint32_t width, uint32_t height) override;

    std::unique_ptr<core::graphics::Pipeline> create_pipeline(const core::graphics::VertexLayout& layout) const override;

    const wk::RenderPass& render_pass() const { return _render_pass; }
    const wk::DescriptorPool& descriptor_pool() const { return _descriptor_pool; }
    const wk::Allocator& allocator() const { return _allocator; }
    const uint32_t width() const override { return _width; }
    const uint32_t height() const override { return _height; }

private:
    void rebuild();

    const wk::ext::glfw::Surface& _surface;
    const wk::PhysicalDevice& _physical_device;
    const wk::Device& _device;
    const wk::Allocator& _allocator;
    const wk::CommandPool& _command_pool;
    const wk::DescriptorPool& _descriptor_pool;

    VkFormat _depth_format;

    wk::RenderPass _render_pass;
    wk::Swapchain _swapchain;

    std::vector<wk::Image> _depth_images;
    std::vector<wk::ImageView> _depth_image_views;
    std::vector<wk::Framebuffer> _framebuffers;

    std::vector<wk::CommandBuffer> _command_buffers;
    std::vector<wk::Semaphore> _image_available_semaphores;
    std::vector<wk::Semaphore> _render_finished_semaphores;
    std::vector<wk::Fence> _frame_in_flight_fences;
    std::vector<wk::DescriptorSet> _descriptor_sets;

    uint32_t _current_frame = 0;
    uint32_t _available_image_index = 0;
    uint32_t _width, _height;
    size_t _max_frames_in_flight = 2;

    const uint32_t _MAX_FRAMES_IN_FLIGHT = 3;
    
    const std::vector<VkFormat> _DEPTH_FORMATS = {
        VK_FORMAT_D32_SFLOAT, 
        VK_FORMAT_D32_SFLOAT_S8_UINT, 
        VK_FORMAT_D24_UNORM_S8_UINT
    };
};

} // namespace drivers::vulkan

#endif // DRIVERS_VULKAN_VULKAN_RENDERER_HPP
