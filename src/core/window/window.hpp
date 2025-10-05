#ifndef CORE_GRAPHICS_WINDOW_HPP
#define CORE_GRAPHICS_WINDOW_HPP

#include <string>
#include <cstdint>

namespace core::window {

class Window {
public:
    virtual ~Window() = default;

    virtual bool should_close() const = 0;
    virtual void wait_events() = 0;
    virtual void poll() = 0;

    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;

    virtual void* native_handle() const = 0;

protected:
    Window() = default;
};

} // namespace core::graphics

#endif // CORE_WINDOW_WINDOW_HPP
