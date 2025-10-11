#ifndef  engine_core_graphics_RENDER_TARGET_HPP
#define  engine_core_graphics_RENDER_TARGET_HPP

#include "pipeline.hpp"
#include "shader.hpp"
#include "descriptor_set_layout.hpp"
#include "vertex_types.hpp"
#include "image_types.hpp"

#include <string>
#include <memory>
#include <vector>

namespace engine::core::graphics {

class RenderTarget {
public:
    virtual ~RenderTarget() = default;

    virtual void* begin_frame(const Pipeline& pipeline) = 0;
    virtual void submit_draws(uint32_t index_count) = 0;
    virtual void end_frame() = 0;

    virtual void resize(uint32_t width, uint32_t height) = 0;

    virtual uint32_t frame_index() const = 0;
    virtual uint32_t frame_count() const = 0;
    virtual void* native_frame_image_view(uint32_t i) const = 0;
    virtual std::string backend_name() const = 0;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_RENDER_TARGET_HPP
