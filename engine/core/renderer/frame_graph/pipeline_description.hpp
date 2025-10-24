#ifndef engine_core_renderer_frame_graph_PIPELINE_DESCRIPTION_HPP
#define engine_core_renderer_frame_graph_PIPELINE_DESCRIPTION_HPP

#include "engine/core/graphics/pipeline.hpp"
#include "engine/core/graphics/image_types.hpp"
#include "engine/core/graphics/vertex_types.hpp"
#include "engine/core/graphics/descriptor_set_layout.hpp"
#include "engine/core/renderer/cache/shader_cache.hpp"

#include <optional>
#include <vector>
#include <cstddef>
#include <functional>

namespace engine::core::renderer {

struct PipelineDescription {
    cache::ShaderCacheId vertex_shader;
    cache::ShaderCacheId fragment_shader;

    const graphics::DescriptorSetLayout* descriptor_set_layout = nullptr;
    graphics::VertexBindingDescription vertex_binding{};
    graphics::PipelineConfig config{};

    std::vector<graphics::ImageAttachmentInfo> color_attachments;
    std::optional<graphics::ImageAttachmentInfo> depth_attachment;

    // chatgpt hash combining
    static inline void hash_combine(std::size_t& seed, std::size_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }

    // chatgpt hash definition
    std::size_t hash() const noexcept {
        std::size_t h = 0;

        // hash shader ids
        hash_combine(h, std::hash<uint64_t>{}(vertex_shader));
        hash_combine(h, std::hash<uint64_t>{}(fragment_shader));

        // hash descriptor set layout pointer
        hash_combine(h, std::hash<const void*>{}(descriptor_set_layout));

        // hash pipeline config
        hash_combine(h, std::hash<bool>{}(config.blending_enabled));
        hash_combine(h, std::hash<bool>{}(config.depth_test_enabled));
        hash_combine(h, std::hash<bool>{}(config.depth_write_enabled));
        hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(config.cull_mode)));
        hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(config.polygon_mode)));
        hash_combine(h, std::hash<uint32_t>{}(config.push_constant.size));
        hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(config.push_constant.stage_flags)));

        // hash color attachments
        for (const graphics::ImageAttachmentInfo& att : color_attachments) {
            hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(att.format)));
            hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(att.usage)));
        }

        if (depth_attachment.has_value()) {
            hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(depth_attachment->format)));
            hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(depth_attachment->usage)));
        }

        // hash vertex binding description
        hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(vertex_binding.binding)));
        hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(vertex_binding.stride)));

        for (const graphics::VertexAttribute& att : vertex_binding.attributes) {
            hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(att.format)));
            hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(att.location)));
            hash_combine(h, std::hash<uint32_t>{}(static_cast<uint32_t>(att.offset)));
        }

        return h;
    }
};

} // namespace engine::core::renderer

#endif // engine_core_renderer_frame_graph_PIPELINE_DESCRIPTION_HPP
