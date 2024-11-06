#include "VulkanApi.hpp"

#include <iostream>
#include <map>
#include <set>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>

#define VLK_ENABLE_VALIDATION_LAYERS

VulkanApi::VulkanApi() {}

void* VulkanApi::create_instance(const std::vector<const char*>& required_extensions) {
#ifdef VLK_ENABLE_VALIDATION_LAYERS
    if (!_is_validation_layers_supported()) {
        std::clog << "validation layers requested, but not available" << std::endl;
    } else {
        std::clog << "validation layers enabled" << std::endl;
    }
#endif
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Application";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> extensions;
    if (!_is_instance_extensions_supported(&extensions, required_extensions)) {
        std::cerr << "not all required extensions are available" << std::endl;
    }

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = _debug_callback;
    debug_create_info.pUserData = nullptr;

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    
    instance_create_info.enabledExtensionCount =(uint32_t)extensions.size();
    instance_create_info.ppEnabledExtensionNames = extensions.data();
    
#ifdef VLK_ENABLE_VALIDATION_LAYERS
    instance_create_info.enabledLayerCount =(uint32_t)_VALIDATION_LAYERS.size();
    instance_create_info.ppEnabledLayerNames = _VALIDATION_LAYERS.data();
    
    instance_create_info.pNext =(VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
#else
    instance_create_info.enabledLayerCount = 0;
    instance_create_info.pNext = nullptr;
#endif

    if (vkCreateInstance(&instance_create_info, nullptr, &_instance)) {
        std::cerr << "failed to create Vulkan instance" << std::endl;
    }

#ifdef VLK_ENABLE_VALIDATION_LAYERS
    _init_debug_messenger();
#endif
    return (void*)_instance;
}

void VulkanApi::create_device(void* surface) {
    _surface = (VkSurfaceKHR)surface;
    _init_physical_device();
    _queue_family_indices = _find_queue_families(_physical_device);
    _init_device();
}

void* VulkanApi::create_swapchain(uint32_t width, uint32_t height) {
    size_t resources_i = _swapchain_resources.emplace();
    SwapchainResources& resources = _swapchain_resources[resources_i];
    resources.width = width;
    resources.height = height;

    size_t swapchain_i = _create_swapchain(resources);

    _create_swapchain_image_views(swapchain_i);

    RenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.image_format = _vk_format_to_image_format(_swapchain_resources[swapchain_i].image_format);
    size_t render_pass = (size_t)create_render_pass(&render_pass_create_info);
    resources.render_pass = render_pass;

    _create_swapchain_framebuffers(swapchain_i);

    size_t command_pool = (size_t)create_command_pool();
    resources.command_pool = command_pool;

    std::vector<void*> command_buffers = create_command_buffers((void*)command_pool, _swapchain_resources[swapchain_i].max_frames_in_flight);
    std::vector<size_t> command_buffers_int;
    command_buffers_int.reserve(resources.max_frames_in_flight);
    for (size_t i = 0; i < command_buffers.size(); ++i) {
        command_buffers_int.emplace_back((size_t)command_buffers[i]);
    }
    _command_pools[command_pool].command_buffers = command_buffers_int;

    _create_swapchain_semaphores(swapchain_i);
    _create_swapchain_fences(swapchain_i);

    return (void*)swapchain_i;
}

void* VulkanApi::create_render_pass(RenderPassCreateInfo* create_info) {
    VkAttachmentDescription color_attachment_description{};
    color_attachment_description.format = _image_format_to_vk_format(create_info->image_format);
    color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference color_attachment_reference{};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass_description{};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;
    
    VkSubpassDependency subpass_dependency{};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;

    size_t render_pass_i = _render_passes.emplace();
    if (vkCreateRenderPass(_device, &render_pass_create_info, nullptr, &(_render_passes[render_pass_i]))) {
        std::cerr << "failed to create render pass" << std::endl;
    }
    std::clog << "created render pass" << std::endl;
    return (void*)render_pass_i;
}

void* VulkanApi::create_shader(std::string path) {
    std::vector<uint8_t> code = _read_spirv_shader(path);

    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.flags = 0;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    size_t shader_i = _shaders.emplace();    
    if (vkCreateShaderModule(_device, &create_info, nullptr, &_shaders[shader_i]) != VK_SUCCESS) {
        std::cerr << "failed to create shader module" << std::endl;
    }
    return (void*)shader_i;
}

void* VulkanApi::create_pipeline(void* swapchain_i, void* render_pass_i) {
    size_t vert_i = (size_t)create_shader("shaders/triangle.vert.spv");
    size_t frag_i = (size_t)create_shader("shaders/triangle.frag.spv");

    VkShaderModule& vert = _shaders[vert_i]; // TODO: keep a handle to the shader modules for deletion
    VkShaderModule& frag = _shaders[frag_i];

    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_infos(2);

    shader_stage_create_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stage_create_infos[0].module = vert;
    shader_stage_create_infos[0].pName = "main";
    shader_stage_create_infos[0].pSpecializationInfo = nullptr;

    shader_stage_create_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stage_create_infos[1].module = frag;
    shader_stage_create_infos[1].pName = "main";
    shader_stage_create_infos[1].pSpecializationInfo = nullptr;

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount =(uint32_t)dynamic_states.size();
    dynamic_state_create_info.pDynamicStates = dynamic_states.data();

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
    vertex_input_state_create_info.pVertexBindingDescriptions = nullptr;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
    vertex_input_state_create_info.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_swapchain_resources[(size_t)swapchain_i].extent.width;
    viewport.height = (float)_swapchain_resources[(size_t)swapchain_i].extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = _swapchain_resources[(size_t)swapchain_i].extent;

    VkPipelineViewportStateCreateInfo viewport_state_create_info{};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.scissorCount = 1;
        
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.lineWidth = 1.0f;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
    
    VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
    color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable = VK_TRUE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 0.0f;
    color_blend_state_create_info.blendConstants[1] = 0.0f;
    color_blend_state_create_info.blendConstants[2] = 0.0f;
    color_blend_state_create_info.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 0;
    pipeline_layout_create_info.pushConstantRangeCount = 0;

    size_t pipeline_layout_i = _pipeline_layouts.emplace();
    if (vkCreatePipelineLayout(_device, &pipeline_layout_create_info, nullptr, &_pipeline_layouts[pipeline_layout_i]) != VK_SUCCESS) {
        std::cerr << "failed to create pipeline layout" << std::endl;
    }
    std::clog << "created pipeline layout" << std::endl;

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = shader_stage_create_infos.size();
    pipeline_create_info.pStages = shader_stage_create_infos.data();
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    
    pipeline_create_info.layout = _pipeline_layouts[pipeline_layout_i];
    pipeline_create_info.renderPass = _render_passes[(size_t)render_pass_i];
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    
    size_t pipeline_i = _pipelines.emplace();
    if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &_pipelines[pipeline_i]) != VK_SUCCESS) {
        std::cerr << "failed to create graphics pipeline" << std::endl;
    }
    std::clog << "created graphics pipeline" << std::endl;

    destroy_shader((void*)vert_i);
    destroy_shader((void*)frag_i);

    return (void*)pipeline_i;
}

