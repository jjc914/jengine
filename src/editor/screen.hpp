#ifndef SCREEN_HPP
#define SCREEN_HPP

#ifndef NDEBUG
#define VLK_ENABLE_VALIDATION_LAYERS
#else
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <limits>
#include <algorithm>
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "vlk_allocator.hpp"

#include "gui/gui.hpp"
#include "logger/logger.hpp"

namespace gen {
    namespace { // internal structs
        struct vlk_queue_family_indices {
            std::optional<uint32_t> graphics_family;
            std::optional<uint32_t> present_family;
            
            bool is_complete() {
                return graphics_family.has_value() && present_family.has_value();
            }
        };

        struct vlk_swapchain_support_details {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> present_modes;
        };

        struct vlk_swapchain_info {
            std::vector<VkImage> images;
            VkFormat image_format;
            VkExtent2D extent;
        };
    }

    class screen {
    public:
    	screen(int32_t width, int32_t height);
    	~screen();

        screen(const screen&) = delete;
        void operator=(const screen&) = delete;

    	void update();

    	bool screen_should_close() const;
    private:
        void _glfw_init(int32_t width, int32_t height);
        void _vlk_init();
        void _gui_init();
        
        void _draw_frame();
        
        static void _glfw_framebuffer_resize_callback(GLFWwindow* window, int32_t width, int32_t height);
        
        void _vlk_init_instance();
        bool _vlk_is_intance_extensions_supported(std::vector<const char*>* enabledExtensions);
        void _vlk_create_application_info(VkApplicationInfo* appInfo);
        void _vlk_create_instance_create_info(VkInstanceCreateInfo* createInfo, VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo, const VkApplicationInfo& appInfo, const std::vector<const char*>& extensions);
        
