#ifndef editor_gui_panels_PANEL_HPP
#define editor_gui_panels_PANEL_HPP

#include <imgui.h>

#include <string>

namespace editor::gui{
    class GuiContext;
}

namespace editor::gui::panels {

class Panel {
public:
    Panel(std::string name, bool visible = true)
        : _name(std::move(name)), _visible(visible) {}

    virtual ~Panel() = default;

    void draw(GuiContext& context) {
        if (!_visible) return;

        ImGuiWindowFlags flags = 
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;
        if (ImGui::Begin(_name.c_str(), &_visible, flags)) {
            on_gui(context);
        }
        ImGui::End();
    }

    bool is_visible() const { return _visible; }
    void set_visible(bool visible) { _visible = visible; }

protected:
    virtual void on_gui(GuiContext& context) = 0;

    std::string _name;
    bool _visible;
};

} // namespace editor::gui::panels

#endif // editor_gui_panels_PANEL_HPP