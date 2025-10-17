#ifndef editor_gui_panels_INSPECTOR_PANEL_HPP
#define editor_gui_panels_INSPECTOR_PANEL_HPP

#include "panel.hpp"

#include <cstdint>

namespace editor::gui::panels {

class InspectorPanel final : public Panel {
public:
    InspectorPanel() : Panel("Inspector") {}

protected:
    void on_gui(GuiContext& context) override;
};

} // namespace editor::gui::panels

#endif // editor_gui_panels_INSPECTOR_PANEL_HPP
