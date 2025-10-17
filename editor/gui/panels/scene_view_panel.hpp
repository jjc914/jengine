#ifndef editor_gui_SCENE_VIEW_PANEL_HPP
#define editor_gui_SCENE_VIEW_PANEL_HPP

#include "editor/gui/imgui_layer.hpp"

#include <imgui.h>
#include <string>
#include <cstdint>

namespace editor::gui {

class SceneViewPanel {
public:
    SceneViewPanel(ImGuiLayer& imgui_layer, const std::string& name = "View")
        : _imgui_layer(imgui_layer), _panel_name(name),
          _width(0), _height(0) {}

    ~SceneViewPanel() {
        if (_texture_id)
            _imgui_layer.unregister_texture(_texture_id);
    }

    SceneViewPanel(const SceneViewPanel&) = delete;
    SceneViewPanel& operator=(const SceneViewPanel&) = delete;

    void set_texture(VkImageView image_view, uint32_t width, uint32_t height) {
        if (_texture_id)
            _imgui_layer.unregister_texture(_texture_id);

        _texture_id = _imgui_layer.register_texture(image_view);
        _width = width;
        _height = height;
    }

    void draw() {
        if (!_texture_id) return;

        ImGui::Begin(_panel_name.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float aspect = static_cast<float>(_width) / static_cast<float>(_height);
        float avail_aspect = avail.x / avail.y;

        ImVec2 size;
        if (avail_aspect > aspect) {
            size.y = avail.y;
            size.x = avail.y * aspect;
        } else {
            size.x = avail.x;
            size.y = avail.x / aspect;
        }

        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos({
            cursor.x + (avail.x - size.x) * 0.5f,
            cursor.y + (avail.y - size.y) * 0.5f
        });

        ImGui::Image(_texture_id, size, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
    }

private:
    ImGuiLayer& _imgui_layer;
    std::string _panel_name;
    ImTextureID _texture_id;
    uint32_t _width, _height;
};

} // namespace editor::gui

#endif // editor_gui_SCENE_VIEW_PANEL_HPP
