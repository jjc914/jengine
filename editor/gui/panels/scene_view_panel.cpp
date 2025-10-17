#include "scene_view_panel.hpp"

#include "editor/gui/editor_gui.hpp"

namespace editor::gui::panels {

void SceneViewPanel::on_gui(GuiContext& context) {
    ImTextureID tex = context.view_texture_id;

    if (!tex) {
        ImGui::TextDisabled("No scene view available");
        return;
    }
    
    ImVec2 avail = ImGui::GetContentRegionAvail();

    uint32_t width = std::max(1, static_cast<int>(avail.x));
    uint32_t height = std::max(1, static_cast<int>(avail.y));

    float aspect = static_cast<float>(avail.x) / avail.y;
    ImVec2 size = avail;
    if (aspect > 1.0f)
        size.x = avail.y * aspect;
    else
        size.y = avail.x / aspect;

    ImVec2 cursor = ImGui::GetCursorPos();
    cursor.x += (avail.x - size.x) * 0.5f;
    cursor.y += (avail.y - size.y) * 0.5f;
    ImGui::SetCursorPos(cursor);

    ImGui::Image(tex, size, ImVec2(0, 1), ImVec2(1, 0));

    context.scene_view_size = ImVec2{static_cast<float>(width), static_cast<float>(height)};
}

}