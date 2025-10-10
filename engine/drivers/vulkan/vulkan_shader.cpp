#include "vulkan_shader.hpp"

#include "vulkan_device.hpp"

#include "engine/core/debug/logger.hpp"

#include <fstream>
#include <vector>

namespace engine::drivers::vulkan {

VulkanShader::VulkanShader(const VulkanDevice& device, core::graphics::ShaderStageFlags stage, const std::string& filepath)
    : _stage(stage)
{
    std::vector<uint8_t> bin = wk::ReadSpirvShader(filepath.c_str());
    _module = wk::ShaderModule(device.device().handle(),
        wk::ShaderModuleCreateInfo{}
            .set_byte_code(bin.size(), reinterpret_cast<const uint32_t*>(bin.data()))
            .to_vk()
    );
}

} // namespace engine::drivers::vulkan
