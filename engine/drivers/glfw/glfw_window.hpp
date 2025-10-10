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
    GlfwWindow(void* instance, const std::string& title, uint32_t width, uint32_t height);
    ~GlfwWindow() override;

    bool should_close() const override;
    void wait_events() override;
    void poll() override;

    void query_support(const core::graphics::Device& device);

    const wk::ext::glfw::Surface& surface() const override { return _surface; }

    uint32_t width() const override { return _width; }
    uint32_t height() const override { return _height; }
    const core::graphics::ImageFormat color_format() const { return _color_format; }
    const core::graphics::ColorSpace color_space() const { return _color_space; }
    const core::graphics::ImageFormat depth_format() const { return _depth_format; }

    void* native_handle() const override { return static_cast<void*>(_window); }

private:
    VkInstance _instance;

    GLFWwindow* _window = nullptr;
    wk::ext::glfw::Surface _surface;

    uint32_t _width = 0;
    uint32_t _height = 0;

    core::graphics::ImageFormat _color_format;
    core::graphics::ColorSpace _color_space;
    core::graphics::ImageFormat _depth_format;
};

} // namespace engine::core::window:

#endif // CORE_WINDOW_GLFW_WINDOW_HPP