void* VulkanApi::create_command_pool() {
    size_t command_pool_i = _command_pools.emplace();
        
    VkCommandPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = _queue_family_indices.graphics_family.value();
    
    if (vkCreateCommandPool(_device, &create_info, nullptr, &_command_pools[command_pool_i].command_pool) != VK_SUCCESS) {
        std::cerr << "failed to create command pool" << std::endl;
    }
    std::clog << "created command pool" << std::endl;

    return (void*)command_pool_i;
}

std::vector<void*> VulkanApi::create_command_buffers(void* command_pool_i, size_t count) {
    std::vector<void*> command_buffers;
    command_buffers.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        size_t command_buffer_i = _command_buffers.emplace();

        VkCommandBufferAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        
        allocate_info.commandPool = _command_pools[(size_t)command_pool_i].command_pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(_device, &allocate_info, &_command_buffers[command_buffer_i]) != VK_SUCCESS) {
            std::cerr << "failed to allocate command buffers" << std::endl;
        }

        command_buffers.emplace_back((void*)command_buffer_i);
    }
    
    std::clog << "allocated command buffers" << std::endl;

    return command_buffers;
}

void* VulkanApi::create_descriptor_pool() {
    VkDescriptorPoolSize pool_sizes[] =
	{
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

    size_t descriptor_pool_i = _descriptor_pools.emplace();
	if (vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptor_pools[descriptor_pool_i]) != VK_SUCCESS) {
        std::cerr << "failed to create descriptor pool" << std::endl;
    }
    return (void*)descriptor_pool_i;
}

