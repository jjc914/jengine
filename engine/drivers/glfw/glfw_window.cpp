#include "glfw_window.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include "engine/core/graphics/device.hpp"
#include "engine/drivers/vulkan/convert_vulkan.hpp"

namespace engine::drivers::glfw {

GlfwWindow::GlfwWindow(void* instance, const std::string& title, uint32_t width, uint32_t height)
    : _width(width), _height(height)
{
    ENGINE_ASSERT(instance != nullptr, "GLFW window creation requires a valid Vulkan instance");
    ENGINE_ASSERT(width > 0 && height > 0, "GLFW window must have non-zero dimensions");
    ENGINE_ASSERT(!title.empty(), "GLFW window title must not be empty");

    _window = glfwCreateWindow(static_cast<int>(width),
                               static_cast<int>(height),
                               title.c_str(),
                               nullptr,
                               nullptr);

    if (!_window) {
        glfwTerminate();
        core::debug::Logger::get_singleton().fatal("failed to create glfw window");
        return;
    }
    
    _surface = wk::ext::glfw::Surface(static_cast<VkInstance>(instance), _window);
    ENGINE_ASSERT(_surface.handle() != VK_NULL_HANDLE, "Failed to create Vulkan surface for GLFW window");
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

void GlfwWindow::query_support(const core::graphics::Device& device) {
    wk::PhysicalDeviceSurfaceSupport support = wk::GetPhysicalDeviceSurfaceSupport(
        static_cast<VkPhysicalDevice>(device.native_physical_device()), _surface.handle()
    );
    VkSurfaceFormatKHR surface_format = wk::ChooseSurfaceFormat(support.formats);
    _color_format = vulkan::FromImageVkFormat(surface_format.format);
    _color_space = vulkan::FromVkColorSpace(surface_format.colorSpace);
    _depth_format = vulkan::FromImageVkFormat(wk::ChooseDepthFormat(static_cast<VkPhysicalDevice>(device.native_physical_device())));
}

} // namespace drivers::glfw