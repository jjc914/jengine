#include "GlfwApi.hpp"

#include <iostream>

GlfwApi::GlfwApi() {
    glfwInit();
}

GlfwApi::~GlfwApi() {
    glfwTerminate();
}

void* GlfwApi::create_window(uint32_t width, uint32_t height, std::string title) {
    size_t window_i = _windows.emplace(nullptr);
    GLFWwindow*& window = _windows[window_i];
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    return (void*)(window_i);
}

void* GlfwApi::create_window_surface(void* instance, void* window) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface((VkInstance)instance, _windows[(size_t)window], nullptr, &surface)) {
        std::cerr << "failed to create window surface and window" << std::endl;
    }
    std::clog << "created window surface and window" << std::endl;
    return (void*)surface;
}

void GlfwApi::poll_events() {
    glfwPollEvents();
}

void GlfwApi::get_window_size(void* window, uint32_t* width, uint32_t* height) const {
    glfwGetFramebufferSize(_windows[(size_t)window], (int*)width, (int*)height);
    while(width == 0 || height == 0) {
        glfwGetFramebufferSize(_windows[(size_t)window], (int*)width, (int*)height);
        glfwWaitEvents();
    }
}

const std::vector<const char*> GlfwApi::get_required_extensions() const {
    uint32_t glfw_required_extension_count = 0;
    const char** glfw_required_extensions = glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);
    std::vector<const char*> required_extensions;
    required_extensions.reserve(glfw_required_extension_count);
    for(uint32_t i = 0; i < glfw_required_extension_count; ++i) {
        required_extensions.emplace_back(glfw_required_extensions[i]);
    }
    return required_extensions;
}

bool GlfwApi::window_should_close(void* window) const {
    return glfwWindowShouldClose(_windows[(size_t)window]);
}

void GlfwApi::destroy_window(void* window) {
    glfwDestroyWindow(_windows[(size_t)window]);
}

void* GlfwApi::get_window_handle(void* window) {
    return _windows[(size_t)window];
}
