#ifndef editor_gui_EDITOR_GUI_HPP
#define editor_gui_EDITOR_GUI_HPP

#include "engine/drivers/vulkan/vulkan_instance.hpp"
#include "engine/drivers/vulkan/vulkan_device.hpp"
#include "engine/core/window/window.hpp"
#include "engine/core/graphics/pipeline.hpp"

#include "panels/panel.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include "editor/scene/editor_camera.hpp"

#include <wk/wulkan.hpp>

#include <imgui.h>

#include <memory>

namespace editor::gui {

struct GuiContext {
    void* command_buffer;

    struct SceneView {
        ImTextureID texture_id;
        scene::EditorCamera* camera = nullptr;
        ImVec2 out_size{0, 0};
    } scene_view;
};

class EditorGui {
public:
    EditorGui(const engine::core::graphics::Instance& instance,
        const engine::core::graphics::Device& device,
        const engine::core::graphics::Pipeline& pipeline,
        const engine::core::window::Window& window
    );
    ~EditorGui();

    void on_gui(GuiContext& context);

    ImTextureID register_texture(VkImageView image_view);
    void unregister_texture(ImTextureID texture_id);

private:
    VkDevice _device;
    VkDescriptorSet _descriptor_set;
    wk::Sampler _sampler;
    std::vector<ImTextureID> _registered_textures;

    bool _is_first_frame = false;
    std::vector<std::unique_ptr<panels::Panel>> _panels;
};

} // namespace editor::gui

#endif // editor_gui_EDITOR_GUI_HPP