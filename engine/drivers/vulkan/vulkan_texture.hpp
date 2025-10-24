#ifndef engine_drivers_vulkan_VULKAN_TEXTURE_HPP
#define engine_drivers_vulkan_VULKAN_TEXTURE_HPP

#include "engine/core/graphics/texture.hpp"
#include "engine/core/graphics/image_types.hpp"

#include <wk/wulkan.hpp>

namespace engine::drivers::vulkan {

class VulkanDevice;

class VulkanTexture final : public core::graphics::Texture {
public:
    VulkanTexture(
        const VulkanDevice& device,
        uint32_t width,
        uint32_t height,
        core::graphics::ImageFormat format,
        uint32_t layers = 1,
        uint32_t mip_levels = 1,
        core::graphics::TextureUsage usage = core::graphics::TextureUsage::COLOR_ATTACHMENT
    );
    VulkanTexture(
        const VulkanDevice& device,
        VkImage existing_image,
        VkImageView existing_view,
        uint32_t width,
        uint32_t height,
        core::graphics::ImageFormat format,
        uint32_t layers = 1,
        uint32_t mip_levels = 1,
        core::graphics::TextureUsage usage = core::graphics::TextureUsage::COLOR_ATTACHMENT
    );

    VulkanTexture(VulkanTexture&& other) = default;
    VulkanTexture& operator=(VulkanTexture&& other) = default;

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    ~VulkanTexture() override = default;

    void bind(void* command_buffer, uint32_t binding = 0) const override;
    void copy_to_cpu(std::vector<uint8_t>& out_pixels) const override;
    void transition(const core::graphics::TextureBarrier& barrier) override;
    void resize(uint32_t width, uint32_t height) override;

    uint32_t layers() const override { return _layers; }
    uint32_t mip_levels() const override { return _mip_levels; }
    core::graphics::ImageFormat format() const override { return _format; }

    core::graphics::TextureLayout layout() const override { return _layout; }
    void set_layout(core::graphics::TextureLayout layout) override { _layout = layout; }

    void* native_image() const override { return (void*)_image.handle(); }
    void* native_image_view() const override { return (void*)_image_view.handle(); }

    VkImage image() const { return _image.handle(); }
    VkImageView view() const { return _image_view.handle(); }
    VkFormat vk_format() const { return _vk_format; }

private:
    void create_image(uint32_t width, uint32_t height);
    void destroy_image();

private:
    const wk::Device& _device;
    const wk::Allocator& _allocator;
    const wk::CommandPool& _command_pool;

    wk::Image _image;
    wk::ImageView _image_view;
    bool _owns_image;

    VkFormat _vk_format;
    VkImageUsageFlags _usage;

    uint32_t _layers;
    uint32_t _mip_levels;
    bool _is_depth;

    core::graphics::ImageFormat _format;
    core::graphics::TextureLayout _layout = core::graphics::TextureLayout::UNDEFINED;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_TEXTURE_HPP
