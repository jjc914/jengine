#ifndef engine_drivers_vulkan_CONVERT_VULKAN_HPP
#define engine_drivers_vulkan_CONVERT_VULKAN_HPP

#include "engine/core/graphics/vertex_types.hpp"
#include "engine/core/graphics/image_types.hpp"
#include "engine/core/graphics/descriptor_types.hpp"
#include "engine/core/graphics/texture.hpp"
#include "engine/core/graphics/pipeline.hpp"

#include <wk/wulkan.hpp>

namespace engine::drivers::vulkan {

inline VkFormat ToVkFormat(core::graphics::ImageFormat fmt) {
    using IF = core::graphics::ImageFormat;
    switch (fmt) {
        case IF::R8_UNORM:           return VK_FORMAT_R8_UNORM;
        case IF::RG8_UNORM:          return VK_FORMAT_R8G8_UNORM;
        case IF::RGB8_UNORM:         return VK_FORMAT_R8G8B8_UNORM;
        case IF::RGBA8_UNORM:        return VK_FORMAT_R8G8B8A8_UNORM;
        case IF::BGRA8_UNORM:        return VK_FORMAT_B8G8R8A8_UNORM;
        case IF::SRGB8:              return VK_FORMAT_R8G8B8_SRGB;
        case IF::SRGBA8:             return VK_FORMAT_R8G8B8A8_SRGB;
        case IF::SBGRA8:             return VK_FORMAT_B8G8R8A8_SRGB;

        case IF::RGBA16_FLOAT:       return VK_FORMAT_R16G16B16A16_SFLOAT;
        case IF::RGBA32_FLOAT:       return VK_FORMAT_R32G32B32A32_SFLOAT;

        case IF::R8_UINT:            return VK_FORMAT_R8_UINT;
        case IF::R16_UINT:           return VK_FORMAT_R16_UINT;
        case IF::R32_UINT:           return VK_FORMAT_R32_UINT;
        case IF::RG8_UINT:           return VK_FORMAT_R8G8_UINT;
        case IF::RG16_UINT:          return VK_FORMAT_R16G16_UINT;
        case IF::RG32_UINT:          return VK_FORMAT_R32G32_UINT;
        case IF::RGBA8_UINT:         return VK_FORMAT_R8G8B8A8_UINT;
        case IF::RGBA16_UINT:        return VK_FORMAT_R16G16B16A16_UINT;
        case IF::RGBA32_UINT:        return VK_FORMAT_R32G32B32A32_UINT;

        case IF::D16_UNORM:          return VK_FORMAT_D16_UNORM;
        case IF::D24_UNORM_S8_UINT:  return VK_FORMAT_D24_UNORM_S8_UINT;
        case IF::D32_FLOAT:          return VK_FORMAT_D32_SFLOAT;
        case IF::D32_FLOAT_S8_UINT:  return VK_FORMAT_D32_SFLOAT_S8_UINT;

        default:
            ENGINE_ASSERT(false, "Unrecognized ImageFormat enum in ToVkFormat()");
            return VK_FORMAT_UNDEFINED;
    }
}

inline core::graphics::ImageFormat FromImageVkFormat(VkFormat fmt) {
    using IF = core::graphics::ImageFormat;
    switch (fmt) {
        case VK_FORMAT_R8_UNORM:             return IF::R8_UNORM;
        case VK_FORMAT_R8G8_UNORM:           return IF::RG8_UNORM;
        case VK_FORMAT_R8G8B8_UNORM:         return IF::RGB8_UNORM;
        case VK_FORMAT_R8G8B8A8_UNORM:       return IF::RGBA8_UNORM;
        case VK_FORMAT_B8G8R8A8_UNORM:       return IF::BGRA8_UNORM;
        case VK_FORMAT_R8G8B8_SRGB:          return IF::SRGB8;
        case VK_FORMAT_R8G8B8A8_SRGB:        return IF::SRGBA8;
        case VK_FORMAT_B8G8R8A8_SRGB:        return IF::SBGRA8;

        case VK_FORMAT_R16G16B16A16_SFLOAT:  return IF::RGBA16_FLOAT;
        case VK_FORMAT_R32G32B32A32_SFLOAT:  return IF::RGBA32_FLOAT;

        case VK_FORMAT_R8_UINT:              return IF::R8_UINT;
        case VK_FORMAT_R16_UINT:             return IF::R16_UINT;
        case VK_FORMAT_R32_UINT:             return IF::R32_UINT;
        case VK_FORMAT_R8G8_UINT:            return IF::RG8_UINT;
        case VK_FORMAT_R16G16_UINT:          return IF::RG16_UINT;
        case VK_FORMAT_R32G32_UINT:          return IF::RG32_UINT;
        case VK_FORMAT_R8G8B8A8_UINT:        return IF::RGBA8_UINT;
        case VK_FORMAT_R16G16B16A16_UINT:    return IF::RGBA16_UINT;
        case VK_FORMAT_R32G32B32A32_UINT:    return IF::RGBA32_UINT;

        case VK_FORMAT_D16_UNORM:            return IF::D16_UNORM;
        case VK_FORMAT_D24_UNORM_S8_UINT:    return IF::D24_UNORM_S8_UINT;
        case VK_FORMAT_D32_SFLOAT:           return IF::D32_FLOAT;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:   return IF::D32_FLOAT_S8_UINT;

        default:
            ENGINE_ASSERT(false, "Unrecognized VkFormat enum in FromImageVkFormat()");
            return IF::UNDEFINED;
    }
}

inline VkFormat ToVkFormat(core::graphics::VertexFormat fmt) {
    using VF = core::graphics::VertexFormat;
    switch (fmt) {
        case VF::FLOAT_1:  return VK_FORMAT_R32_SFLOAT;
        case VF::FLOAT_2:  return VK_FORMAT_R32G32_SFLOAT;
        case VF::FLOAT_3:  return VK_FORMAT_R32G32B32_SFLOAT;
        case VF::FLOAT_4:  return VK_FORMAT_R32G32B32A32_SFLOAT;

        case VF::INT_1:    return VK_FORMAT_R32_SINT;
        case VF::INT_2:    return VK_FORMAT_R32G32_SINT;
        case VF::INT_3:    return VK_FORMAT_R32G32B32_SINT;
        case VF::INT_4:    return VK_FORMAT_R32G32B32A32_SINT;

        case VF::UINT_1:   return VK_FORMAT_R32_UINT;
        case VF::UINT_2:   return VK_FORMAT_R32G32_UINT;
        case VF::UINT_3:   return VK_FORMAT_R32G32B32_UINT;
        case VF::UINT_4:   return VK_FORMAT_R32G32B32A32_UINT;

        case VF::HALF_1:   return VK_FORMAT_R16_SFLOAT;
        case VF::HALF_2:   return VK_FORMAT_R16G16_SFLOAT;
        case VF::HALF_3:   return VK_FORMAT_R16G16B16_SFLOAT;
        case VF::HALF_4:   return VK_FORMAT_R16G16B16A16_SFLOAT;

        case VF::UNORM8_4: return VK_FORMAT_R8G8B8A8_UNORM;

        default:
            ENGINE_ASSERT(false, "Unrecognized VertexFormat enum in ToVkFormat()");
            return VK_FORMAT_UNDEFINED;
    }
}

inline VkImageLayout ToVkSubpassLayout(core::graphics::TextureUsage usage) {
    using IU = core::graphics::TextureUsage;
    const uint32_t u = static_cast<uint32_t>(usage);

    const bool is_present  = u & static_cast<uint32_t>(IU::PRESENT_SRC);
    const bool is_depth    = u & static_cast<uint32_t>(IU::DEPTH_ATTACHMENT);
    const bool is_color    = u & static_cast<uint32_t>(IU::COLOR_ATTACHMENT);
    const bool is_sampled  = u & static_cast<uint32_t>(IU::SAMPLED_IMAGE);
    const bool is_storage  = u & static_cast<uint32_t>(IU::STORAGE_IMAGE);
    const bool is_copy_src = u & static_cast<uint32_t>(IU::COPY_SRC);
    const bool is_copy_dst = u & static_cast<uint32_t>(IU::COPY_DST);

    if (is_depth)   return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    if (is_present) return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if (is_color)   return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if (is_sampled) return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (is_storage) return VK_IMAGE_LAYOUT_GENERAL;
    if (is_copy_src) return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    if (is_copy_dst) return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    ENGINE_ASSERT(false, "Unsupported ImageUsage for subpass layout");
    return VK_IMAGE_LAYOUT_GENERAL;
}

inline VkImageLayout ToVkFinalLayout(core::graphics::TextureUsage usage) {
    using IU = core::graphics::TextureUsage;
    const uint32_t u = static_cast<uint32_t>(usage);

    const bool is_present  = u & static_cast<uint32_t>(IU::PRESENT_SRC);
    const bool is_depth    = u & static_cast<uint32_t>(IU::DEPTH_ATTACHMENT);
    const bool is_color    = u & static_cast<uint32_t>(IU::COLOR_ATTACHMENT);
    const bool is_sampled  = u & static_cast<uint32_t>(IU::SAMPLED_IMAGE);
    const bool is_copy_src = u & static_cast<uint32_t>(IU::COPY_SRC);
    const bool is_copy_dst = u & static_cast<uint32_t>(IU::COPY_DST);

    if (is_present)  return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    if (is_sampled)  return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (is_depth)    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    if (is_color)    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    if (is_copy_src) return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    if (is_copy_dst) return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    return VK_IMAGE_LAYOUT_GENERAL;
}

inline VkColorSpaceKHR ToVkColorSpace(core::graphics::ColorSpace cs) {
    using CS = core::graphics::ColorSpace;
    switch (cs) {
        case CS::SRGB_NONLINEAR: return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        case CS::LINEAR:         return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        case CS::HDR10_ST2084:   return VK_COLOR_SPACE_HDR10_ST2084_EXT;
        case CS::HDR10_HLG:      return VK_COLOR_SPACE_HDR10_HLG_EXT;
        case CS::DISPLAY_P3:     return VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT;
        default:
            ENGINE_ASSERT(false, "Unrecognized ColorSpace enum in ToVkColorSpace()");
            return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }
}

inline core::graphics::ColorSpace FromVkColorSpace(VkColorSpaceKHR cs) {
    using CS = core::graphics::ColorSpace;
    switch (cs) {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:        return CS::SRGB_NONLINEAR;
        case VK_COLOR_SPACE_HDR10_ST2084_EXT:          return CS::HDR10_ST2084;
        case VK_COLOR_SPACE_HDR10_HLG_EXT:             return CS::HDR10_HLG;
        case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:  return CS::DISPLAY_P3;
        default:
            ENGINE_ASSERT(false, "Unrecognized VkColorSpaceKHR enum in FromVkColorSpace()");
            return CS::UNKNOWN;
    }
}

inline VkDescriptorType ToVkDescriptorType(core::graphics::DescriptorType type) {
    using core::graphics::DescriptorType;
    switch (type) {
        case DescriptorType::SAMPLER:                 return VK_DESCRIPTOR_TYPE_SAMPLER;
        case DescriptorType::COMBINED_IMAGE_SAMPLER:  return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case DescriptorType::SAMPLED_IMAGE:           return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case DescriptorType::STORAGE_IMAGE:           return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DescriptorType::UNIFORM_TEXEL_BUFFER:    return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case DescriptorType::STORAGE_TEXEL_BUFFER:    return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case DescriptorType::UNIFORM_BUFFER:          return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType::STORAGE_BUFFER:          return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescriptorType::INPUT_ATTACHMENT:        return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        default:
            ENGINE_ASSERT(false, "Unrecognized DescriptorType enum in ToVkDescriptorType()");
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

inline VkShaderStageFlags ToVkShaderStageFlags(core::graphics::ShaderStageFlags stage) {
    using core::graphics::ShaderStageFlags;
    VkShaderStageFlags flags = 0;
    const uint32_t s = static_cast<uint32_t>(stage);

    if (s & static_cast<uint32_t>(ShaderStageFlags::VERTEX))
        flags |= VK_SHADER_STAGE_VERTEX_BIT;
    if (s & static_cast<uint32_t>(ShaderStageFlags::FRAGMENT))
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (s & static_cast<uint32_t>(ShaderStageFlags::COMPUTE))
        flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    if (s & static_cast<uint32_t>(ShaderStageFlags::GEOMETRY))
        flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
    if (s & static_cast<uint32_t>(ShaderStageFlags::TESSELLATION_CONTROL))
        flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if (s & static_cast<uint32_t>(ShaderStageFlags::TESSELLATION_EVALUATION))
        flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if (s & static_cast<uint32_t>(ShaderStageFlags::RAYGEN))
        flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    if (s & static_cast<uint32_t>(ShaderStageFlags::MISS))
        flags |= VK_SHADER_STAGE_MISS_BIT_KHR;
    if (s & static_cast<uint32_t>(ShaderStageFlags::CLOSEST_HIT))
        flags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    if (s & static_cast<uint32_t>(ShaderStageFlags::ANY_HIT))
        flags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    if (s & static_cast<uint32_t>(ShaderStageFlags::INTERSECTION))
        flags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;

    return flags;
}

inline uint32_t BytesPerPixel(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R32_UINT:       return 4;
        case VK_FORMAT_R8G8B8A8_UNORM: return 4;
        case VK_FORMAT_B8G8R8A8_UNORM: return 4;
        default:
            ENGINE_ASSERT(false, "Unsupported color format for readback");
            return 4;
    }
}

inline VkCullModeFlags ToVkCullMode(core::graphics::CullMode mode) {
    switch (mode) {
        case core::graphics::CullMode::NONE:           return VK_CULL_MODE_NONE;
        case core::graphics::CullMode::FRONT:          return VK_CULL_MODE_FRONT_BIT;
        case core::graphics::CullMode::BACK:           return VK_CULL_MODE_BACK_BIT;
        case core::graphics::CullMode::FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
        default:
            ENGINE_ASSERT(false, "Unsupported CullMode");
            return VK_CULL_MODE_NONE;
    }
}

inline VkPolygonMode ToVkPolygonMode(core::graphics::PolygonMode mode) {
    switch (mode) {
        case core::graphics::PolygonMode::FILL:  return VK_POLYGON_MODE_FILL;
        case core::graphics::PolygonMode::LINE:  return VK_POLYGON_MODE_LINE;
        case core::graphics::PolygonMode::POINT: return VK_POLYGON_MODE_POINT;
        default:
            ENGINE_ASSERT(false, "Unsupported PolygonMode");
            return VK_POLYGON_MODE_FILL;
    }
}

inline VkImageUsageFlags ToVkUsage(core::graphics::TextureUsage usage) {
    using IU = core::graphics::TextureUsage;
    VkImageUsageFlags flags = 0;
    const uint32_t u = static_cast<uint32_t>(usage);

    if (u & static_cast<uint32_t>(IU::COLOR_ATTACHMENT))
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (u & static_cast<uint32_t>(IU::DEPTH_ATTACHMENT))
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (u & static_cast<uint32_t>(IU::SAMPLED_IMAGE))
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (u & static_cast<uint32_t>(IU::STORAGE_IMAGE))
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (u & static_cast<uint32_t>(IU::COPY_SRC))
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (u & static_cast<uint32_t>(IU::COPY_DST))
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (u & static_cast<uint32_t>(IU::PRESENT_SRC))
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (flags == 0) {
        ENGINE_ASSERT(false, "No image usage flag found");
        flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    return flags;
}

inline VkImageLayout ToVkLayout(core::graphics::TextureLayout layout) {
    using TL = core::graphics::TextureLayout;
    switch (layout) {
        case TL::UNDEFINED:     return VK_IMAGE_LAYOUT_UNDEFINED;
        case TL::GENERAL:       return VK_IMAGE_LAYOUT_GENERAL;
        case TL::COLOR:         return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case TL::DEPTH:         return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case TL::SAMPLE:        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case TL::TRANSFER_SRC:  return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case TL::TRANSFER_DST:  return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case TL::PRESENT:       return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        default:                return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

}

#endif