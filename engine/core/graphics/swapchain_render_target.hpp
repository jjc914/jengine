#ifndef engine_core_graphics_SWAPCHAIN_RENDER_TARGET_HPP
#define engine_core_graphics_SWAPCHAIN_RENDER_TARGET_HPP

#include "render_target.hpp"

namespace engine::core::graphics {

class SwapchainRenderTarget : public RenderTarget {
public:
    virtual ~SwapchainRenderTarget() = default;

    virtual void resize(uint32_t width, uint32_t height) = 0;

    virtual void present() = 0;

    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    
    virtual Texture* frame_color_texture(uint32_t i) const = 0;
};

}

#endif