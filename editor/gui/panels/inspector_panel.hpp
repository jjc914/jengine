#ifndef editor_gui_panels_INSPECTOR_PANEL_HPP
#define editor_gui_panels_INSPECTOR_PANEL_HPP

#include "panel.hpp"

#include <cstdint>

namespace editor::gui::panels {

class InspectorPanel final : public Panel {
public:
    InspectorPanel() : Panel("Inspector") {}

    void on_gui_focus(GuiContext& context) override {}
    void on_gui_hover(GuiContext& context) override {}
protected:
    void on_gui(GuiContext& context) override;
};

} // namespace editor::gui::panels

#endif // editor_gui_panels_INSPECTOR_PANEL_HPP
