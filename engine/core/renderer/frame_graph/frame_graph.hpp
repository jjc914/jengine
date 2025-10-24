#ifndef engine_core_renderer_frame_graph_FRAME_GRAPH_HPP
#define engine_core_renderer_frame_graph_FRAME_GRAPH_HPP

#include "attachment.hpp"
#include "render_pass.hpp"
#include "pipeline_description.hpp"

#include "engine/core/renderer/cache/pipeline_cache.hpp"

#include "engine/core/graphics/device.hpp"
#include "engine/core/graphics/pipeline.hpp"
#include "engine/core/graphics/render_target.hpp"

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <optional>

namespace engine::core::renderer::framegraph {

class FrameGraph {
public:
    FrameGraph(const graphics::Device* device, 
        cache::ShaderCache& shader_cache,
        cache::PipelineCache& pipeline_cache
    );

    FrameGraph(const FrameGraph&) = delete;
    FrameGraph& operator=(const FrameGraph&) = delete;

    FrameGraph(FrameGraph&& other) noexcept
        : _device(other._device)
        , _shader_cache(other._shader_cache)
        , _pipeline_cache(other._pipeline_cache)
        , _attachment_descriptions(std::move(other._attachment_descriptions))
        , _render_passes(std::move(other._render_passes))
        , _attachment_instances(std::move(other._attachment_instances))
        , _render_pass_instances(std::move(other._render_pass_instances))
        , _owned_textures(std::move(other._owned_textures))
        , _owned_render_targets(std::move(other._owned_render_targets))
        , _baked_pass_order(std::move(other._baked_pass_order))
        , _attachment_lifetimes(std::move(other._attachment_lifetimes))
        , _pass_to_render_target(std::move(other._pass_to_render_target))
        , _pipeline_instances(std::move(other._pipeline_instances))
    {
        other._device = nullptr;
    }

    FrameGraph& operator=(FrameGraph&& other) noexcept {
        if (this != &other) {
            _device = other._device;
            other._device = nullptr;

            _attachment_descriptions = std::move(other._attachment_descriptions);
            _render_passes = std::move(other._render_passes);
            _attachment_instances = std::move(other._attachment_instances);
            _render_pass_instances = std::move(other._render_pass_instances);
            _owned_textures = std::move(other._owned_textures);
            _owned_render_targets = std::move(other._owned_render_targets);
            _baked_pass_order = std::move(other._baked_pass_order);
            _attachment_lifetimes = std::move(other._attachment_lifetimes);
            _pass_to_render_target = std::move(other._pass_to_render_target);
            _pipeline_instances = std::move(other._pipeline_instances);
        }
        return *this;
    }

    ~FrameGraph() = default;

    AttachmentId register_attachment(const AttachmentDescription& attachments);
    RenderPassId add_pass(const RenderPass& pass);

    void update_attachment_texture(AttachmentId attachment, graphics::Texture* tex) {
        _attachment_descriptions[attachment.id].texture_override = tex;
        _attachment_instances[attachment.id].texture = tex;
    }

    void bake();
    void execute();
    
private:
    // resources
    struct AttachmentInstance {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t layers = 1;
        uint32_t mips = 1;
        graphics::Texture* texture;
        bool is_owned = true;
    };

    struct RenderPassInstance {
        graphics::CommandBuffer* command_buffer;
        const graphics::Pipeline* pipeline;
        graphics::RenderTarget* render_target;
        const graphics::Shader* vertex_shader;
        const graphics::Shader* fragment_shader;
    };

    struct AttachmentLifetime {
        size_t first_use = SIZE_MAX;
        size_t last_use = 0;
    };

    const graphics::Device* _device;

    cache::ShaderCache& _shader_cache;
    cache::PipelineCache& _pipeline_cache;

    // logical resources
    std::vector<AttachmentDescription> _attachment_descriptions;
    std::vector<RenderPass> _render_passes;

    // physical resource handles
    std::vector<AttachmentInstance> _attachment_instances;
    std::vector<RenderPassInstance> _render_pass_instances;

    // owned physical resources
    std::vector<std::unique_ptr<graphics::Texture>> _owned_textures;
    std::vector<std::unique_ptr<graphics::RenderTarget>> _owned_render_targets;

    // baked data
    std::vector<size_t> _baked_pass_order;
    std::vector<AttachmentLifetime> _attachment_lifetimes;
    std::vector<size_t> _pass_to_render_target;
    std::unordered_map<size_t, const graphics::Pipeline*> _pipeline_instances;
};

} // namespace engine::core::renderer::framegraph

#endif // engine_core_renderer_frame_graph_FRAME_GRAPH_HPP
