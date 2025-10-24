#ifndef editor_renderer_EDITOR_RENDERER_HPP
#define editor_renderer_EDITOR_RENDERER_HPP

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
#include "engine/core/renderer/frame_graph/frame_graph.hpp"

#include "editor/gui/editor_gui.hpp"

#include "engine/core/window/window.hpp"
#include "engine/core/scene/scene.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include "editor/scene/editor_camera.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <optional>

namespace editor::renderer {

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};

struct PushConstants {
    glm::mat4 model;
};

class EditorRenderer final : public engine::core::renderer::Renderer {
public:
    explicit EditorRenderer(const engine::core::window::Window& main_window);
    ~EditorRenderer() override {
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

    engine::core::renderer::cache::MaterialCacheId material_id(const std::string& material) const {
        auto it = _material_cache_entries.find(material);
        ENGINE_ASSERT(it != _material_cache_entries.end(), "Material not found in material cache entries: {}", material);
        return it->second;
    }

    engine::core::graphics::Device* device() const { return _device.get(); }

    engine::core::renderer::cache::MeshCache& mesh_cache() const { return *_mesh_cache; }
    engine::core::renderer::cache::ShaderCache& shader_cache() const { return *_shader_cache; }
    engine::core::renderer::cache::MaterialCache& material_cache() const { return *_material_cache; }
    engine::core::renderer::cache::PipelineCache& pipeline_cache() const { return *_pipeline_cache; }

private:
    uint32_t _width = 0;
    uint32_t _height = 0;

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

    // graphics resources
    std::unique_ptr<engine::core::graphics::DescriptorSetLayout> _vertex_ubo_layout;

    // core frame graph
    std::unique_ptr<engine::core::renderer::framegraph::FrameGraph> _frame_graph;
    engine::core::renderer::framegraph::RenderPassId _scene_pass_id{};
    engine::core::renderer::framegraph::RenderPassId _editor_pick_pass_id{};
    engine::core::renderer::framegraph::RenderPassId _present_pass_id{};

    // render targets
    std::unique_ptr<engine::core::graphics::SwapchainRenderTarget> _main_swapchain;
    engine::core::renderer::framegraph::AttachmentId _swapchain_attachment{};

    // gui
    engine::core::renderer::cache::PipelineCacheId _imgui_pipeline_id{};
    std::unique_ptr<engine::core::graphics::DescriptorSetLayout> _imgui_layout;
    std::unique_ptr<gui::EditorGui> _editor_gui;
};

} // namespace editor::renderer

#endif // editor_renderer_EDITOR_RENDERER_HPP
