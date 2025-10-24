#ifndef engine_core_renderer_frame_graph_ATTACHMENT_HPP
#define engine_core_renderer_frame_graph_ATTACHMENT_HPP

#include "frame_graph_id.hpp"

#include "engine/core/graphics/image_types.hpp"
#include "engine/core/graphics/texture.hpp"

#include <cstdint>
#include <vector>
#include <optional>

namespace engine::core::renderer::framegraph {

using AttachmentId = FrameGraphId<struct AttachmentTag>;

struct AttachmentDescription {
    graphics::ImageFormat format;
    uint32_t width;
    uint32_t height;

    graphics::Texture* texture_override = nullptr;

private:
    
};

} // namespace engine::core::renderer::framegraph

#endif // engine_core_renderer_frame_graph_ATTACHMENT_HPP
