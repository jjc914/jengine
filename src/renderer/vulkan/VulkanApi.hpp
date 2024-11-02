#ifndef VULKAN_API_HPP
#define VULKAN_API_HPP

#include "core/GraphicsApi.hpp"
#include "utils/SparseVector.hpp"

#include <vulkan/vulkan_core.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <vector>
#include <optional>

namespace {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;

        bool is_complete() const {
            return graphics_family.has_value() && present_family.has_value();
        }
    };

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    struct SwapchainResources {
        SwapchainResources() = default;

        bool is_dirty;

        size_t width;
        size_t height;

        uint32_t max_frames_in_flight = 2;

        std::vector<VkImage> images;
        VkFormat image_format;
        VkExtent2D extent;

        std::vector<size_t> image_views;
        std::vector<size_t> framebuffers;

        size_t render_pass;
        size_t command_pool;

        std::vector<size_t> image_available_semaphores;
        std::vector<size_t> render_finished_semaphores;
        std::vector<size_t> frame_in_flight_fences;
        
        uint32_t current_frame_resource_index;
    };

    struct CommandPool {
        VkCommandPool command_pool;

        std::vector<size_t> command_buffers;
    };
}

class VulkanApi : public GraphicsApi {
public:
    VulkanApi(void);

    void* create_instance(const std::vector<const char*>& required_extensions) override;
    void create_device(void* surface) override;
    void* create_swapchain(uint32_t width, uint32_t height) override;
    void* create_render_pass(RenderPassCreateInfo* create_info) override;
    void* create_shader(std::string path) override;
    void* create_pipeline(void* swapchain_i, void* render_pass_i) override;
    void* create_command_pool() override;
    std::vector<void*> create_command_buffers(void* command_pool_i, size_t count) override;
    void* create_descriptor_pool(void) override;

    void draw_frame(void* swapchain_i, void* pipeline) override;
    void update_swapchain(void* swapchain_i, uint32_t width, uint32_t height) override;

    void destroy_instance(void) override;
    void destroy_device(void) override;
    void destroy_swapchain(void* swapchain_i) override;
    void destroy_render_pass(void* render_pass_i) override;
    void destroy_shader(void* shader_i) override;
    void destroy_pipeline(void* pipeline_i) override;
    void destroy_command_pool(void* command_pool_i) override;

    void wait_device_idle(void) override;
    
    void* get_render_pass_handle(void* render_pass) override;
    void* get_instance_handle(void) override;
    void* get_physical_device_handle(void) override;
    void* get_device_handle(void) override;
    void* get_graphics_queue_family(void) override;
    void* get_graphics_queue_handle(void) override;
    void* get_descriptor_pool_handle(void* descriptor_pool) override;
    // void* get_command_pool_handle(void* command_pool) override;
    // void* get_command_buffer_handle(void* command_buffer) override;
    // void* get_render_pass_handle(void* render_pass) override;
    // void* get_framebuffer_handle(void* framebuffer) override;

private:
    bool _is_validation_layers_supported(void) const;
    bool _is_instance_extensions_supported(std::vector<const char*>* enabled_extensions, std::vector<const char*> window_required_extensions) const;
    int32_t _rate_physical_device(VkPhysicalDevice device) const;
    bool _is_device_suitable(VkPhysicalDevice device) const;
    QueueFamilyIndices _find_queue_families(VkPhysicalDevice device) const;
    bool _is_device_extension_supported(VkPhysicalDevice device) const;
    SwapchainSupportDetails _query_swapchain_support(VkPhysicalDevice device) const;
    VkSurfaceFormatKHR _choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availalbe_formats) const;
    VkPresentModeKHR _choose_swap_surface_presentation_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const;
    VkExtent2D _choose_swap_surface_extent(uint32_t width, uint32_t height, const VkSurfaceCapabilitiesKHR& capabilities) const;
    std::vector<uint8_t> _read_spirv_shader(const std::string& fileName) const;

    void _init_instance(const std::vector<const char*> required_extensions);
    void _init_debug_messenger(void);
    void _init_physical_device(void);
    void _init_device(void);

    size_t _create_swapchain(SwapchainResources& resources);
    void _create_swapchain_image_views(size_t swapchain_i);
    void _create_swapchain_framebuffers(size_t swapchain_i);
    void _create_swapchain_semaphores(size_t swapchain_i);
    void _create_swapchain_fences(size_t swapchain_i);
    void _update_swapchain(size_t swapchain_i);
    void _record_command_buffer(VkCommandBuffer& buffer, VkPipeline& pipeline, SwapchainResources& resources, uint32_t image_index);

    VkFormat _image_format_to_vk_format(RenderPassCreateInfo::ImageFormat format);
    RenderPassCreateInfo::ImageFormat _vk_format_to_image_format(VkFormat format);

    static VKAPI_ATTR VkBool32 VKAPI_CALL _debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                              VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                              void* pUserData);

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
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;

    VkSurfaceKHR _surface;
    VkPhysicalDevice _physical_device;
    VkDevice _device;

    VkQueue _graphics_queue;
    VkQueue _present_queue;
    QueueFamilyIndices _queue_family_indices;

    SparseVector<VkSwapchainKHR> _swapchains;
    SparseVector<SwapchainResources> _swapchain_resources;
    SparseVector<CommandPool> _command_pools;

    SparseVector<VkRenderPass> _render_passes;
    SparseVector<VkShaderModule> _shaders;
    SparseVector<VkPipeline> _pipelines;
    SparseVector<VkPipelineLayout> _pipeline_layouts;
    SparseVector<VkImageView> _image_views;
    SparseVector<VkFramebuffer> _framebuffers;
    SparseVector<VkCommandBuffer> _command_buffers;
    SparseVector<VkSemaphore> _semaphores;
    SparseVector<VkFence> _fences;

    SparseVector<VkDescriptorPool> _descriptor_pools;
};

#endif