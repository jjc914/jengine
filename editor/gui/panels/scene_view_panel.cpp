#include "scene_view_panel.hpp"

#include "editor/gui/editor_gui.hpp"

#include <algorithm>

namespace editor::gui::panels {

void SceneViewPanel::on_gui(GuiContext& context) {
    // --- Scene View Image ---
    ImTextureID tex = context.scene_view.texture_id;
    if (!tex) {
        ImGui::TextDisabled("No scene view available");
        return;
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 scene_origin = ImGui::GetCursorScreenPos();
    ImVec2 scene_end = ImVec2(scene_origin.x + avail.x, scene_origin.y + avail.y);

    ImGui::Image(tex, avail, ImVec2(0, 1), ImVec2(1, 0));
    context.scene_view.out_size = avail;

    // scene view overlay
    ImGui::SetNextWindowBgAlpha(0.8f);
    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(scene_origin.x + 20, scene_origin.y + 20), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags overlay_flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("Overlay Tools", nullptr, overlay_flags);
    ImGui::Text("Scene Tools");
    ImGui::Separator();
    ImGui::Button("Select");
    ImGui::Button("Move");
    ImGui::Button("Rotate");

    // clamp position
    if (avail.x > 1 && avail.y > 1) {
        ImVec2 overlay_pos  = ImGui::GetWindowPos();
        ImVec2 overlay_size = ImGui::GetWindowSize();
        ImVec2 clamped_pos  = overlay_pos;

        const float pad = 5.0f;

        float min_x = scene_origin.x + pad;
        float max_x = scene_end.x - overlay_size.x - pad;
        float min_y = scene_origin.y + pad;
        float max_y = scene_end.y - overlay_size.y - pad;

        if (max_x > min_x && max_y > min_y) {
            clamped_pos.x = std::clamp(overlay_pos.x, min_x, max_x);
            clamped_pos.y = std::clamp(overlay_pos.y, min_y, max_y);

            if (clamped_pos.x != overlay_pos.x || clamped_pos.y != overlay_pos.y)
                ImGui::SetWindowPos("Overlay Tools", clamped_pos);
        }
    }
    ImGui::End();
}

void SceneViewPanel::on_gui_focus(GuiContext& context) {
    ImGuiIO& io = ImGui::GetIO();

    bool right = io.MouseDown[1];
    bool middle = io.MouseDown[2];
    float scroll = io.MouseWheel;
    glm::vec2 delta(io.MouseDelta.x, io.MouseDelta.y);
    std::cout << right << std::endl;

    context.scene_view.camera->update_input(io.DeltaTime, middle, right, delta, scroll);
}

void SceneViewPanel::on_gui_hover(GuiContext& context) {
}

}