void VulkanApi::draw_frame(void* swapchain_i, void* pipeline_i) {
    VkSwapchainKHR& sc = _swapchains[(size_t)swapchain_i];
    SwapchainResources& resources = _swapchain_resources[(size_t)swapchain_i];
    vkWaitForFences(_device, 1, &_fences[resources.frame_in_flight_fences[resources.current_frame_resource_index]], VK_TRUE, UINT64_MAX);
        
    VkResult result;

    uint32_t image_index;
    result = vkAcquireNextImageKHR(_device, sc, UINT64_MAX, _semaphores[resources.image_available_semaphores[resources.current_frame_resource_index]], VK_NULL_HANDLE, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || resources.is_dirty) {
        resources.is_dirty = false;
        _update_swapchain((size_t)swapchain_i);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "failed to acquire swap chain image" << std::endl;
    }
    vkResetFences(_device, 1, &_fences[resources.frame_in_flight_fences[resources.current_frame_resource_index]]);

    VkCommandBuffer& buffer = _command_buffers[_command_pools[resources.command_pool].command_buffers[resources.current_frame_resource_index]];
    vkResetCommandBuffer(buffer, 0);

    _record_command_buffer(buffer, _pipelines[(size_t)pipeline_i], resources, image_index);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore wait_semaphores[] = { _semaphores[resources.image_available_semaphores[resources.current_frame_resource_index]] };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;

    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;
    
    VkSemaphore signal_semaphores[] = { _semaphores[resources.render_finished_semaphores[resources.current_frame_resource_index]] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    
    vkQueueSubmit(_graphics_queue, 1, &submit_info, _fences[resources.frame_in_flight_fences[resources.current_frame_resource_index]]);

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = { _swapchains[(size_t)swapchain_i] };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;

    present_info.pImageIndices = &image_index;

    result = vkQueuePresentKHR(_present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        _update_swapchain((size_t)swapchain_i);
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "failed to acquire swap chain image" << std::endl;
    }
    
    resources.current_frame_resource_index =(resources.current_frame_resource_index + 1) % resources.max_frames_in_flight;
}

void VulkanApi::update_swapchain(void* swapchain_i, uint32_t width, uint32_t height) {
    _swapchain_resources[(size_t)swapchain_i].width = width;
    _swapchain_resources[(size_t)swapchain_i].height = height;
    _swapchain_resources[(size_t)swapchain_i].is_dirty = true;
    _update_swapchain((size_t)swapchain_i);
}

bool VulkanApi::_is_validation_layers_supported() const {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layer_properties(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layer_properties.data());
    
    for (const auto& required_layer : _VALIDATION_LAYERS) {
        bool is_supported = false;
        for (const auto& available_layer_property : available_layer_properties) {
            if (strcmp(required_layer, available_layer_property.layerName) == 0) {
                is_supported = true;
                break;
            }
        }
        if (!is_supported) {
            return false;
        }
    }
    return true;
}

int32_t VulkanApi::_rate_physical_device(VkPhysicalDevice device) const {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);
    
    int32_t score = 0;
    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    } else if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU){
        score += 500;
    } else {
        return 0;
    }
    if (!_is_device_suitable(device)) {
        return 0;
    }
    return score;
}

bool VulkanApi::_is_device_suitable(VkPhysicalDevice device) const {
    QueueFamilyIndices indices = _find_queue_families(device);
    bool is_extensions_supported = _is_device_extension_supported(device);
    
    bool is_swapchain_adequate = false;
    if (is_extensions_supported) {
        SwapchainSupportDetails swapchain_support = _query_swapchain_support(device);
        is_swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
    }

    return indices.is_complete() && is_extensions_supported && is_swapchain_adequate;
}

bool VulkanApi::_is_instance_extensions_supported(std::vector<const char*>* enabled_extensions, std::vector<const char*> required_extensions) const {
#ifdef __APPLE__
    required_extensions.insert(required_extensions.end(), _PLATFORM_REQUIRED_INSTANCE_EXTENSIONS.begin(), _PLATFORM_REQUIRED_INSTANCE_EXTENSIONS.end());
#endif
    
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extension_properties(extension_count);
    
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extension_properties.data());
    
