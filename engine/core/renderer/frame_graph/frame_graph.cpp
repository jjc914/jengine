#include "frame_graph.hpp"

#include "engine/core/graphics/image_types.hpp"

#include "engine/core/debug/logger.hpp"

#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>

namespace engine::core::renderer::framegraph {

FrameGraph::FrameGraph(const graphics::Device* device,
    cache::ShaderCache& shader_cache,
    cache::PipelineCache& pipeline_cache
)
    : _device(device),
      _shader_cache(shader_cache),
      _pipeline_cache(pipeline_cache)
{}

AttachmentId FrameGraph::register_attachment(const AttachmentDescription& description) {
    AttachmentId id{static_cast<uint32_t>(_attachment_descriptions.size())};
    _attachment_descriptions.push_back(description);
    return id;
}

RenderPassId FrameGraph::add_pass(const RenderPass& pass) {
    RenderPassId id{static_cast<uint32_t>(_render_passes.size())};
    _render_passes.push_back(pass);
    return id;
}

void FrameGraph::bake() {
    // get texture readers and writers
    std::unordered_map<AttachmentId, std::vector<size_t>> readers;
    std::unordered_map<AttachmentId, size_t> writers;

    for (size_t i = 0; i < _render_passes.size(); ++i) {
        for (const AttachmentId& id : _render_passes[i].reads_color()) {
            readers[id].push_back(i);
        }
        for (const AttachmentId& id : _render_passes[i].writes_color()) {
            writers[id] = i;
        }

        if (_render_passes[i].read_depth()) {
            readers[*_render_passes[i].read_depth()].push_back(i);
        }
        if (_render_passes[i].write_depth()) {
            writers[*_render_passes[i].write_depth()] = i;
        }
    }

    // build adjacency maps (writers -> readers)
    std::vector<std::vector<size_t>> adj(_render_passes.size());
    for (auto& [tex_id, writer_index] : writers) {
        auto it = readers.find(tex_id);
        if (it != readers.end()) {
            for (size_t reader_index : it->second) {
                adj[writer_index].push_back(reader_index);
            }
        }
    }

    // count in degrees for graph nodes
    std::vector<uint32_t> in_degrees(adj.size());
    for (size_t i = 0; i < in_degrees.size(); ++i) {
        for (size_t reader_index : adj[i]) {
            ++in_degrees[reader_index];
        }
    }

    // topological sort
    _baked_pass_order.clear();
    _baked_pass_order.reserve(_render_passes.size());

    std::queue<size_t> to_visit;
    for (size_t i = 0; i < in_degrees.size(); ++i) {
        if (in_degrees[i] == 0) {
            to_visit.push(i);
        }
    }
    ENGINE_ASSERT(to_visit.size() != 0, "Could not find the top of the FrameGraph. Perhaps there's a cycle?");

    while (!to_visit.empty()) {
        size_t index = to_visit.front();
        to_visit.pop();
        _baked_pass_order.push_back(index);

        for (size_t reader : adj[index]) {
            if (--in_degrees[reader] == 0) {
                to_visit.push(reader);
            }
        }
    }
    ENGINE_ASSERT(_baked_pass_order.size() == _render_passes.size(), "Cycle detected in FrameGraph");

    // compute attachment lifetimes
    _attachment_lifetimes.clear();
    _attachment_lifetimes.resize(_attachment_descriptions.size());

    for (size_t i = 0; i < _render_passes.size(); ++i) {
        for (const AttachmentId& texture : _render_passes[i].reads_color()) {
            _attachment_lifetimes[texture.id].first_use = std::min(_attachment_lifetimes[texture.id].first_use, i);
            _attachment_lifetimes[texture.id].last_use = std::max(_attachment_lifetimes[texture.id].last_use, i);
        }
        for (const AttachmentId& texture : _render_passes[i].writes_color()) {
            _attachment_lifetimes[texture.id].first_use = std::min(_attachment_lifetimes[texture.id].first_use, i);
            _attachment_lifetimes[texture.id].last_use = std::max(_attachment_lifetimes[texture.id].last_use, i);
        }

        if (_render_passes[i].read_depth()) {
            _attachment_lifetimes[_render_passes[i].read_depth()->id].first_use = std::min(_attachment_lifetimes[_render_passes[i].read_depth()->id].first_use, i);
            _attachment_lifetimes[_render_passes[i].read_depth()->id].last_use = std::max(_attachment_lifetimes[_render_passes[i].read_depth()->id].last_use, i);
        }
        if (_render_passes[i].write_depth()) {
            _attachment_lifetimes[_render_passes[i].write_depth()->id].first_use = std::min(_attachment_lifetimes[_render_passes[i].write_depth()->id].first_use, i);
            _attachment_lifetimes[_render_passes[i].write_depth()->id].last_use = std::max(_attachment_lifetimes[_render_passes[i].write_depth()->id].last_use, i);
        }
    }

    _owned_textures.clear();
    _attachment_instances.clear();
    _attachment_instances.reserve(_attachment_descriptions.size());

    for (size_t i = 0; i < _attachment_descriptions.size(); ++i) {
        const AttachmentDescription& desc = _attachment_descriptions[i];

        AttachmentInstance instance{};
        instance.width = desc.width;
        instance.height = desc.height;
        instance.layers = 1;
        instance.mips = 1;
        instance.is_owned = true;

        if (desc.texture_override) {
            // owned by external
            instance.texture = desc.texture_override;
            instance.is_owned = false;
        } else {
            const bool is_depth = graphics::IsDepthFormat(desc.format);
            std::unique_ptr<graphics::Texture> tex = _device->create_texture(
                desc.width,
                desc.height,
                desc.format,
                1, 1,
                is_depth ? graphics::TextureUsage::DEPTH_ATTACHMENT : graphics::TextureUsage::COLOR_ATTACHMENT
            );
            instance.texture = tex.get();
            _owned_textures.emplace_back(std::move(tex));
        }

        _attachment_instances.push_back(std::move(instance));
    }

    // create render pass resources
    _render_pass_instances.clear();
    _render_pass_instances.reserve(_render_passes.size());

    for (RenderPass& pass : _render_passes) {
        RenderPassInstance instance;
        if (pass.has_pipeline_override()) {
            instance.pipeline = _pipeline_cache.get(pass.pipeline_override());
        } else {
            PipelineDescription desc;
            desc.vertex_shader = pass.vertex_shader();
            desc.fragment_shader = pass.fragment_shader();
            desc.descriptor_set_layout = pass.descriptor_set_layout();
            desc.vertex_binding = pass.vertex_binding();
            desc.config = pass.pipeline_config();

            // collect target attachments
            std::vector<graphics::ImageAttachmentInfo> pipeline_attachment_info;

            for (const AttachmentId& id : pass.writes_color()) {
                const AttachmentDescription& att_desc = _attachment_descriptions[id.id];

                graphics::ImageAttachmentInfo info{};
                info.set_format(att_desc.format);
                info.set_usage(graphics::TextureUsage::COLOR_ATTACHMENT | graphics::TextureUsage::SAMPLED_IMAGE);

                desc.color_attachments.push_back(info);
                pipeline_attachment_info.push_back(info);
            }

            if (pass.write_depth()) {
                const AttachmentDescription& att_desc = _attachment_descriptions[pass.write_depth()->id];

                graphics::ImageAttachmentInfo depth_info{};
                depth_info.set_format(att_desc.format);
                depth_info.set_usage(graphics::TextureUsage::DEPTH_ATTACHMENT | graphics::TextureUsage::SAMPLED_IMAGE);

                desc.depth_attachment = depth_info;
                pipeline_attachment_info.push_back(depth_info);
            }

            // compute hash
            size_t hash = desc.hash();

            // get frame graph deduplicated pipeline handle
            const graphics::Pipeline* pipeline = nullptr;
            auto it = _pipeline_instances.find(hash);
            if (it != _pipeline_instances.end()) {
                pipeline = it->second;
            } else {
                pipeline = _pipeline_cache.get(
                    _pipeline_cache.register_pipeline(
                        *_shader_cache.get(pass.vertex_shader()),
                        *_shader_cache.get(pass.fragment_shader()),
                        *pass.descriptor_set_layout(),
                        pass.vertex_binding(),
                        pipeline_attachment_info,
                        pass.pipeline_config()
                    )
                );
                
                _pipeline_instances[hash] = pipeline;
            }
            instance.pipeline = pipeline;
        }

        // resolve handles
        instance.vertex_shader = _shader_cache.get(pass.vertex_shader());
        instance.fragment_shader = _shader_cache.get(pass.fragment_shader());

        // store instance
        _render_pass_instances.push_back(std::move(instance));
    }

    // create render targets for each pass
    _owned_render_targets.clear();
    _owned_render_targets.reserve(_render_passes.size());

    for (size_t i = 0; i < _render_passes.size(); ++i) {
        RenderPass& pass = _render_passes[i];
        RenderPassInstance& instance = _render_pass_instances[i];

        if (pass.has_render_target_override()) {
            instance.render_target = pass.render_target_override();
            continue;
        }

        // collect attachments
        graphics::AttachmentInfo attachments{};
        for (const AttachmentId& id : pass.writes_color()) {
            attachments.color_textures.push_back(_attachment_instances[id.id].texture);
        }

        if (pass.write_depth()) {
            attachments.depth_texture = _attachment_instances[pass.write_depth()->id].texture;
        }

        // validate
        if (!attachments.color_textures.empty()) {
            const uint32_t w = attachments.color_textures.front()->width();
            const uint32_t h = attachments.color_textures.front()->height();
            for (const graphics::Texture* tex : attachments.color_textures) {
                ENGINE_ASSERT(tex->width() == w && tex->height() == h,
                    "FrameGraph: Color attachments in a pass must have matching dimensions"
                );
            }
            if (attachments.depth_texture) {
                ENGINE_ASSERT(attachments.depth_texture->width()  == w && attachments.depth_texture->height() == h,
                    "FrameGraph: Depth attachment size must match color attachments"
                );
            }
        } else if (attachments.depth_texture) {
            ENGINE_ASSERT(attachments.depth_texture->width() > 0 && attachments.depth_texture->height() > 0,
                "FrameGraph: Depth attachment has invalid size"
            );
        }

        // get dimensions
        uint32_t width = 0, height = 0;
        if (!attachments.color_textures.empty()) {
            width  = attachments.color_textures.front()->width();
            height = attachments.color_textures.front()->height();
        } else if (attachments.depth_texture) {
            width  = attachments.depth_texture->width();
            height = attachments.depth_texture->height();
        } else {
            width = height = 1;
        }

        // create render target
        std::unique_ptr<graphics::RenderTarget> render_target =
            _device->create_texture_render_target(
                *instance.pipeline,
                attachments
            );

        instance.render_target = render_target.get();
        _owned_render_targets.emplace_back(std::move(render_target));
    }
}

void FrameGraph::execute() {
    for (size_t pass_index : _baked_pass_order) {
        RenderPass& pass = _render_passes[pass_index];
        RenderPassInstance& pass_instance = _render_pass_instances[pass_index];

        // transition reads to sampling layout
        for (const AttachmentId& id : pass.reads_color()) {
            AttachmentInstance& instance = _attachment_instances[id.id];

            core::graphics::TextureBarrier barrier;
            barrier.old_layout = instance.texture->layout();
            barrier.new_layout = core::graphics::TextureLayout::SAMPLE;
            barrier.usage_before = core::graphics::TextureUsage::UNDEFINED;
            barrier.usage_after = core::graphics::TextureUsage::SAMPLED_IMAGE;

            instance.texture->transition(barrier);
        }

        if (pass.read_depth()) {
            AttachmentInstance& instance = _attachment_instances[pass.read_depth()->id];

            core::graphics::TextureBarrier barrier;
            barrier.old_layout = instance.texture->layout();
            barrier.new_layout = core::graphics::TextureLayout::SAMPLE;
            barrier.usage_before = core::graphics::TextureUsage::UNDEFINED;
            barrier.usage_after = core::graphics::TextureUsage::SAMPLED_IMAGE;

            instance.texture->transition(barrier);
        }

        // transition writes to color/depth
        for (const AttachmentId& id : pass.writes_color()) {
            AttachmentInstance& instance = _attachment_instances[id.id];

            core::graphics::TextureBarrier barrier;
            barrier.old_layout = instance.texture->layout();
            barrier.new_layout = core::graphics::TextureLayout::COLOR;
            barrier.usage_before = core::graphics::TextureUsage::UNDEFINED;
            barrier.usage_after = core::graphics::TextureUsage::COLOR_ATTACHMENT;

            instance.texture->transition(barrier);
        }

        if (pass.write_depth()) {
            AttachmentInstance& instance = _attachment_instances[pass.write_depth()->id];

            core::graphics::TextureBarrier barrier;
            barrier.old_layout = instance.texture->layout();
            barrier.new_layout = core::graphics::TextureLayout::DEPTH;
            barrier.usage_before = core::graphics::TextureUsage::UNDEFINED;
            barrier.usage_after = core::graphics::TextureUsage::DEPTH_ATTACHMENT;

            instance.texture->transition(barrier);
        }
        
        // build render pass context (frame-specific for now)
        RenderPassContext context;
        
        // begin frame
        glm::vec4 clear_color = pass.clear_color().value_or(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        glm::vec2 clear_depth = pass.clear_depth().value_or(glm::vec2(0, 1));
        context.command_buffer = pass_instance.render_target->begin_frame(
            *pass_instance.pipeline,
            clear_color,
            clear_depth
        );
        context.pipeline = pass_instance.pipeline;

        // execute render pass
        pass.execute(context);

        // end frame
        pass_instance.render_target->end_frame();
    }
}

} // namespace engine::core::renderer::framegraph
