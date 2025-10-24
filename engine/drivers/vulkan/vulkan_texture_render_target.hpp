#ifndef engine_drivers_vulkan_VULKAN_TEXTURE_RENDER_TARGET_HPP
#define engine_drivers_vulkan_VULKAN_TEXTURE_RENDER_TARGET_HPP

#include "vulkan_render_target.hpp"
#include "vulkan_command_buffer.hpp"

#include "engine/core/graphics/command_buffer.hpp"
#include "engine/core/graphics/texture.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <optional>

namespace engine::drivers::vulkan {

class VulkanDevice;

class VulkanTextureRenderTarget final : public VulkanRenderTarget {
public:
    VulkanTextureRenderTarget(
        const VulkanDevice& device,
        const core::graphics::Pipeline& pipeline,
        const core::graphics::AttachmentInfo& attachments,
        uint32_t max_in_flight
    );

    VulkanTextureRenderTarget(VulkanTextureRenderTarget&&) noexcept = default;
    VulkanTextureRenderTarget& operator=(VulkanTextureRenderTarget&&) noexcept = default;

    VulkanTextureRenderTarget(const VulkanTextureRenderTarget&) = delete;
    VulkanTextureRenderTarget& operator=(const VulkanTextureRenderTarget&) = delete;

    ~VulkanTextureRenderTarget() override = default;

    core::graphics::CommandBuffer* begin_frame(const core::graphics::Pipeline& pipeline,
        glm::vec4 color_clear,
        glm::vec2 depth_clear
    ) override;
    void end_frame() override;

    void* native_frame_image_view(uint32_t i) const override {
        return (void*)_color_textures[i]->native_image_view();
    }

private:
    void rebuild();

    const wk::Device& _device;
    const wk::Allocator& _allocator;
    const wk::CommandPool& _command_pool;
    const wk::Queue& _graphics_queue;

    VkRenderPass _render_pass;
    std::vector<const core::graphics::Texture*> _color_textures;
    const core::graphics::Texture* _depth_texture;
    std::vector<VulkanCommandBuffer> _command_buffers;

    VkFormat _color_format;
    VkFormat _depth_format;
    VkExtent2D _extent;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_TEXTURE_RENDER_TARGET_HPP