#ifdef VLK_ENABLE_VALIDATION_LAYERS
    required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    (*enabled_extensions) = required_extensions;
    
    for (const auto& required_extension : required_extensions) {
        bool is_supported = false;
        for (const auto& available_extension_property : available_extension_properties) {
            if (strcmp(required_extension, available_extension_property.extensionName) == 0) {
                is_supported = true;
                break;
            }
        }
        if (!is_supported) {
            return false;
        }
    }
    return true;
}

QueueFamilyIndices VulkanApi::_find_queue_families(VkPhysicalDevice device) const {
    QueueFamilyIndices indices{};
        
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
    
    for (uint32_t i = 0; i < queue_families.size(); ++i) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }
        
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &present_support);
        if (present_support) {
            indices.present_family = i;
        }
        
        if (indices.is_complete()) {
            break;
        }
    }
    return indices;
}

bool VulkanApi::_is_device_extension_supported(VkPhysicalDevice device) const {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());
    
    for(const auto& required_extension : _REQUIRED_DEVICE_EXTENSIONS) {
        bool is_supported = false;
        for(const auto& availableExtension : available_extensions) {
            if(strcmp(required_extension, availableExtension.extensionName) == 0) {
                is_supported = true;
                break;
            }
        }
        if(!is_supported) {
            return false;
        }
    }
    return true;
}

SwapchainSupportDetails VulkanApi::_query_swapchain_support(VkPhysicalDevice device) const {
    SwapchainSupportDetails details;
        
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);
    
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &format_count, nullptr);
    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &format_count, details.formats.data());
    }
    
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_mode_count, nullptr);
    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_mode_count, details.present_modes.data());
    }
    
    return details;
}

VkSurfaceFormatKHR VulkanApi::_choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) const {
    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }
    
    return available_formats[0]; // TODO: rank and choose best format
}

VkPresentModeKHR VulkanApi::_choose_swap_surface_presentation_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanApi::_choose_swap_surface_extent(uint32_t width, uint32_t height, const VkSurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    VkExtent2D actual_extent = {
        (uint32_t)width,
        (uint32_t)height
    };

    actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actual_extent;
}

std::vector<uint8_t> VulkanApi::_read_spirv_shader(const std::string& file_name) const {
    std::filesystem::path bin_dir;
    #ifdef __APPLE__
    char buf[PATH_MAX];
    uint32_t bufsize = PATH_MAX;
    if (_NSGetExecutablePath(buf, &bufsize) != 0) {
        std::cerr << "failed to get executable path" << std::endl;
    }
    bin_dir = buf;
    bin_dir.remove_filename();
#else
    std::cerr << "reading executable path not implemented for non-apple devices" << std::endl;
#endif
    std::clog << "reading shader: " << bin_dir / file_name << std::endl;
    
    std::ifstream file(bin_dir / file_name, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "failed to open shader" << std::endl;
    }
    
    size_t file_size = (size_t)file.tellg();
    std::vector<uint8_t> buffer(file_size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), file_size);
    file.close();

    return buffer;
}

