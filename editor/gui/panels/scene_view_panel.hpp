#ifndef editor_gui_panels_SCENE_VIEW_PANEL_HPP
#define editor_gui_panels_SCENE_VIEW_PANEL_HPP

#include "panel.hpp"

#include <cstdint>

namespace editor::gui::panels {

class SceneViewPanel final : public Panel {
public:
    SceneViewPanel() : Panel("Scene View") {}

protected:
    void on_gui(GuiContext& context) override;

private:
    ImTextureID _texture_id;
    float _aspect_ratio;
};

} // namespace editor::gui::panels

#endif // editor_gui_panels_SCENE_VIEW_PANEL_HPP
