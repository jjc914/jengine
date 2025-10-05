#ifndef CORE_WINDOW_GLFW_WINDOW_HPP
#define CORE_WINDOW_GLFW_WINDOW_HPP

#include "window.hpp"

#include <wk/ext/glfw/glfw_internal.hpp>

#include <stdexcept>

namespace core::window::glfw {

class GlfwWindow final : public Window {
public:
    GlfwWindow(void* instance, const std::string& title, uint32_t width, uint32_t height);
    ~GlfwWindow() override;

    bool should_close() const override;
    void wait_events() override;
    void poll() override;

    uint32_t width() const override { return _width; }
    uint32_t height() const override { return _height; }

    void* native_handle() const override { return static_cast<void*>(_window); }

private:
    GLFWwindow* _window = nullptr;

    uint32_t _width = 0;
    uint32_t _height = 0;
};

} // namespace core::window:

#endif // CORE_WINDOW_GLFW_WINDOW_HPP