void VulkanApi::_init_debug_messenger() {
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = _debug_callback;
    create_info.pUserData = nullptr;

    auto vkCreateDebugUtilsMessengerEXT =(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT == nullptr) {
        std::cerr << "failed to create vkCreateDebugUtilsMessengerEXT function" << std::endl;;
    }
    if (vkCreateDebugUtilsMessengerEXT(_instance, &create_info, nullptr, &_debug_messenger) != VK_SUCCESS) {
        std::cerr << "failed to create debug messenger" << std::endl;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApi::_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                             VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                             void* pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cerr << pCallbackData->pMessage << std::endl;
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::clog << pCallbackData->pMessage << std::endl;
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        std::clog << pCallbackData->pMessage << std::endl;
    } else {
        std::clog << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

void VulkanApi::_init_physical_device() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(_instance, &device_count, nullptr);
    if (device_count == 0) {
        std::cerr << "failed to find GPUs with Vulkan support" << std::endl;
    }
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(_instance, &device_count, devices.data());

    std::multimap<int32_t, VkPhysicalDevice> device_scores;
    for (const auto& device : devices) {
        int32_t score = _rate_physical_device(device);
        device_scores.insert({ score, device });
    }

    if (device_scores.rbegin()->first > 0) {
        _physical_device = device_scores.rbegin()->second;
    } else {
        std::cerr << "failed to find a suitable GPU" << std::endl;
    }

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(_physical_device, &device_properties);
    std::clog << "found GPU: " << device_properties.deviceName << std::endl;
}

void VulkanApi::_init_device() {
    VkPhysicalDeviceFeatures device_features{};
        
    std::vector<const char*> enabled_extensions;
    enabled_extensions.insert(enabled_extensions.end(), _PLATFORM_REQUIRED_DEVICE_EXTENSIONS.begin(), _PLATFORM_REQUIRED_DEVICE_EXTENSIONS.end());
    enabled_extensions.insert(enabled_extensions.end(), _REQUIRED_DEVICE_EXTENSIONS.begin(), _REQUIRED_DEVICE_EXTENSIONS.end());

    std::set<uint32_t> unique_queue_families = { _queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value() };
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos(unique_queue_families.size());
    int32_t i = 0;
    for (uint32_t index : unique_queue_families) {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = index;
        queue_create_infos[i].queueCount = 1;
        float queue_priority = 1.0;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
        ++i;
    }
    
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = queue_create_infos.size();
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount =(uint32_t)enabled_extensions.size();
    create_info.ppEnabledExtensionNames = enabled_extensions.data();
#ifdef VLK_ENABLE_VALIDATION_LAYERS
    create_info.enabledLayerCount = static_cast<uint32_t>(_VALIDATION_LAYERS.size());
    create_info.ppEnabledLayerNames = _VALIDATION_LAYERS.data();
#else
    create_info.enabledLayerCount = 0;
#endif
    
    if (vkCreateDevice(_physical_device, &create_info, nullptr, &_device) != VK_SUCCESS) {
        std::cerr << "failed to create logical device" << std::endl;
    }
    
    vkGetDeviceQueue(_device, _queue_family_indices.graphics_family.value(), 0, &_graphics_queue);
    vkGetDeviceQueue(_device, _queue_family_indices.present_family.value(), 0, &_present_queue);
    std::clog << "created logical device" << std::endl;
}

size_t VulkanApi::_create_swapchain(SwapchainResources& resources) {
    SwapchainSupportDetails details = _query_swapchain_support(_physical_device);

    VkSurfaceFormatKHR surface_format = _choose_swap_surface_format(details.formats);
    VkPresentModeKHR presentMode = _choose_swap_surface_presentation_mode(details.present_modes);
    VkExtent2D extent = _choose_swap_surface_extent(resources.width, resources.height, details.capabilities);
    
    uint32_t image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0) {
        image_count = std::clamp(image_count,
                                details.capabilities.minImageCount,
                                details.capabilities.maxImageCount);
    }
    
    VkSwapchainCreateInfoKHR create_info{};
    uint32_t queue_family_indices[] = {_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()};
    
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = _surface;
    
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    if (_queue_family_indices.graphics_family != _queue_family_indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    create_info.preTransform = details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = presentMode;
    create_info.clipped = VK_TRUE;
    
    create_info.oldSwapchain = VK_NULL_HANDLE;
    
    size_t swapchain_i = _swapchains.emplace();
    if (vkCreateSwapchainKHR(_device, &create_info, nullptr, &_swapchains[swapchain_i]) != VK_SUCCESS) {
        std::cerr << "failed to create swapchain" << std::endl;
    }

    VkSwapchainKHR& swapchain = _swapchains[swapchain_i];
    vkGetSwapchainImagesKHR(_device, swapchain, &image_count, nullptr);
    resources.images.resize(image_count);
    vkGetSwapchainImagesKHR(_device, swapchain, &image_count, resources.images.data());

    resources.image_format = surface_format.format;
    resources.extent = extent;
    
    std::clog << "created swapchain" << std::endl;;

    return swapchain_i;
}

void VulkanApi::_create_swapchain_image_views(size_t swapchain_i) {
    SwapchainResources& resources = _swapchain_resources[swapchain_i];
    
    resources.image_views.resize(resources.images.size());
    for (size_t i = 0; i < resources.image_views.size(); ++i) {
        size_t image_view_i = _image_views.emplace();

        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        
        create_info.image = resources.images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = resources.image_format;
        
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(_device, &create_info, nullptr, &_image_views[image_view_i]) != VK_SUCCESS) {
            std::cerr << "failed to create image views" << std::endl;
        }
        std::clog << "created " << resources.image_views.size() << " image views" << std::endl;

        resources.image_views[i] = image_view_i;
    }
}

