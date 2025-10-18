#ifndef  engine_core_graphics_RENDER_TARGET_HPP
#define  engine_core_graphics_RENDER_TARGET_HPP

#include "pipeline.hpp"
#include "shader.hpp"
#include "descriptor_set_layout.hpp"
#include "vertex_types.hpp"
#include "image_types.hpp"

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <memory>
#include <vector>

namespace engine::core::graphics {

class RenderTarget {
public:
    virtual ~RenderTarget() = default;

    virtual void* begin_frame(const Pipeline& pipeline,
        glm::vec4 color_clear = {0.0f, 0.0f, 0.0f, 1.0f},
        glm::vec2 depth_clear = {1.0f, 0.0f}
    ) = 0;
    virtual void submit_draws(uint32_t index_count) = 0;
    virtual void end_frame() = 0;

    virtual void push_constants(void* command_buffer, void* pipeline_layout, const void* data, size_t size, ShaderStageFlags stage_flags) = 0;

    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual std::vector<uint32_t> copy_color_to_cpu(uint32_t attachment_index = 0) = 0;

    virtual uint32_t frame_index() const = 0;
    virtual uint32_t frame_count() const = 0;
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    virtual void* native_frame_image_view(uint32_t i) const = 0;
    virtual std::string backend_name() const = 0;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_RENDER_TARGET_HPP
