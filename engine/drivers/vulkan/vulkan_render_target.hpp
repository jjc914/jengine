#ifndef engine_drivers_vulkan_VULKAN_RENDER_TARGET_HPP
#define engine_drivers_vulkan_VULKAN_RENDER_TARGET_HPP

#include "convert_vulkan.hpp"

#include "engine/core/graphics/render_target.hpp"
#include "engine/core/graphics/descriptor_set_layout.hpp"
#include "engine/core/graphics/vertex_types.hpp"
#include "engine/core/graphics/shader.hpp"
#include "engine/core/graphics/image_types.hpp"

#include <wk/wulkan.hpp>

namespace engine::drivers::vulkan {

class VulkanRenderTarget : public core::graphics::RenderTarget {
public:
    VulkanRenderTarget(const wk::Device& device) : _device(device) {}
    ~VulkanRenderTarget() override = default;
    
    void push_constants(void* command_buffer, void* pipeline_layout, const void* data, size_t size, core::graphics::ShaderStageFlags stage_flags) override {
        ENGINE_ASSERT(command_buffer, "Invalid command buffer");
        ENGINE_ASSERT(size <= 128, "Push constants exceed Vulkan limit");

        VkCommandBuffer cmd = static_cast<VkCommandBuffer>(command_buffer);
        vkCmdPushConstants(cmd, static_cast<VkPipelineLayout>(pipeline_layout), ToVkShaderStageFlags(stage_flags), 0,
            static_cast<uint32_t>(size), data);
    }

    uint32_t frame_index() const override { return _frame_index; }
    uint32_t frame_count() const override { return _frame_count; }
    uint32_t width() const override { return _width; }
    uint32_t height() const override { return _height; }
    std::string backend_name() const override { return "Vulkan"; }

protected:
    const wk::Device& _device;

    std::vector<wk::Framebuffer> _framebuffers;
    uint32_t _frame_index = 0;
    uint32_t _frame_count = 0;

    std::vector<wk::Semaphore> _image_available_semaphores;
    std::vector<wk::Semaphore> _render_finished_semaphores;
    std::vector<wk::Fence> _in_flight_fences;
    std::vector<wk::CommandBuffer> _command_buffers;

    uint32_t _width;
    uint32_t _height;

    uint32_t _max_in_flight = 1;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_RENDER_TARGET_HPP