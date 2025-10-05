#include "glfw_window.hpp"

namespace core::window::glfw {

GlfwWindow::GlfwWindow(void* instance, const std::string& title, uint32_t width, uint32_t height)
    : _width(width), _height(height)
{
    _window = glfwCreateWindow(static_cast<int>(width),
                               static_cast<int>(height),
                               title.c_str(),
                               nullptr,
                               nullptr);
    if (!_window) {
        glfwTerminate();
        throw std::runtime_error("failed to create glfw window");
    }
}

GlfwWindow::~GlfwWindow() {
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    glfwTerminate();
}

bool GlfwWindow::should_close() const {
    return glfwWindowShouldClose(_window);
}

void GlfwWindow::wait_events() {
    glfwWaitEvents();
}

void GlfwWindow::poll() {
    glfwPollEvents();
    
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    _width = static_cast<uint32_t>(width);
    _height = static_cast<uint32_t>(height);
}

} // namespace core::window::glfw