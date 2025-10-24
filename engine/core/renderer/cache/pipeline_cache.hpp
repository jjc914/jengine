#ifndef engine_core_renderer_cache_PIPELINE_CACHE_HPP
#define engine_core_renderer_cache_PIPELINE_CACHE_HPP

#include "engine/core/graphics/device.hpp"
#include "engine/core/graphics/pipeline.hpp"
#include "engine/core/graphics/shader.hpp"
#include "engine/core/graphics/descriptor_set_layout.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

namespace engine::core::renderer::cache {

using PipelineCacheId = uint32_t;

class PipelineCache {
public:
    explicit PipelineCache(graphics::Device& device)
        : _device(device), _next_id(0) {}

    PipelineCache(const PipelineCache&) = delete;
    PipelineCache& operator=(const PipelineCache&) = delete;
    ~PipelineCache() = default;

    PipelineCacheId register_pipeline(
        const graphics::Shader& vert_shader,
        const graphics::Shader& frag_shader,
        const graphics::DescriptorSetLayout& layout,
        const graphics::VertexBindingDescription& vertex_binding,
        const std::vector<graphics::ImageAttachmentInfo>& attachments,
        const core::graphics::PipelineConfig& config)
    {
        std::unique_ptr<graphics::Pipeline> pipeline = _device.create_pipeline(vert_shader, frag_shader, layout, vertex_binding, attachments, config);

        PipelineCacheId id = _next_id++;
        _pipelines.emplace_back(std::move(pipeline));
        return id;
    }

    void clear() {
        _pipelines.clear();
        _next_id = 0;
    }

    graphics::Pipeline* get(PipelineCacheId id) {
        ENGINE_ASSERT(id < _next_id, "Invalid PipelineCacheId for PipelineCache");
        return _pipelines.at(id).get();
    }

    const graphics::Pipeline* get(PipelineCacheId id) const {
        ENGINE_ASSERT(id < _next_id, "Invalid PipelineCacheId for PipelineCache");
        return _pipelines.at(id).get();
    }

private:
    graphics::Device& _device;

    PipelineCacheId _next_id;
    std::vector<std::unique_ptr<graphics::Pipeline>> _pipelines;
};

} // namespace engine::core::renderer::cache

#endif // engine_core_renderer_cache_PIPELINE_CACHE_HPP
