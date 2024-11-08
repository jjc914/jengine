#include "core/api/GraphicsApi.hpp"
#include "core/api/WindowApi.hpp"

#include "renderer/vulkan/VulkanApi.hpp"
#include "window/glfw/GlfwApi.hpp"
#include "editor_ui/ImGuiLayer.hpp"

#include "core/ecs/EcsManager.hpp"

#include <iostream>

struct PositionRotationSystem {
    void operator()(Position p, Rotation r) {
        std::cout << "p: " << p.x << ", " << p.y << ", " << p.z << '\n';
        std::cout << "r: " << r.x << ", " << r.y << ", " << r.z << '\n';
    }
};

void test_ecs() {
    EcsManager manager;

    Entity entity_a = manager.create_entity();
    Entity entity_b = manager.create_entity();
    manager.add_components<Position, Rotation>(entity_a, Position{5.0f, 2.0f, 4.0f}, Rotation{1.0f, 0.05f, 0.01f});
    manager.add_components<Position, Rotation>(entity_b, Position{2.0f, 1.0f, 3.0f}, Rotation{5.0f, 0.01f, 0.02f});

    manager.register_system<Position, Rotation>(PositionRotationSystem{});
    manager.update();
}

int main(int argc, const char* argv[]) {
    test_ecs();
    return 0;

    VulkanApi vulkan_api;
    GraphicsApi* graphics_api = &vulkan_api;

    GlfwApi glfw_api;
    WindowApi* window_api = &glfw_api;

    ImGuiLayer imgui_layer = ImGuiLayer();
    
    void* instance = graphics_api->create_instance(window_api->get_required_extensions());
    void* window = window_api->create_window(980, 720, "Vulkan");
    void* surface = window_api->create_window_surface(instance, window);
    graphics_api->create_device(surface);

    uint32_t width, height;
    window_api->get_window_size(window, &width, &height);
    void* swapchain = graphics_api->create_swapchain(width, height);

    RenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.image_format = RenderPassCreateInfo::ImageFormat::FORMAT_SRGB8;
    void* render_pass = graphics_api->create_render_pass(&render_pass_create_info);

    void* pipeline = graphics_api->create_pipeline(swapchain, render_pass);

    imgui_layer.initialize((VulkanApi*)graphics_api, (GlfwApi*)window_api, window);

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
