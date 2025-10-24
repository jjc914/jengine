#include "vulkan_device.hpp"

#include "vulkan_instance.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_shader.hpp"
#include "vulkan_texture.hpp"
#include "vulkan_mesh_buffer.hpp"
#include "vulkan_swapchain_render_target.hpp"
#include "vulkan_texture_render_target.hpp"
#include "vulkan_material.hpp"
#include "vulkan_descriptor_set_layout.hpp"
#include "convert_vulkan.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

#include "engine/core/window/window.hpp"
#include "engine/core/graphics/image_types.hpp"

namespace engine::drivers::vulkan {

VulkanDevice::VulkanDevice(const VulkanInstance& instance, const core::window::Window& window)
    : _instance(instance.instance())
{
    // physical device
    VkPhysicalDeviceFeatures2 physical_device_features = wk::PhysicalDeviceFeatures2{}.to_vk();
    _physical_device = wk::PhysicalDevice(static_cast<VkInstance>(instance.native_instance()),
        wk::GetRequiredDeviceExtensions(), &physical_device_features,
        &wk::DefaultPhysicalDeviceFeatureScorer
    );
    wk::ext::glfw::Surface surface(static_cast<VkInstance>(instance.native_instance()), static_cast<GLFWwindow*>(window.native_window()));
    
    _queue_families = wk::FindQueueFamilies(_physical_device.handle());

    ENGINE_ASSERT(_queue_families.graphics_family.has_value(), "Selected physical device has no graphics queue family");

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device.handle(), &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device.handle(), &queue_family_count, queue_families.data());

    // get surface information (assume all are compatible)
    wk::PhysicalDeviceSurfaceSupport support = wk::GetPhysicalDeviceSurfaceSupport(
        _physical_device.handle(), surface.handle()
    );
    VkSurfaceFormatKHR surface_format = wk::ChooseSurfaceFormat(support.formats);
    _present_format = vulkan::FromImageVkFormat(surface_format.format);
    _present_color_space = vulkan::FromVkColorSpace(surface_format.colorSpace);
    _depth_format = vulkan::FromImageVkFormat(wk::ChooseDepthFormat(_physical_device.handle()));

    for (uint32_t i = 0; i < queue_family_count; ++i) {
        VkBool32 supports_present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device.handle(), i, surface.handle(), &supports_present);
        if (supports_present) {
            _present_family = i;
            break;
        }
    }

    // cache supported formats
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device.handle(), surface.handle(), &format_count, nullptr);
    if (format_count == 0) {
        core::debug::Logger::get_singleton().fatal("No surface formats available to this physical device");
    }

    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device.handle(), surface.handle(), &format_count, formats.data());

    if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        _present_format = FromImageVkFormat(VK_FORMAT_R8G8B8A8_UNORM);
        _present_color_space = core::graphics::ColorSpace::SRGB_NONLINEAR;
    }
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_R8G8B8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            _present_format = FromImageVkFormat(VK_FORMAT_R8G8B8A8_UNORM);
            _present_color_space = core::graphics::ColorSpace::SRGB_NONLINEAR;
            break;
        }
    }

    std::vector<VkFormat> depth_candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    for (const VkFormat& format : depth_candidates) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(_physical_device.handle(), format, &properties);

        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            _depth_format = FromImageVkFormat(format);
            break;
        }
    }

    if (_depth_format == engine::core::graphics::ImageFormat::UNDEFINED) {
        core::debug::Logger::get_singleton().fatal("Failed to find supported depth format for this physical device");
    }

    // device
    const float QUEUE_PRIORITY = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos = {
        wk::DeviceQueueCreateInfo{}
            .set_queue_family_index(_queue_families.graphics_family.value())
            .set_queue_count(1)
            .set_p_queue_priorities(&QUEUE_PRIORITY)
            .to_vk()
    };
    
    if (_present_family != _queue_families.graphics_family.value()) {
        queue_create_infos.push_back(
            wk::DeviceQueueCreateInfo{}
                .set_queue_family_index(_present_family)
                .set_queue_count(1)
                .set_p_queue_priorities(&QUEUE_PRIORITY)
                .to_vk());
    }

    _device = wk::Device(_physical_device.handle(), _queue_families,
        wk::DeviceCreateInfo{}
            .set_p_enabled_features(&_physical_device.features())
            .set_enabled_extensions(_physical_device.extensions().size(),
                                    _physical_device.extensions().data())
            .set_queue_create_infos(queue_create_infos.size(), queue_create_infos.data())
            .to_vk());

    _graphics_queue = wk::Queue(_device.handle(), _queue_families.graphics_family.value());

    // command pool
    _command_pool = wk::CommandPool(_device.handle(),
        wk::CommandPoolCreateInfo{}
            .set_flags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                    VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)
            .set_queue_family_index(_queue_families.graphics_family.value())
            .to_vk()
    );

    // descriptor pool
    VkDescriptorPoolSize pool_sizes[] = {
        wk::DescriptorPoolSize{}
            .set_type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            .set_descriptor_count(1024)
            .to_vk(),
        wk::DescriptorPoolSize{}
            .set_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            .set_descriptor_count(1024)
            .to_vk(),
        wk::DescriptorPoolSize{}
            .set_type(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            .set_descriptor_count(256)
            .to_vk()
    };

    _descriptor_pool = wk::DescriptorPool(_device.handle(),
        wk::DescriptorPoolCreateInfo{}
            .set_max_sets(1024)
            .set_flags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .set_pool_sizes(static_cast<uint32_t>(std::size(pool_sizes)), pool_sizes)
            .to_vk()
    );

    // allocator
    VmaVulkanFunctions vulkan_functions{};
    _allocator = wk::Allocator(
        wk::AllocatorCreateInfo{}
            .set_vulkan_api_version(VK_API_VERSION_1_3)
            .set_instance(static_cast<VkInstance>(instance.native_instance()))
            .set_physical_device(_physical_device.handle())
            .set_device(_device.handle())
            .set_p_vulkan_functions(&vulkan_functions)
            .to_vk()
    );
}

