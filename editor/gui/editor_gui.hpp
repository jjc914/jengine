#ifndef editor_gui_EDITOR_GUI_HPP
#define editor_gui_EDITOR_GUI_HPP

#include "engine/drivers/vulkan/vulkan_instance.hpp"
#include "engine/drivers/vulkan/vulkan_device.hpp"
#include "engine/core/window/window.hpp"
#include "engine/core/graphics/pipeline.hpp"

#include "panels/panel.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include "engine/core/scene/scene.hpp"

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
        
        engine::core::scene::Scene* scene = nullptr;
        std::optional<engine::core::scene::Entity> selected_entity;

        ImVec2 out_size{0, 0};
        ImVec2 out_pos{0, 0};
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

private:
    VkDevice _device;

    bool _is_first_frame = false;
    std::vector<std::unique_ptr<panels::Panel>> _panels;
};

} // namespace editor::gui

#endif // editor_gui_EDITOR_GUI_HPP