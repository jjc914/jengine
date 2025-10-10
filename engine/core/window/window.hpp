#ifndef engine_core_window_WINDOW_HPP
#define engine_core_window_WINDOW_HPP

#include "engine/core/graphics/image_types.hpp"

#include <wk/ext/glfw/surface.hpp>

#include <string>
#include <cstdint>

namespace engine::core::graphics {

class Device;

}

namespace engine::core::window {

class Window {
public:
    virtual ~Window() = default;

    virtual bool should_close() const = 0;
    virtual void wait_events() = 0;
    virtual void poll() = 0;

    virtual void query_support(const graphics::Device& device) = 0;

    virtual const wk::ext::glfw::Surface& surface() const = 0;

    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    virtual const graphics::ImageFormat color_format() const = 0;
    virtual const graphics::ColorSpace color_space() const = 0;
    virtual const graphics::ImageFormat depth_format() const = 0;

    virtual void* native_handle() const = 0;

protected:
    Window() = default;
};

} // namespace engine::core::window

#endif // engine_core_window_WINDOW_HPP
