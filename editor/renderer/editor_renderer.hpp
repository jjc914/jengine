#ifndef editor_renderer_RENDERER_HPP
#define editor_renderer_RENDERER_HPP

#include "engine/core/graphics/instance.hpp"
#include "engine/core/graphics/device.hpp"
#include "engine/core/graphics/pipeline.hpp"
#include "engine/core/graphics/render_target.hpp"
#include "engine/core/graphics/descriptor_set_layout.hpp"

#include "engine/core/renderer/renderer.hpp"
#include "engine/core/renderer/cache/shader_cache.hpp"
#include "engine/core/renderer/cache/mesh_cache.hpp"
#include "engine/core/renderer/cache/material_cache.hpp"
#include "engine/core/renderer/cache/pipeline_cache.hpp"
#include "engine/core/renderer/frame_graph.hpp"

#include "engine/core/window/window.hpp"
#include "engine/core/scene/scene.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include "editor/gui/imgui_layer.hpp"

#include <memory>
#include <cstdint>
#include <unordered_map>

namespace editor::renderer {

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class EditorRenderer final : public engine::core::renderer::Renderer {
public:
    EditorRenderer(const engine::core::window::Window& main_window);
    ~EditorRenderer() {
        _device->wait_idle();
    }

    void resize(uint32_t width, uint32_t height);

    void render_scene(engine::core::scene::Scene& scene);

    engine::core::renderer::cache::MeshCacheId mesh_id(const std::string& mesh) const {
        auto it = _mesh_cache_entries.find(mesh);
        ENGINE_ASSERT(it != _mesh_cache_entries.end(), "Mesh not found in mesh cache entries: {}", mesh);
        return it->second;
    }

    engine::core::renderer::cache::ShaderCacheId shader_id(const std::string& shader) const {
        auto it = _shader_cache_entries.find(shader);
        ENGINE_ASSERT(it != _shader_cache_entries.end(), "Shader not found in shader cache entries: {}", shader);
        return it->second;
    }

    engine::core::renderer::cache::MaterialCacheId material_id(std::string material) const {
        auto it = _material_cache_entries.find(material);
        ENGINE_ASSERT(it != _material_cache_entries.end(), "Mesh not found in mesh cache entries: {}", material);
        return it->second;
    }

    engine::core::renderer::cache::PipelineCacheId pipeline_id(std::string pipeline) const {
        auto it = _pipeline_cache_entries.find(pipeline);
        ENGINE_ASSERT(it != _pipeline_cache_entries.end(), "Mesh not found in mesh cache entries: {}", pipeline);
        return it->second;
    }

private:
    void rebuild();

    uint32_t _width;
    uint32_t _height;

    // core graphics objects
    std::unique_ptr<engine::core::graphics::Instance> _instance;
    std::unique_ptr<engine::core::graphics::Device> _device;

    // resource caches
    std::unique_ptr<engine::core::renderer::cache::MeshCache> _mesh_cache;
    std::unordered_map<std::string, engine::core::renderer::cache::MeshCacheId> _mesh_cache_entries;

    std::unique_ptr<engine::core::renderer::cache::ShaderCache> _shader_cache;
    std::unordered_map<std::string, engine::core::renderer::cache::ShaderCacheId> _shader_cache_entries;
    
    std::unique_ptr<engine::core::renderer::cache::MaterialCache> _material_cache;
    std::unordered_map<std::string, engine::core::renderer::cache::MaterialCacheId> _material_cache_entries;
    
    std::unique_ptr<engine::core::renderer::cache::PipelineCache> _pipeline_cache;
    std::unordered_map<std::string, engine::core::renderer::cache::PipelineCacheId> _pipeline_cache_entries;

    // frame graph
    engine::core::renderer::FrameGraph _frame_graph;
    engine::core::renderer::RenderPassId _scene_pass_id;
    engine::core::renderer::RenderPassId _present_pass_id;

    // render targets
    std::unique_ptr<engine::core::graphics::RenderTarget> _main_viewport;
    std::unique_ptr<engine::core::graphics::RenderTarget> _editor_view_target;

    // ui
    std::unique_ptr<gui::ImGuiLayer> _gui_layer;
    std::vector<ImTextureID> _editor_camera_preview_textures;
};
    
}

#endif // editor_renderer_RENDERER_HPP