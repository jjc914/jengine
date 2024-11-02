#include "ImGuiLayer.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <iostream>

static void check_vk_result(VkResult err) {
    if (err == 0)
        return;
    std::cerr << "[vulkan] Error: VkResult = " << err << std::endl;
    if (err < 0)
        abort();
}

void ImGuiLayer::initialize(VulkanApi* vk_api, GlfwApi* glfw_api, void* window) {
    _vk_api = vk_api;
    _glfw_api = glfw_api;
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)_glfw_api->get_window_handle(window), true);

    _descriptor_pool = _vk_api->create_descriptor_pool();

    RenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.image_format = RenderPassCreateInfo::ImageFormat::FORMAT_SRGB8;
    _render_pass = _vk_api->create_render_pass(&render_pass_create_info);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = (VkInstance)_vk_api->get_instance_handle();
    init_info.PhysicalDevice = (VkPhysicalDevice)_vk_api->get_physical_device_handle();
    init_info.Device = (VkDevice)_vk_api->get_device_handle();
    init_info.QueueFamily = (uint64_t)_vk_api->get_graphics_queue_family();
    init_info.Queue = (VkQueue)_vk_api->get_graphics_queue_handle();
    init_info.DescriptorPool = (VkDescriptorPool)_vk_api->get_descriptor_pool_handle(_descriptor_pool);
    init_info.RenderPass = (VkRenderPass)_vk_api->get_render_pass_handle(_render_pass);
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);
}

void ImGuiLayer::update() {
}

void ImGuiLayer::on_event() {
}

void ImGuiLayer::destroy() {
}
