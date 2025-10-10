#include "vulkan_instance.hpp"

#include "vulkan_device.hpp"

#include "core/debug/logger.hpp"

namespace engine::drivers::vulkan {

VulkanInstance::VulkanInstance() {
#ifndef NDEBUG
#ifndef WLK_ENABLE_VALIDATION_LAYERS
    core::debug::Logger::get_singleton().warn("Debug mode is enabled but Vulkan validation layers are disabled");
#endif
#endif
#ifdef WLK_ENABLE_VALIDATION_LAYERS
    VkDebugUtilsMessengerCreateInfoEXT debug_ci = wk::DebugMessengerCreateInfo{}
        .set_message_severity(
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        .set_message_type(
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        .set_user_callback(wk::DefaultDebugMessengerCallback)
        .to_vk();
#endif

    std::vector<const char*> instance_extensions = wk::ext::glfw::GetDefaultGlfwRequiredInstanceExtensions();
    ENGINE_ASSERT(!instance_extensions.empty(), "Failed to retrieve required GLFW instance extensions");

    std::vector<const char*> instance_layers;
#ifdef WLK_ENABLE_VALIDATION_LAYERS
    instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    instance_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    VkApplicationInfo application_info = wk::ApplicationInfo{}
        .set_application_name("editor")
        .set_application_version(VK_MAKE_VERSION(1, 0, 0))
        .set_engine_name("No Engine")
        .set_engine_version(VK_MAKE_VERSION(1, 0, 0))
        .set_api_version(VK_API_VERSION_1_3)
        .to_vk();

    _instance = wk::Instance(
        wk::InstanceCreateInfo{}
            .set_flags(VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR)
            .set_extensions(instance_extensions.size(), instance_extensions.data())
            .set_layers(instance_layers.size(), instance_layers.data())
            .set_p_application_info(&application_info)
#ifdef WLK_ENABLE_VALIDATION_LAYERS
            .set_p_next(&debug_ci)
#endif
            .to_vk()
    );
#ifdef WLK_ENABLE_VALIDATION_LAYERS
    _debug_messenger = wk::DebugMessenger(_instance.handle(), debug_ci);
#endif
}

std::unique_ptr<core::graphics::Device> VulkanInstance::create_device(const core::window::Window& window) const {
    return std::make_unique<VulkanDevice>(*this, window);
}

}