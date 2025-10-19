#ifndef editor_renderer_EDITOR_SCENE_VIEW_RENDERER_HPP
#define editor_renderer_EDITOR_SCENE_VIEW_RENDERER_HPP

#include "engine/core/scene/scene.hpp"
#include "editor/scene/editor_camera.hpp"

#include "engine/core/renderer/frame_graph.hpp"
#include "engine/core/renderer/cache/pipeline_cache.hpp"
#include "engine/core/renderer/cache/material_cache.hpp"
#include "engine/core/renderer/cache/mesh_cache.hpp"
#include "engine/core/renderer/cache/shader_cache.hpp"

#include "engine/core/graphics/device.hpp"
#include "engine/core/graphics/render_target.hpp"
#include "engine/core/graphics/descriptor_set_layout.hpp"

#include "editor/gui/editor_gui.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <memory>
#include <vector>
#include <imgui.h>

namespace editor::renderer {

class EditorRenderer;

class EditorSceneViewRenderer {
public:
    EditorSceneViewRenderer(
        EditorRenderer& editor_renderer,
        uint32_t width,
        uint32_t height
    );

    void register_passes(
        engine::core::scene::Scene& scene,
        engine::core::renderer::FrameGraph& graph
    );
        
    void resize(uint32_t width, uint32_t height);

    ImTextureID texture_id(uint32_t frame_index) const { return _imgui_texture_ids[frame_index]; }

    engine::core::scene::Entity selected_entity() const { return _selected_entity; }

    uint32_t frame_index() const { return _view_target->frame_index(); }
    editor::scene::EditorCamera& camera() { return *_camera; }
    
    void handle_picking(uint32_t frame_index, glm::vec2 view_pos, glm::vec2 view_size);

private:
    void render_scene_pass(engine::core::scene::Scene& scene);
    void render_pick_pass(engine::core::scene::Scene& scene);

    engine::core::graphics::Device* _device;
    engine::core::renderer::cache::PipelineCache& _pipeline_cache;
    engine::core::renderer::cache::MaterialCache& _material_cache;
    engine::core::renderer::cache::MeshCache& _mesh_cache;
    gui::EditorGui& _gui;

    std::unique_ptr<editor::scene::EditorCamera> _camera;

    std::unique_ptr<engine::core::graphics::RenderTarget> _view_target;
    std::unique_ptr<engine::core::graphics::RenderTarget> _pick_target;

    engine::core::renderer::FrameGraph _frame_graph;

    std::vector<ImTextureID> _imgui_texture_ids;
    wk::Sampler _sampler;
    glm::vec2 _view_pos{0.0f, 0.0f};
    glm::vec2 _view_size{0.0f, 0.0f};

    uint32_t _width = 0;
    uint32_t _height = 0;

    std::optional<glm::vec2> _pending_resize;
    engine::core::scene::Entity _selected_entity{0};

    engine::core::renderer::cache::PipelineCacheId _mesh_normal_pipeline_id;
    engine::core::renderer::cache::PipelineCacheId _mesh_pick_pipeline_id;
    engine::core::renderer::cache::MaterialCacheId _mesh_pick_material_id;
    engine::core::renderer::cache::PipelineCacheId _mesh_outline_pipeline_id;
    engine::core::renderer::cache::MaterialCacheId _mesh_outline_material_id;
    engine::core::renderer::cache::MeshCacheId _cube_id;
    engine::core::renderer::cache::PipelineCacheId _skybox_pipeline_id;
    engine::core::renderer::cache::MaterialCacheId _skybox_material_id;
};

} // namespace editor::renderer

#endif
