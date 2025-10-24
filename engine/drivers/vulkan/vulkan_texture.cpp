#include "vulkan_texture.hpp"

#include "vulkan_device.hpp"
#include "convert_vulkan.hpp"

#include "engine/core/debug/assert.hpp"

#include <cstring>

namespace engine::drivers::vulkan {

VulkanTexture::VulkanTexture(
    const VulkanDevice& device,
    uint32_t width,
    uint32_t height,
    core::graphics::ImageFormat format,
    uint32_t layers,
    uint32_t mip_levels,
    core::graphics::TextureUsage usage
)
    : _device(device.device()),
      _allocator(device.allocator()),
      _command_pool(device.command_pool()),
      _layers(layers),
      _mip_levels(mip_levels),
      _format(format),
      _vk_format(ToVkFormat(format)),
      _usage(ToVkUsage(usage)),
      _is_depth(core::graphics::IsDepthFormat(format)),
      _owns_image(true)
{
    _width = width;
    _height = height;

    // create image (owning)
    _image = wk::Image(
        _allocator.handle(),
        wk::ImageCreateInfo{}
            .set_image_type(VK_IMAGE_TYPE_2D)
            .set_format(_vk_format)
            .set_extent({ _width, _height, 1 })
            .set_mip_levels(_mip_levels)
            .set_array_layers(_layers)
            .set_samples(VK_SAMPLE_COUNT_1_BIT)
            .set_tiling(VK_IMAGE_TILING_OPTIMAL)
            .set_usage(_usage)
            .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
            .set_initial_layout(VK_IMAGE_LAYOUT_UNDEFINED)
            .to_vk(),
        wk::AllocationCreateInfo{}.set_usage(VMA_MEMORY_USAGE_GPU_ONLY).to_vk()
    );

    // create image view (owning)
    _image_view = wk::ImageView(
        _device.handle(),
        wk::ImageViewCreateInfo{}
            .set_image(_image.handle())
            .set_view_type((_layers > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D)
            .set_format(_vk_format)
            .set_subresource_range(
                wk::ImageSubresourceRange{}
                    .set_aspect_mask(_is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT)
                    .set_base_mip_level(0)
                    .set_level_count(_mip_levels)
                    .set_base_array_layer(0)
                    .set_layer_count(_layers)
                    .to_vk()
            )
            .to_vk()
    );

    _layout = core::graphics::TextureLayout::UNDEFINED;
}

VulkanTexture::VulkanTexture(
    const VulkanDevice& device,
    VkImage existing_image,
    VkImageView existing_view,
    uint32_t width,
    uint32_t height,
    core::graphics::ImageFormat format,
    uint32_t layers,
    uint32_t mip_levels,
    core::graphics::TextureUsage usage
)
    : _device(device.device()),
      _allocator(device.allocator()),
      _command_pool(device.command_pool()),
      _layers(layers),
      _mip_levels(mip_levels),
      _format(format),
      _vk_format(ToVkFormat(format)),
      _usage(ToVkUsage(usage)),
      _is_depth(core::graphics::IsDepthFormat(format)),
      _owns_image(true)
{
    _width = width;
    _height = height;

    // create image (non-owning)
    _image = wk::Image(existing_image, _allocator.handle());
    _image_view = wk::ImageView(existing_view, _device.handle());

    _layout = core::graphics::TextureLayout::UNDEFINED;
}

void VulkanTexture::bind(void* command_buffer, uint32_t binding) const {
    ENGINE_ASSERT(command_buffer != nullptr, "Attempted to bind texture with null command buffer");
}

void VulkanTexture::copy_to_cpu(std::vector<uint8_t>& out_pixels) const {
    VkDeviceSize image_size = static_cast<VkDeviceSize>(_width) * _height * 4;
    out_pixels.resize(static_cast<size_t>(image_size));

    wk::Buffer staging(
        _allocator.handle(),
        wk::BufferCreateInfo{}
            .set_size(image_size)
            .set_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT)
            .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
            .to_vk(),
        wk::AllocationCreateInfo{}.set_usage(VMA_MEMORY_USAGE_CPU_ONLY).to_vk()
    );

    wk::CommandBuffer cb(
        _device.handle(),
        wk::CommandBufferAllocateInfo{}
            .set_command_pool(_command_pool.handle())
            .to_vk()
    );

    VkCommandBufferBeginInfo begin_info = wk::CommandBufferBeginInfo{}
        .set_flags(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
        .to_vk();
    vkBeginCommandBuffer(cb.handle(), &begin_info);

    VkBufferImageCopy region = wk::BufferImageCopy{}
        .set_buffer_offset(0)
        .set_buffer_row_length(0)
        .set_buffer_image_height(0)
        .set_image_subresource(
            wk::ImageSubresourceLayers{}
                .set_aspect_mask(_is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT)
                .set_mip_level(0)
                .set_base_array_layer(0)
                .set_layer_count(1)
                .to_vk()
        )
        .set_image_offset({0, 0, 0})
        .set_image_extent({ _width, _height, 1 })
        .to_vk();

    vkCmdCopyImageToBuffer(
        cb.handle(),
        _image.handle(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        staging.handle(),
        1,
        &region
    );

    vkEndCommandBuffer(cb.handle());

    VkSubmitInfo submit_info = wk::SubmitInfo{}
        .set_command_buffers(1, &cb.handle())
        .to_vk();

    vkQueueSubmit(_device.graphics_queue().handle(), 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(_device.graphics_queue().handle());

    void* mapped = nullptr;
    vmaMapMemory(_allocator.handle(), staging.allocation(), &mapped);
    std::memcpy(out_pixels.data(), mapped, static_cast<size_t>(image_size));
    vmaUnmapMemory(_allocator.handle(), staging.allocation());
}

void VulkanTexture::transition(const core::graphics::TextureBarrier& layout_barrier) {
    if (_layout == layout_barrier.new_layout &&
        layout_barrier.old_layout != core::graphics::TextureLayout::UNDEFINED)
        return;

    wk::CommandBuffer cb(
        _device.handle(),
        wk::CommandBufferAllocateInfo{}
            .set_command_pool(_command_pool.handle())
            .to_vk()
    );

    VkCommandBufferBeginInfo begin_info = wk::CommandBufferBeginInfo{}
        .set_flags(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
        .to_vk();
    vkBeginCommandBuffer(cb.handle(), &begin_info);

    VkImageMemoryBarrier barrier = wk::ImageMemoryBarrier{}
        .set_old_layout(ToVkLayout(
            layout_barrier.old_layout == core::graphics::TextureLayout::UNDEFINED ? _layout
            : layout_barrier.old_layout))
        .set_new_layout(ToVkLayout(layout_barrier.new_layout))
        .set_src_access(0)
        .set_dst_access(
            (layout_barrier.new_layout == core::graphics::TextureLayout::SAMPLE)
                ? VK_ACCESS_SHADER_READ_BIT
                : (_is_depth
                    ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                    : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        )
        .set_image(_image.handle())
        .set_subresource_range(
            wk::ImageSubresourceRange{}
                .set_aspect_mask(_is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT)
                .set_base_mip_level(0)
                .set_level_count(_mip_levels)
                .set_base_array_layer(0)
                .set_layer_count(_layers)
                .to_vk()
        )
        .to_vk();

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dst_stage =
        (layout_barrier.new_layout == core::graphics::TextureLayout::SAMPLE)
            ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            : (_is_depth
                ? (VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT)
                : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    vkCmdPipelineBarrier(cb.handle(), src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkEndCommandBuffer(cb.handle());

    VkSubmitInfo submit_info = wk::SubmitInfo{}
        .set_command_buffers(1, &cb.handle())
        .to_vk();

    vkQueueSubmit(_device.graphics_queue().handle(), 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(_device.graphics_queue().handle());

    _layout = layout_barrier.new_layout;
}

void VulkanTexture::resize(uint32_t width, uint32_t height) {
    if (width == _width && height == _height) return;

    // non-owning textures cannot be resized
    if (!_owns_image) {
        ENGINE_ASSERT(false, "Attempted to resize a non-owning VulkanTexture. Recreate the source instead.");
        return;
    }

    _width = width;
    _height = height;

    // recreate image
    _image = wk::Image(
        _allocator.handle(),
        wk::ImageCreateInfo{}
            .set_image_type(VK_IMAGE_TYPE_2D)
            .set_format(_vk_format)
            .set_extent({ _width, _height, 1 })
            .set_mip_levels(_mip_levels)
            .set_array_layers(_layers)
            .set_samples(VK_SAMPLE_COUNT_1_BIT)
            .set_tiling(VK_IMAGE_TILING_OPTIMAL)
            .set_usage(_usage)
            .set_sharing_mode(VK_SHARING_MODE_EXCLUSIVE)
            .set_initial_layout(VK_IMAGE_LAYOUT_UNDEFINED)
            .to_vk(),
        wk::AllocationCreateInfo{}.set_usage(VMA_MEMORY_USAGE_GPU_ONLY).to_vk()
    );

    // recreate image view
    _image_view = wk::ImageView(
        _device.handle(),
        wk::ImageViewCreateInfo{}
            .set_image(_image.handle())
            .set_view_type((_layers > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D)
            .set_format(_vk_format)
            .set_subresource_range(
                wk::ImageSubresourceRange{}
                    .set_aspect_mask(_is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT)
                    .set_base_mip_level(0)
                    .set_level_count(_mip_levels)
                    .set_base_array_layer(0)
                    .set_layer_count(_layers)
                    .to_vk()
            )
            .to_vk()
    );

    _layout = core::graphics::TextureLayout::UNDEFINED;
}

} // namespace engine::drivers::vulkan