void VulkanApi::_create_swapchain_framebuffers(size_t swapchain_i) {
    SwapchainResources& resources = _swapchain_resources[swapchain_i];
    
    resources.framebuffers.resize(resources.images.size());        
    for (size_t i = 0; i < resources.framebuffers.size(); ++i) {
        size_t framebuffer_i = _framebuffers.emplace();

        std::vector<VkImageView> attachments(1);
        attachments[0] = _image_views[resources.image_views[i]];
        
        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = _render_passes[resources.render_pass];
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments.data();
        create_info.width = resources.extent.width;
        create_info.height = resources.extent.height;
        create_info.layers = 1;
        
        if (vkCreateFramebuffer(_device, &create_info, nullptr, &_framebuffers[framebuffer_i]) != VK_SUCCESS) {
            std::cerr << "failed to create framebuffer" << std::endl;
        }

        resources.framebuffers[i] = framebuffer_i;
    }
    std::clog << "created " << resources.framebuffers.size() << " framebuffers" << std::endl;;
}

void VulkanApi::_create_swapchain_semaphores(size_t swapchain_i) {
    SwapchainResources& resources = _swapchain_resources[swapchain_i];

    resources.image_available_semaphores.resize(resources.max_frames_in_flight);
    resources.render_finished_semaphores.resize(resources.max_frames_in_flight);
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (int32_t i = 0; i < resources.max_frames_in_flight; ++i) {
        size_t semaphore_i = _semaphores.emplace();
        if (vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_semaphores[semaphore_i]) != VK_SUCCESS) {
            std::cerr << "failed to create semaphores" << std::endl;
        }
        resources.image_available_semaphores[i] = semaphore_i;
            
        semaphore_i = _semaphores.emplace();
        if (vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_semaphores[semaphore_i]) != VK_SUCCESS) {
            std::cerr << "failed to create semaphores" << std::endl;
        }
        resources.render_finished_semaphores[i] = semaphore_i;
    }
    std::clog << "created semaphores" << std::endl;
}

void VulkanApi::_create_swapchain_fences(size_t swapchain_i) {
    SwapchainResources& resources = _swapchain_resources[swapchain_i];

    resources.frame_in_flight_fences.resize(resources.max_frames_in_flight);
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (int32_t i = 0; i < resources.max_frames_in_flight; ++i) {
        size_t fence_i = _fences.emplace();
        if (vkCreateFence(_device, &fence_create_info, nullptr, &_fences[fence_i]) != VK_SUCCESS) {
            std::cerr << "failed to create fences" << std::endl;;
        }
        resources.frame_in_flight_fences[i] = fence_i;
    }
    std::clog << "created fences" << std::endl;
}

