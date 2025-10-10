#ifndef engine_core_graphics_DESCRIPTOR_TYPES_HPP
#define engine_core_graphics_DESCRIPTOR_TYPES_HPP

#include <cstdint>
#include <vector>

namespace engine::core::graphics {

enum class ShaderStageFlags : uint32_t {
    NONE        = 0,
    VERTEX      = 1 << 0,
    FRAGMENT    = 1 << 1,
    COMPUTE     = 1 << 2,
    GEOMETRY    = 1 << 3,
    TESSELLATION_CONTROL    = 1 << 4,
    TESSELLATION_EVALUATION = 1 << 5,
    RAYGEN      = 1 << 6,
    MISS        = 1 << 7,
    CLOSEST_HIT = 1 << 8,
    ANY_HIT     = 1 << 9,
    INTERSECTION = 1 << 10,

    ALL_GRAPHICS = (VERTEX | FRAGMENT | GEOMETRY |
                   TESSELLATION_CONTROL | TESSELLATION_EVALUATION),
    ALL         = 0xFFFFFFFF
};

constexpr inline ShaderStageFlags operator|(ShaderStageFlags a, ShaderStageFlags b) noexcept {
    return static_cast<ShaderStageFlags>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
    );
}

constexpr inline ShaderStageFlags operator&(ShaderStageFlags a, ShaderStageFlags b) noexcept {
    return static_cast<ShaderStageFlags>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
    );
}

constexpr inline ShaderStageFlags& operator|=(ShaderStageFlags& a, ShaderStageFlags b) noexcept {
    a = a | b;
    return a;
}

constexpr inline ShaderStageFlags& operator&=(ShaderStageFlags& a, ShaderStageFlags b) noexcept {
    a = a & b;
    return a;
}

constexpr inline bool operator!(ShaderStageFlags a) noexcept {
    return static_cast<uint32_t>(a) == 0;
}

enum class DescriptorType : uint32_t {
    UNDEFINED = 0,
    SAMPLER,
    COMBINED_IMAGE_SAMPLER,
    SAMPLED_IMAGE,
    STORAGE_IMAGE,
    UNIFORM_TEXEL_BUFFER,
    STORAGE_TEXEL_BUFFER,
    UNIFORM_BUFFER,
    STORAGE_BUFFER,
    INPUT_ATTACHMENT
};

struct DescriptorLayoutBinding {
    uint32_t binding;
    DescriptorType type;
    uint32_t count = 1;
    ShaderStageFlags visibility = ShaderStageFlags::ALL;

    DescriptorLayoutBinding& set_binding(uint32_t b) { binding = b; return *this; }
    DescriptorLayoutBinding& set_type(DescriptorType t) { type = t; return *this; }
    DescriptorLayoutBinding& set_count(uint32_t c) { count = c; return *this; }
    DescriptorLayoutBinding& set_visibility(ShaderStageFlags v) { visibility = v; return *this; }
};

struct DescriptorLayoutDescription {
    std::vector<DescriptorLayoutBinding> bindings;

    DescriptorLayoutDescription& add_binding(uint32_t binding, DescriptorType type, uint32_t count = 1, ShaderStageFlags visibility = ShaderStageFlags::ALL) {
        bindings.emplace_back(binding, type, count, visibility);
        return *this;
    }

    DescriptorLayoutDescription& add_binding(DescriptorLayoutBinding b) {
        bindings.emplace_back(b);
        return *this;
    }
};

}

#endif