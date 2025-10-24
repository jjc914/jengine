#ifndef engine_drivers_vulkan_VULKAN_SHADER_HPP
#define engine_drivers_vulkan_VULKAN_SHADER_HPP

#include "engine/core/graphics/shader.hpp"

#include <wk/wulkan.hpp>

namespace engine::drivers::vulkan {

class VulkanDevice;

class VulkanShader final : public core::graphics::Shader {
public:
    VulkanShader(const VulkanDevice& device, core::graphics::ShaderStageFlags stage, const std::string& filepath);

    VulkanShader(VulkanShader&& other) = default;
    VulkanShader& operator=(VulkanShader&& other) = default;

    VulkanShader(const VulkanShader&) = delete;
    VulkanShader& operator=(const VulkanShader&) = delete;

    ~VulkanShader() override = default;

    core::graphics::ShaderStageFlags stage() const override { return _stage; }

    void* native_shader() const override { return static_cast<void*>(_module.handle()); }
    std::string backend_name() const override { return "Vulkan"; }

private:
    wk::ShaderModule _module;
    core::graphics::ShaderStageFlags _stage;
};

} // namespace engine::drivers::vulkan

#endif // engine_drivers_vulkan_VULKAN_SHADER_HPP