#ifndef editor_gui_panels_PANEL_HPP
#define editor_gui_panels_PANEL_HPP

#include <imgui.h>

#include <string>
#include <iostream>

namespace editor::gui{
    class GuiContext;
}

namespace editor::gui::panels {

class Panel {
public:
    Panel(std::string name, bool visible = true)
        : _name(std::move(name)), _is_visible(visible) {}

    virtual ~Panel() = default;

    void draw(GuiContext& context) {
        if (!_is_visible) return;

        ImGuiWindowFlags flags = 
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin(_name.c_str(), &_is_visible, flags)) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 origin = ImGui::GetCursorScreenPos();

            ImVec2 min = origin;
            ImVec2 max = ImVec2(origin.x + avail.x, origin.y + avail.y);

            on_gui(context);
            
            ImVec2 mouse = ImGui::GetIO().MousePos;

            _is_hovered = 
                (mouse.x >= min.x && mouse.x <= max.x &&
                mouse.y >= min.y && mouse.y <= max.y);

            if (_is_hovered
                && (ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                || ImGui::IsMouseClicked(ImGuiMouseButton_Right)
                || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
            {
                _is_focused = true;
            }
            if (!_is_hovered
                && (ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                || ImGui::IsMouseClicked(ImGuiMouseButton_Right)
                || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
            {
                _is_focused = false;
            }
        }

        ImGui::End();
    }

    virtual void on_gui_focus(GuiContext& context) = 0;
    virtual void on_gui_hover(GuiContext& context) = 0;

    bool is_hovered() const { return _is_hovered; }
    bool is_focused() const { return _is_focused; }
    bool is_visible() const { return _is_visible; }
    void set_visible(bool visible) { _is_visible = visible; }

    std::string name() const { return _name; }

protected:
    virtual void on_gui(GuiContext& context) = 0;

    std::string _name;
    bool _is_hovered;
    bool _is_focused;
    bool _is_visible;
};

} // namespace editor::gui::panels

#endif // editor_gui_panels_PANEL_HPP