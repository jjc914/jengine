#include "console_panel.hpp"

#include "editor/gui/editor_gui.hpp"

namespace editor::gui::panels {

void ConsolePanel::on_gui(GuiContext& context) {
    ImGui::TextDisabled("No console available");
}

}