void VulkanApi::_update_swapchain(size_t swapchain_i) {
    VkSwapchainKHR& swapchain = _swapchains[swapchain_i];
    SwapchainResources& resources = _swapchain_resources[swapchain_i];

    vkDeviceWaitIdle(_device);
    
    destroy_swapchain((void*)swapchain_i);

    SwapchainSupportDetails details = _query_swapchain_support(_physical_device);

    VkSurfaceFormatKHR surface_format = _choose_swap_surface_format(details.formats);
    VkPresentModeKHR presentMode = _choose_swap_surface_presentation_mode(details.present_modes);
    VkExtent2D extent = _choose_swap_surface_extent(resources.width, resources.height, details.capabilities);
    
    uint32_t image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0) {
        image_count = std::clamp(image_count,
                                details.capabilities.minImageCount,
                                details.capabilities.maxImageCount);
    }
    
    VkSwapchainCreateInfoKHR create_info{};
    uint32_t queue_family_indices[] = {_queue_family_indices.graphics_family.value(), _queue_family_indices.present_family.value()};
    
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = _surface;
    
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    if (_queue_family_indices.graphics_family != _queue_family_indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    create_info.preTransform = details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = presentMode;
    create_info.clipped = VK_TRUE;
    
    create_info.oldSwapchain = VK_NULL_HANDLE;
    
    if (vkCreateSwapchainKHR(_device, &create_info, nullptr, &swapchain) != VK_SUCCESS) {
        std::cerr << "failed to create swapchain" << std::endl;
    }
    
    std::clog << "created swapchain";

    vkGetSwapchainImagesKHR(_device, swapchain, &image_count, nullptr);
    resources.images.resize(image_count);
    vkGetSwapchainImagesKHR(_device, swapchain, &image_count, resources.images.data());

    resources.image_format = _choose_swap_surface_format(details.formats).format;
    resources.extent = _choose_swap_surface_extent(resources.width, resources.height, details.capabilities);

    for (size_t i = 0; i < resources.image_views.size(); ++i) {
        VkImageView& image_view = _image_views[resources.image_views[i]];

        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        
        create_info.image = resources.images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = resources.image_format;
        
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(_device, &create_info, nullptr, &image_view) != VK_SUCCESS) {
            std::cerr << "failed to create image views" << std::endl;
        }
        std::clog << "created image views" << std::endl;
    }
   
    for (size_t i = 0; i < resources.framebuffers.size(); ++i) {
        VkFramebuffer& framebuffer = _framebuffers[resources.framebuffers[i]];

        std::vector<VkImageView> attachments(1);
        attachments[0] = _image_views[resources.image_views[i]];
        
        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = _render_passes[resources.render_pass];
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments.data();
        create_info.width = resources.extent.width;
        create_info.height = resources.extent.height;
        create_info.layers = 1;
        
        if (vkCreateFramebuffer(_device, &create_info, nullptr, &framebuffer) != VK_SUCCESS) {
            std::cerr << "failed to create framebuffer" << std::endl;
        }
    }
    std::clog << "created " << resources.framebuffers.size() << " framebuffers" << std::endl;;

    _create_swapchain_semaphores(swapchain_i);
    _create_swapchain_fences(swapchain_i);

    resources.is_dirty = false;

    std::clog << "updated swapchain" << std::endl;
}

void VulkanApi::_record_command_buffer(VkCommandBuffer& buffer, VkPipeline& pipeline, SwapchainResources& resources, uint32_t image_index) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(buffer, &begin_info)) {
        std::cerr << "failed to begin recording command buffer" << std::endl;
    }

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    
    render_pass_info.renderPass = _render_passes[resources.render_pass];
    render_pass_info.framebuffer = _framebuffers[resources.framebuffers[image_index]];
    
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = resources.extent;
    
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;
    
    vkCmdBeginRenderPass(buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)resources.extent.width;
    viewport.height = (float)resources.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = resources.extent;
    vkCmdSetScissor(buffer, 0, 1, &scissor);
    
    vkCmdDraw(buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(buffer);
    if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
        std::cerr << "failed to record command buffer" << std::endl;
    }
}

VkFormat VulkanApi::_image_format_to_vk_format(RenderPassCreateInfo::ImageFormat format) {
    switch(format) {
        case RenderPassCreateInfo::ImageFormat::FORMAT_SRGB8:
            return VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
        default:
            std::cerr << "unknown image format" << std::endl;
            break;
    }
    return (VkFormat)0;
}

RenderPassCreateInfo::ImageFormat VulkanApi::_vk_format_to_image_format(VkFormat format) {
    switch(format) {
        case VkFormat::VK_FORMAT_B8G8R8A8_SRGB:
            return RenderPassCreateInfo::ImageFormat::FORMAT_SRGB8;
        default:
            std::cerr << "unknown image format" << std::endl;
            break;
    }
    return RenderPassCreateInfo::ImageFormat::NONE;
}

