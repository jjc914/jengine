#ifndef engine_drivers_glfw_GLFW_WINDOW_HPP
#define engine_drivers_glfw_GLFW_WINDOW_HPP

#include "engine/core/window/window.hpp"

#include <wk/wulkan.hpp>
#include <wk/ext/glfw/glfw_internal.hpp>
#include <wk/ext/glfw/surface.hpp>

#include <stdexcept>

namespace engine::drivers::glfw {

class GlfwWindow final : public core::window::Window {
public:
    GlfwWindow(const std::string& title, uint32_t width, uint32_t height);
    ~GlfwWindow() override;

    bool should_close() const override;
    void wait_events() override;
    void poll() override;

    uint32_t width() const override { return _width; }
    uint32_t height() const override { return _height; }

    void* native_window() const override { return static_cast<void*>(_window); }

private:
    VkInstance _instance;

    GLFWwindow* _window = nullptr;

    uint32_t _width = 0;
    uint32_t _height = 0;
};

} // namespace engine::core::window:

#endif // CORE_WINDOW_GLFW_WINDOW_HPP
