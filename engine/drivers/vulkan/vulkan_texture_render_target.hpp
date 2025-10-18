#ifndef engine_drivers_vulkan_VULKAN_TEXTURE_RENDER_TARGET_HPP
#define engine_drivers_vulkan_VULKAN_TEXTURE_RENDER_TARGET_HPP

#include "vulkan_render_target.hpp"

#include <glm/glm.hpp>

#include <optional>

namespace engine::drivers::vulkan {

class VulkanDevice;

class VulkanTextureRenderTarget final : public VulkanRenderTarget {
public:
    VulkanTextureRenderTarget(
        const VulkanDevice& device,
        const core::graphics::Pipeline& pipeline,
        uint32_t width,
        uint32_t height,
        bool use_depth = true,
        uint32_t max_in_flight = 1
    );
    ~VulkanTextureRenderTarget() override = default;

    void* begin_frame(const core::graphics::Pipeline& pipeline,
        glm::vec4 color_clear,
        glm::vec2 depth_clear
    ) override;
    void submit_draws(uint32_t index_count) override;
    void end_frame() override;

    void resize(uint32_t width, uint32_t height) override;
    std::vector<uint32_t> copy_color_to_cpu(uint32_t frame_index = 0) override;
    
    void* native_frame_image_view(uint32_t i) const override { return static_cast<void*>(_color_image_views[i].handle()); }

private:
    void rebuild();

    const wk::Allocator& _allocator;
    const wk::CommandPool& _command_pool;
    const wk::Queue& _graphics_queue;
    VkRenderPass _render_pass;

    std::vector<wk::Image> _color_images;
    std::vector<wk::ImageView> _color_image_views;
    std::vector<wk::Image> _depth_images;
    std::vector<wk::ImageView> _depth_image_views;

    VkFormat _color_format;
    VkFormat _depth_format;
};

} // namespace engine::drivers::vulkan

#endif