void VulkanApi::destroy_instance() {
#ifdef VLK_ENABLE_VALIDATION_LAYERS
    auto vkDestroyDebugUtilsMessengerEXT =(PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
    if(vkDestroyDebugUtilsMessengerEXT != nullptr) {
        vkDestroyDebugUtilsMessengerEXT(_instance, _debug_messenger, nullptr);
    }
#endif
    vkDestroyInstance(_instance, nullptr); 
    std::clog << "destroyed instance" << std::endl;
}

void VulkanApi::destroy_device() {
    vkDestroyDevice(_device, nullptr);

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    std::clog << "destroyed device" << std::endl;
}

void VulkanApi::destroy_swapchain(void* swapchain_i) {
    SwapchainResources& resources = _swapchain_resources[(size_t)swapchain_i];

    for (size_t& semaphore_i : resources.image_available_semaphores) {
        vkDestroySemaphore(_device, _semaphores[semaphore_i], nullptr);
        _semaphores.erase(semaphore_i);
    }

    for (size_t& semaphore_i : resources.render_finished_semaphores) {
        vkDestroySemaphore(_device, _semaphores[semaphore_i], nullptr);
        _semaphores.erase(semaphore_i);
    }

    for (size_t& fence_i : resources.frame_in_flight_fences) {
        vkDestroyFence(_device, _fences[fence_i], nullptr);
        _semaphores.erase(fence_i);
    }

    destroy_command_pool((void*)resources.command_pool);

    for (size_t& framebuffer_i : resources.framebuffers) {
        vkDestroyFramebuffer(_device, _framebuffers[framebuffer_i], nullptr);
        _framebuffers.erase(framebuffer_i);
    }

    for (size_t& image_view_i : resources.image_views) {
        vkDestroyImageView(_device, _image_views[image_view_i], nullptr);
        _image_views.erase(image_view_i);
    }

    destroy_render_pass((void*)resources.render_pass);

    vkDestroySwapchainKHR(_device, _swapchains[(size_t)swapchain_i], nullptr);
    _swapchains.erase((size_t)swapchain_i);
    std::clog << "destroyed swapchain" << std::endl;
}

void VulkanApi::destroy_render_pass(void* render_pass_i) {
    vkDestroyRenderPass(_device, _render_passes[(size_t)render_pass_i], nullptr);
    _render_passes.erase((size_t)render_pass_i);
    std::clog << "destroyed render pass" << std::endl;
}

void VulkanApi::destroy_shader(void* shader_i) {
    vkDestroyShaderModule(_device, _shaders[(size_t)shader_i], nullptr);
    _shaders.erase((size_t)shader_i);
    std::clog << "destroyed shader" << std::endl;
}

void VulkanApi::destroy_pipeline(void* pipeline_i) {
    vkDestroyPipelineLayout(_device, _pipeline_layouts[(size_t)pipeline_i], nullptr);
    vkDestroyPipeline(_device, _pipelines[(size_t)pipeline_i], nullptr);
    _pipelines.erase((size_t)pipeline_i);
    std::clog << "destroyed pipeline" << std::endl;
}

void VulkanApi::destroy_command_pool(void* command_pool_i) {
    for (size_t command_buffer_i : _command_pools[(size_t)command_pool_i].command_buffers) {
        vkFreeCommandBuffers(_device, _command_pools[(size_t)command_pool_i].command_pool, 1, &_command_buffers[command_buffer_i]);
    }

    vkDestroyCommandPool(_device, _command_pools[(size_t)command_pool_i].command_pool, nullptr);
    _command_pools.erase((size_t)command_pool_i);
    std::clog << "destroyed command pool" << std::endl;
}

void VulkanApi::wait_device_idle() {
    vkDeviceWaitIdle(_device);
}

void* VulkanApi::get_render_pass_handle(void* render_pass) {
    return (void*)_render_passes[(size_t)render_pass];
}

void* VulkanApi::get_instance_handle(void) {
    return (void*)_instance;
}

void* VulkanApi::get_physical_device_handle(void) {
    return (void*)_physical_device;
}

void* VulkanApi::get_device_handle(void) {
    return (void*)_device;
}

void* VulkanApi::get_graphics_queue_family(void) {
    return (void*)(uint64_t)_queue_family_indices.graphics_family.value();
}

void* VulkanApi::get_graphics_queue_handle(void) {
    return (void*)_graphics_queue;
}

void* VulkanApi::get_descriptor_pool_handle(void* descriptor_pool) {
    return (void*)_descriptor_pools[(size_t)descriptor_pool];
}

// void* VulkanApi::get_command_pool_handle(void* command_pool) {
//     return (void*);
// }

// void* VulkanApi::get_command_buffer_handle(void* command_buffer) {
//     return (void*);
// }

// void* VulkanApi::get_render_pass_handle(void* render_pass) {
//     return (void*);
// }

// void* VulkanApi::get_framebuffer_handle(void* framebuffer) {
//     return (void*);
// }
