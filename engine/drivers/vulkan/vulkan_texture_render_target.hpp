#ifndef engine_drivers_vulkan_VULKAN_TEXTURE_RENDER_TARGET_HPP
#define engine_drivers_vulkan_VULKAN_TEXTURE_RENDER_TARGET_HPP

#include "engine/core/graphics/render_target.hpp"

#include <wk/wulkan.hpp>
#include <vector>

namespace engine::drivers::vulkan {

class VulkanDevice;

/**
 * @brief Represents an offscreen Vulkan render target that renders into one or more textures.
 *
 * This class is used for post-processing, reflection maps, shadow maps, or any offscreen rendering.
 * It creates its own RenderPass, Framebuffer, and the corresponding color/depth VkImage resources.
 *
 * Typical usage:
 *   - Scene pass: render to VulkanTextureRenderTarget
 *   - Postprocess pass: sample from it and render to Viewport
 */
class VulkanTextureRenderTarget {
public:
    VulkanTextureRenderTarget();
    ~VulkanTextureRenderTarget() = default;

    void begin(VkCommandBuffer cmd) {}
    void end(VkCommandBuffer cmd) {}

    // const wk::ImageView& color_view() const { return _color_view; }
    // const wk::ImageView& depth_view() const { return _depth_view; }
    // const wk::Framebuffer& framebuffer() const { return _framebuffer; }
    // const wk::RenderPass& render_pass() const { return _render_pass; }

private:
    // const wk::Device& _device;

    // wk::RenderPass _render_pass;
    // wk::Framebuffer _framebuffer;

    // wk::Image _color_image;
    // wk::ImageView _color_view;

    // wk::Image _depth_image;
    // wk::ImageView _depth_view;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_TEXTURE_RENDER_TARGET_HPP
