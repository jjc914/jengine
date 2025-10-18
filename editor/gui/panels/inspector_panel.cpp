#include "inspector_panel.hpp"

#include "editor/gui/editor_gui.hpp"
#include "editor/components/transform.hpp"
#include "editor/components/mesh_renderer.hpp"

#include "engine/core/scene/scene.hpp"

namespace editor::gui::panels {

void InspectorPanel::on_gui(GuiContext& context) {
    if (!context.scene_view.selected_entity) {
        ImGui::TextDisabled("No Entity selected");
        return;
    }

    ENGINE_ASSERT(context.scene_view.scene, "InspectorPanel requires a valid reference to a Scene");

    engine::core::scene::Entity entity = *context.scene_view.selected_entity;
    ImGui::Text("Entity %llu", (uint64_t)entity);

    context.scene_view.scene->for_each_component(entity, [&](std::type_index type, void* ptr) {
        if (type == typeid(components::Transform)) {
            auto& t = *reinterpret_cast<components::Transform*>(ptr);
            ImGui::SeparatorText("Transform");
            ImGui::DragFloat3("Position", &t.position.x, 0.1f);
            ImGui::DragFloat4("Rotation", &t.rotation.x, 0.1f);
            ImGui::DragFloat3("Scale",    &t.scale.x, 0.1f);
        } else if (type == typeid(components::MeshRenderer)) {
            auto& m = *reinterpret_cast<components::MeshRenderer*>(ptr);
            ImGui::SeparatorText("MeshRenderer");
            ImGui::Text("Mesh: %d", m.mesh_id);
            ImGui::Text("Material: %d", m.material_id);
        } else {
            ImGui::SeparatorText(type.name());
            ImGui::TextDisabled("(no component drawer implemented)");
        }
    });
}

}