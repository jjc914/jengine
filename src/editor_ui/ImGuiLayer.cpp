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

void ImGuiLayer::initialize(void* window, void* instance, void* physical_device, void* device, void* queue, void* render_pass) {
    IMGUI_CHECKVERSION();

    _device = (VkDevice)device;

    VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	if (vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptor_pool) != VK_SUCCESS) {
        std::cerr << "failed to create descriptor pool" << std::endl;
    }

    size_t graphics_family;
        
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties((VkPhysicalDevice)physical_device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties((VkPhysicalDevice)physical_device, &queue_family_count, queue_families.data());
    
    for (size_t i = 0; i < queue_families.size(); ++i) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_family = i;
            break;
        }
    }

    VkCommandPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = graphics_family;

    if (vkCreateCommandPool(_device, &create_info, nullptr, &_command_pool) != VK_SUCCESS) {
        std::cerr << "failed to create command pool" << std::endl;
    }

    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    
    allocate_info.commandPool = _command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;
    
    if (vkAllocateCommandBuffers(_device, &allocate_info, &_command_buffer)) {
        std::cerr << "failed to allocate command buffer" << std::endl;
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = (VkInstance)instance;
	init_info.PhysicalDevice = (VkPhysicalDevice)physical_device;
	init_info.Device = _device;
	init_info.Queue = (VkQueue)queue;
    init_info.RenderPass = (VkRenderPass)render_pass;
	init_info.DescriptorPool = _descriptor_pool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&init_info);

    VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;

	begin_info.pInheritanceInfo = nullptr;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(_command_buffer, &begin_info) != VK_SUCCESS) {
        std::cerr << "failed to create command buffer" << std::endl;
    }

	ImGui_ImplVulkan_CreateFontsTexture();

	if (vkEndCommandBuffer(_command_buffer)) {
        std::cerr << "failed to create command buffer" << std::endl;
    }
    
    VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = nullptr;
	submit_info.pWaitDstStageMask = nullptr;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &_command_buffer;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = nullptr;

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(_device, &fence_create_info, nullptr, &_fence) != VK_SUCCESS) {
        std::cerr << "failed to create fences" << std::endl;;
    }

    vkResetFences(_device, 1, &_fence);

	if (vkQueueSubmit((VkQueue)queue, 1, &submit_info, _fence) != VK_SUCCESS) {
        std::cerr << "failed to create command buffer" << std::endl;
    }

	vkWaitForFences(_device, 1, &_fence, true, 9999999999);
	vkResetFences(_device, 1, &_fence);

	vkResetCommandPool(_device, _command_pool, 0);

    std::clog << "initialized imgui layer" << std::endl;
}

void ImGuiLayer::update() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow(nullptr);

    ImGui::Render();
}

void ImGuiLayer::on_event() {

}

void ImGuiLayer::destroy() {
    vkDeviceWaitIdle(_device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyFence(_device, _fence, nullptr);
    vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
    vkFreeCommandBuffers(_device, _command_pool, 1, &_command_buffer);
    vkDestroyCommandPool(_device, _command_pool, nullptr);
}
