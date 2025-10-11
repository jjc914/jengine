#ifndef editor_ui_IMGUI_LAYER_HPP
#define editor_ui_IMGUI_LAYER_HPP

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include "engine/drivers/vulkan/vulkan_instance.hpp"
#include "engine/drivers/vulkan/vulkan_device.hpp"
#include "engine/core/window/window.hpp"
#include "engine/core/graphics/pipeline.hpp"

#include <wk/wulkan.hpp>

#include <imgui.h>

#include <memory>

namespace editor::ui {

class ImGuiLayer {
public:
    ImGuiLayer(const engine::core::graphics::Instance& instance,
        const engine::core::graphics::Device& device,
        const engine::core::graphics::Pipeline& pipeline,
        const engine::core::window::Window& window
    );
    ~ImGuiLayer();

    void initialize();

    void begin_frame();
    void end_frame(void* cb);

    ImTextureID register_texture(VkImageView image_view);
    void unregister_texture(ImTextureID texture_id);

private:
    VkDevice _device;
    VkDescriptorSet _descriptor_set;
    wk::Sampler _sampler;
    std::vector<ImTextureID> _registered_textures;
};

} // namespace editor::ui

#endif // editor_ui_IMGUI_LAYER_HPP