void VulkanDevice::wait_idle() {
    vkDeviceWaitIdle(_device.handle());
}

std::unique_ptr<core::graphics::Shader> VulkanDevice::create_shader(core::graphics::ShaderStageFlags stage, const std::string& filepath) const {
    return std::make_unique<VulkanShader>(
        *this,
        stage, filepath
    );
}

std::unique_ptr<core::graphics::MeshBuffer> VulkanDevice::create_mesh_buffer(
    const void* vertex_data, uint32_t vertex_size, uint32_t vertex_count,
    const void* index_data, uint32_t index_size, uint32_t index_count) const 
{
    ENGINE_ASSERT(vertex_data != nullptr, "Vertex buffer creation requires valid data pointer");
    ENGINE_ASSERT(vertex_size > 0 && vertex_count > 0, "Vertex buffer size/count must be greater than zero");
    return std::make_unique<VulkanMeshBuffer>(
        *this,
        vertex_data, vertex_size, vertex_count,
        index_data, index_size, index_count
    );
}

std::unique_ptr<core::graphics::Texture> VulkanDevice::create_texture(
    uint32_t width,
    uint32_t height,
    core::graphics::ImageFormat format,
    uint32_t layers,
    uint32_t mip_levels,
    core::graphics::TextureUsage usage) const
{
    ENGINE_ASSERT(width != 0 && height != 0, "Texture requires a non-zero width and height");
    return std::make_unique<VulkanTexture>(
        *this,
        width, height,
        format,
        layers, mip_levels,
        usage
    );
}

std::unique_ptr<core::graphics::Texture> VulkanDevice::create_texture_from_native(
    void* image,
    void* view,
    uint32_t width,
    uint32_t height,
    core::graphics::ImageFormat format,
    uint32_t layers,
    uint32_t mip_levels,
    core::graphics::TextureUsage usage
) const {
    ENGINE_ASSERT(width != 0 && height != 0, "Texture requires a non-zero width and height");
    return std::make_unique<VulkanTexture>(
        *this,
        static_cast<VkImage>(image),
        static_cast<VkImageView>(view),
        width, height,
        format,
        layers,
        mip_levels,
        usage
    );
}

std::unique_ptr<core::graphics::Pipeline> VulkanDevice::create_pipeline(
    const core::graphics::Shader& vert, const core::graphics::Shader& frag,
    const core::graphics::DescriptorSetLayout& layout,
    const core::graphics::VertexBindingDescription& vertex_binding, 
    const std::vector<core::graphics::ImageAttachmentInfo>& attachment_info,
    const core::graphics::PipelineConfig& config
) const {
    return std::make_unique<VulkanPipeline>(
        *this,
        static_cast<VkShaderModule>(vert.native_shader()), static_cast<VkShaderModule>(frag.native_shader()),
        vertex_binding,
        layout,
        attachment_info,
        config
    );
}

std::unique_ptr<core::graphics::SwapchainRenderTarget> VulkanDevice::create_swapchain_render_target(
    const core::window::Window& window, const core::graphics::Pipeline& pipeline,
    uint32_t max_in_flight, bool has_depth
) const {
    return std::make_unique<VulkanSwapchainRenderTarget>(
        *this, window, pipeline, max_in_flight, has_depth
    );
}

std::unique_ptr<core::graphics::RenderTarget> VulkanDevice::create_texture_render_target(
    const core::graphics::Pipeline& pipeline,
    const core::graphics::AttachmentInfo& attachments,
    uint32_t max_in_flight
) const {
    return std::make_unique<VulkanTextureRenderTarget>(
        *this, pipeline, attachments, max_in_flight
    );
}

std::unique_ptr<core::graphics::DescriptorSetLayout> VulkanDevice::create_descriptor_set_layout(
    const core::graphics::DescriptorLayoutDescription& description
) const {
    return std::make_unique<VulkanDescriptorSetLayout>(
        *this, description
    );
}

}