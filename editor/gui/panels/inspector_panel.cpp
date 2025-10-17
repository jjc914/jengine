#include "inspector_panel.hpp"

#include "editor/gui/editor_gui.hpp"

namespace editor::gui::panels {

void InspectorPanel::on_gui(GuiContext& context) {
    ImGui::TextDisabled("No inspector available");
}

}