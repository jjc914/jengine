#include "core/GraphicsApi.hpp"
#include "core/WindowApi.hpp"

#include "renderer/vulkan/VulkanApi.hpp"
#include "window/glfw/GlfwApi.hpp"
#include "editor_ui/ImGuiLayer.hpp"

#include <iostream>

int main(int argc, const char* argv[]) {
    VulkanApi vlk_api;
    GlfwApi glfw_api;
    GraphicsApi* graphics_api = &vlk_api;
    WindowApi* window_api = &glfw_api;
    ImGuiLayer imgui_layer = ImGuiLayer();
    
    void* instance = graphics_api->create_instance(window_api->get_required_extensions());
    void* window = window_api->create_window(980, 720, "Vulkan");
    void* surface = window_api->create_window_surface(instance, window);
    graphics_api->create_device(surface);

    uint32_t width, height;
    window_api->get_window_size(window, &width, &height);
    void* swapchain = graphics_api->create_swapchain(width, height);
    void* render_pass = graphics_api->create_render_pass(swapchain);
    void* pipeline = graphics_api->create_pipeline(swapchain, render_pass);

    imgui_layer.initialize(window_api->get_window_handle(window), instance, graphics_api->get_physical_device(), graphics_api->get_device(), graphics_api->get_graphics_queue(), graphics_api->get_render_pass_handle(render_pass));

    while (!window_api->window_should_close(window)) {
        window_api->poll_events();
        imgui_layer.update();
        graphics_api->draw_frame(swapchain, pipeline);
    }

    graphics_api->wait_device_idle();
    graphics_api->destroy_pipeline(pipeline);
    graphics_api->destroy_render_pass(render_pass);
    graphics_api->destroy_swapchain(swapchain);
    graphics_api->destroy_device();
    graphics_api->destroy_instance();
    window_api->destroy_window(window);

    return EXIT_SUCCESS;
}
