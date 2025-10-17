#include "scene_view_panel.hpp"

#include "editor/gui/editor_gui.hpp"

namespace editor::gui::panels {

void SceneViewPanel::on_gui(GuiContext& context) {
    // scene view image
    ImTextureID tex = context.scene_view.texture_id;
    if (!tex) {
        ImGui::TextDisabled("No scene view available");
        return;
    }
    
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 scene_origin = ImGui::GetCursorScreenPos();
    ImGui::Image(tex, avail, ImVec2(0,1), ImVec2(1,0));
    context.scene_view.out_size = avail;

    // tool overlay
    ImGui::SetCursorPos(ImVec2(10, 60)); // offset from top-left of the panel
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(20, 20, 20, 160)); // semi-transparent background
    if (ImGui::BeginChild("SceneViewOverlay", ImVec2(150, 60), true)) {
        ImGui::Text("Overlay Panel");
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void SceneViewPanel::on_gui_focus(GuiContext& context) {
    ImGuiIO& io = ImGui::GetIO();

    bool right = io.MouseDown[1];
    bool middle = io.MouseDown[2];
    float scroll = io.MouseWheel;
    glm::vec2 delta(io.MouseDelta.x, io.MouseDelta.y);

    context.scene_view.camera->update_input(io.DeltaTime, middle, right, delta, scroll);
}

void SceneViewPanel::on_gui_hover(GuiContext& context) {
}

}