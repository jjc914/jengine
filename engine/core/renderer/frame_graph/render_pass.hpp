#ifndef engine_core_renderer_frame_graph_RENDER_PASS_HPP
#define engine_core_renderer_frame_graph_RENDER_PASS_HPP

#include "frame_graph_id.hpp"
#include "attachment.hpp"

#include "engine/core/graphics/pipeline.hpp"
#include "engine/core/graphics/render_target.hpp"
#include "engine/core/graphics/vertex_types.hpp"

#include "engine/core/renderer/cache/shader_cache.hpp"
#include "engine/core/renderer/cache/pipeline_cache.hpp"

#include <glm/glm.hpp>

#include <functional>
#include <vector>
#include <string>
#include <optional>

namespace engine::core::renderer::framegraph {

using RenderPassId = FrameGraphId<struct RenderPassTag>;

struct RenderPassContext {
    const graphics::Pipeline* pipeline;
    graphics::CommandBuffer* command_buffer;
};

class RenderPass {
public:
    using ExecuteFn = std::function<void(RenderPassContext&)>;

    RenderPass() = default;

    void set_clear_color(glm::vec4 color) { _clear_color = color; }
    void set_clear_depth(glm::vec2 depth) { _clear_depth = depth; }

    void add_read_color(AttachmentId att)  { _reads_color.push_back(att); }
    void set_read_depth(AttachmentId att)  { _read_depth = att; }
    void add_write_color(AttachmentId att) { _writes_color.push_back(att); }
    void set_write_depth(AttachmentId att) { _write_depth = att; }

    void set_vertex_shader(cache::ShaderCacheId id) { _vertex_shader = id; }
    void set_fragment_shader(cache::ShaderCacheId id) { _fragment_shader = id; }

    void set_descriptor_set_layout(graphics::DescriptorSetLayout* layout) { _descriptor_set_layout = layout; }
    void set_vertex_binding(const graphics::VertexBindingDescription& binding) { _vertex_binding = binding; }
    void set_pipeline_config(graphics::PipelineConfig config) { _config = std::move(config); }

    void set_pipeline_override(cache::PipelineCacheId id) {
        _pipeline_override = id;
        _has_pipeline_override = true;
    }

    void set_render_target_override(graphics::RenderTarget* render_target) {
        _render_target_override = render_target;
        _has_render_target_override = true;
    }

    void set_execute(ExecuteFn fn) { _execute = std::move(fn); }

    const std::optional<glm::vec4>& clear_color() const { return _clear_color; }
    const std::optional<glm::vec2>& clear_depth() const { return _clear_depth; }

    const std::vector<AttachmentId>& reads_color()  const { return _reads_color; }
    const std::optional<AttachmentId>& read_depth()  const { return _read_depth; }
    const std::vector<AttachmentId>& writes_color() const { return _writes_color; }
    const std::optional<AttachmentId>& write_depth() const { return _write_depth; }

    const cache::ShaderCacheId& vertex_shader() const { return _vertex_shader; }
    const cache::ShaderCacheId& fragment_shader() const { return _fragment_shader; }

    const graphics::DescriptorSetLayout* descriptor_set_layout() const { return _descriptor_set_layout; }
    const graphics::VertexBindingDescription& vertex_binding() const { return _vertex_binding; }
    const graphics::PipelineConfig& pipeline_config() const { return _config; }

    const cache::PipelineCacheId& pipeline_override() const { return _pipeline_override; }
    const bool has_pipeline_override() const { return _has_pipeline_override; }
    graphics::RenderTarget* render_target_override() const { return _render_target_override; }
    const bool has_render_target_override() const { return _has_render_target_override; }

    void execute(RenderPassContext& context) const {
        if (_execute) _execute(context);
    }

private:
    std::optional<glm::vec4> _clear_color;
    std::optional<glm::vec2> _clear_depth;

    // read/write dependencies
    std::vector<AttachmentId> _reads_color;
    std::vector<AttachmentId> _writes_color;
    std::optional<AttachmentId> _read_depth;
    std::optional<AttachmentId> _write_depth;

    // shaders
    cache::ShaderCacheId _vertex_shader{};
    cache::ShaderCacheId _fragment_shader{};

    graphics::DescriptorSetLayout* _descriptor_set_layout = nullptr;
    graphics::VertexBindingDescription _vertex_binding{};
    graphics::PipelineConfig _config{};

    cache::PipelineCacheId _pipeline_override{};
    bool _has_pipeline_override = false;

    graphics::RenderTarget* _render_target_override{};
    bool _has_render_target_override = false;

    ExecuteFn _execute = nullptr;
};

} // namespace engine::core::renderer::framegraph

#endif // engine_core_renderer_frame_graph_RENDER_PASS_HPP
