#ifndef CORE_GRAPHICS_RENDERER_HPP
#define CORE_GRAPHICS_RENDERER_HPP

#include "pipeline.hpp"
#include "vertex_layout.hpp"

#include <cstdint>
#include <memory>

namespace core::graphics {

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void* begin_frame() = 0;
    virtual void submit_draws(uint32_t index_count) = 0;
    virtual void end_frame() = 0;

    virtual void resize(uint32_t width, uint32_t height) = 0;
    
    virtual std::unique_ptr<Pipeline> create_pipeline(const VertexLayout& layout) const = 0;

    virtual const uint32_t width() const = 0;
    virtual const uint32_t height() const = 0;
};

} // namespace core::graphics

#endif // CORE_GRAPHICS_RENDERER_HPP
