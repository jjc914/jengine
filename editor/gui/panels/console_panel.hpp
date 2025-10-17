#ifndef editor_gui_panels_CONSOLE_PANEL_HPP
#define editor_gui_panels_CONSOLE_PANEL_HPP

#include "panel.hpp"

#include <cstdint>

namespace editor::gui::panels {

class ConsolePanel final : public Panel {
public:
    ConsolePanel() : Panel("Console") {}

protected:
    void on_gui(GuiContext& context) override;
};

} // namespace editor::gui::panels

#endif // editor_gui_panels_CONSOLE_PANEL_HPP