        void _vlk_init_debug_messenger();
        bool _vlk_is_validation_layers_supported();
        void _vlk_create_debug_utils_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT* createInfo);
        static VKAPI_ATTR VkBool32 VKAPI_CALL _vlk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                  void* pUserData);
        
        void _vlk_init_surface();
        
        void _vlk_init_physical_device();
        int32_t _vlk_rate_physical_device(VkPhysicalDevice device);
        vlk_queue_family_indices _vlk_find_queue_families(VkPhysicalDevice device);
        
        void _vlk_init_device();
        void _vlk_create_device_queue_create_info(VkDeviceQueueCreateInfo* createInfo, const uint32_t index);
        void _vlk_create_device_create_info(VkDeviceCreateInfo* createInfo,
                                            const std::vector<VkDeviceQueueCreateInfo>& queueCreateInfos,
                                            const VkPhysicalDeviceFeatures& deviceFeatures,
                                            const std::vector<const char*>& enabledExtensions);
        bool _vlk_is_device_suitable(VkPhysicalDevice device);
        bool _vlk_is_device_extension_supported(VkPhysicalDevice device);
        
        void _vlk_init_swapchain();
        vlk_swapchain_support_details _vlk_query_swapchain_support(VkPhysicalDevice device);
        VkSurfaceFormatKHR _vlk_choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR _vlk_choose_swap_surface_presentation_mode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D _vlk_choose_swap_surface_extent(const VkSurfaceCapabilitiesKHR& capabilities);
        void _vlk_create_swapchain_create_info(VkSwapchainCreateInfoKHR* createInfo,
                                                const vlk_swapchain_support_details& details,
                                                const uint32_t& imageCount,
                                                const VkSurfaceFormatKHR& surfaceFormat,
                                                const VkPresentModeKHR& presentMode,
                                                const VkExtent2D& extent);
        void _vlk_update_swapchain();
        void _vlk_cleanup_swapchain();
        
        void _vlk_init_image_views();
        void _vlk_create_image_view_create_info(VkImageViewCreateInfo* createInfo, const uint32_t& index);
        
        void _vlk_init_render_pass();
        void _vlk_create_render_pass_create_info(VkRenderPassCreateInfo* createInfo,
                                                 const VkAttachmentDescription& colorAttachmentDescription,
                                                 const VkSubpassDescription& subpassDescription,
                                                 const VkSubpassDependency& subpassDependency);
        
        void _vlk_init_graphics_pipeline();
        std::vector<uint8_t> _vlk_read_spirv_shader(const std::string& fileName);
        VkShaderModule _vlk_create_shader_module(const std::vector<uint8_t>& code);
        void _vlk_create_pipeline_create_info(VkGraphicsPipelineCreateInfo* createInfo,
                                              const std::vector<VkPipelineShaderStageCreateInfo>& shaderCreateInfos,
                                              const VkPipelineVertexInputStateCreateInfo& vertexInputStateCreateInfo,
                                              const VkPipelineInputAssemblyStateCreateInfo& inputAssemblyStateCreateInfo,
                                              const VkPipelineViewportStateCreateInfo& viewportStateCreateInfo,
                                              const VkPipelineRasterizationStateCreateInfo& rasterizationStateCreateInfo,
                                              const VkPipelineMultisampleStateCreateInfo& multisampleStateCreateInfo,
                                              const VkPipelineDepthStencilStateCreateInfo& depthStencilStateCreateInfo,
                                              const VkPipelineColorBlendStateCreateInfo& colorBlendStateCreateInfo,
                                              const VkPipelineDynamicStateCreateInfo& dynamicStates);
        void _vlk_create_shader_module_create_info(VkShaderModuleCreateInfo* createInfo, const std::vector<uint8_t>& code);
        void _vlk_create_shader_stage_create_info(VkPipelineShaderStageCreateInfo* createInfo,
                                                           const VkShaderStageFlagBits& shaderStageBit,
                                                           const VkShaderModule& shaderModule);
        void _vlk_create_dynamic_state_create_info(VkPipelineDynamicStateCreateInfo* createInfo,
                                                   const std::vector<VkDynamicState>& dynamicStates);
        void _vlk_create_vertex_input_create_info(VkPipelineVertexInputStateCreateInfo* createInfo);
        void _vlk_create_input_assembly_state_create_info(VkPipelineInputAssemblyStateCreateInfo* createInfo);
        void _vlk_create_viewport_state_create_info(VkPipelineViewportStateCreateInfo* createInfo);
        void _vlk_create_rasterization_state_create_info(VkPipelineRasterizationStateCreateInfo* createInfo);
        void _vlk_create_multisample_state_create_info(VkPipelineMultisampleStateCreateInfo* createInfo);
        void _vlk_create_depth_stencil_state_create_info(VkPipelineDepthStencilStateCreateInfo* createInfo);
        void _vlk_create_color_blend_attachment_state(VkPipelineColorBlendAttachmentState* createInfo);
        void _vlk_create_color_blend_state_create_info(VkPipelineColorBlendStateCreateInfo* createInfo,
                                                       const VkPipelineColorBlendAttachmentState& colorBlendAttachmentState);
        void _vlk_create_pipeline_layout_create_info(VkPipelineLayoutCreateInfo* createInfo);
        
        void _vlk_init_framebuffers();
        void _vlk_create_framebuffer_create_info(VkFramebufferCreateInfo* createInfo, const std::vector<VkImageView>& attachments);
        
        void _vlk_init_command_pool();
        void _vlk_create_command_pool_create_info(VkCommandPoolCreateInfo* createInfo, const vlk_queue_family_indices& indices);
        
        void _vlk_init_command_buffers();
        void _vlk_record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        
        void _vlk_init_sync_objects();
        void _vlk_init_semaphores();
        void _vlk_init_fences();
        void _vlk_reset_semaphores();
        
        void _vlk_init_vertex_buffer();
        void _vlk_init_index_buffer();
        
        const std::string _LOGGER_TAG = "GEN_SCREEN";
        logger _logger;

        const std::vector<const char*> _VALIDATION_LAYERS = {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> _REQUIRED_DEVICE_EXTENSIONS = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    #ifdef __APPLE__
        const std::vector<const char*> _PLATFORM_REQUIRED_INSTANCE_EXTENSIONS = {
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        };
        const std::vector<const char*> _PLATFORM_REQUIRED_DEVICE_EXTENSIONS = {
            "VK_KHR_portability_subset"
        };
    #else
        const std::vector<const char*> _PLATFORM_REQUIRED_INSTANCE_EXTENSIONS = {
        };
        const std::vector<const char*> _PLATFORM_REQUIRED_DEVICE_EXTENSIONS = {  
        };
    #endif
        GLFWwindow* _glfw_window;
        VkInstance _vlk_instance;
        
        VkPhysicalDevice _vlk_physical_device;
        VkDevice _vlk_device;
        VkQueue _vlk_graphics_queue;
        VkQueue _vlk_present_queue;
        
        VkSurfaceKHR _vlk_surface;
        VkSwapchainKHR _vlk_swapchain;
        vlk_swapchain_info _vlk_swapchain_info;
        std::vector<VkImageView> _vlk_image_views;
        
        VkRenderPass _vlk_render_pass;
        VkPipelineLayout _vlk_pipeline_layout;
        VkPipeline _vlk_pipeline;
        
        const uint32_t _MAX_FRAMES_IN_FLIGHT = 2;
        uint32_t _vlk_current_frame_resource_index;
        
        std::vector<VkFramebuffer> _vlk_framebuffers;
        VkCommandPool _vlk_command_pool;
        std::vector<VkCommandBuffer> _vlk_command_buffers;
        
        std::vector<VkSemaphore> _vlk_image_available_semaphores;
        std::vector<VkSemaphore> _vlk_render_finished_semaphores;
        std::vector<VkFence> _vlk_frame_in_flight_fences;
        
        bool _vlk_is_resized_framebuffer;
        
        VkBuffer _vlk_vertex_buffer;
        VkDeviceMemory _vlk_vertex_buffer_memory;
        VkBuffer _vlk_index_buffer;
        VkDeviceMemory _vlk_index_buffer_memory;
        
        VkDebugUtilsMessengerEXT _vlk_debug_messenger;
    };
}

#endif