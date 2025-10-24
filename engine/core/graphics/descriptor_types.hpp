#ifndef engine_core_graphics_DESCRIPTOR_TYPES_HPP
#define engine_core_graphics_DESCRIPTOR_TYPES_HPP

#include "engine/core/debug/assert.hpp"

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
    ALL          = 0xFFFFFFFF
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

    DescriptorLayoutBinding& set_binding(uint32_t b) { 
        ENGINE_ASSERT(b < 32, "Descriptor binding index unusually large (expected <32)");
        binding = b; 
        return *this; 
    }

    DescriptorLayoutBinding& set_type(DescriptorType t) { 
        ENGINE_ASSERT(t != DescriptorType::UNDEFINED, "Descriptor type must be defined");
        type = t; 
        return *this; 
    }

    DescriptorLayoutBinding& set_count(uint32_t c) { 
        ENGINE_ASSERT(c > 0, "Descriptor count must be greater than 0");
        count = c; 
        return *this; 
    }

    DescriptorLayoutBinding& set_visibility(ShaderStageFlags v) { 
        ENGINE_ASSERT(v != ShaderStageFlags::NONE, "Descriptor must be visible to at least one shader stage");
        visibility = v; 
        return *this; 
    }

    bool operator==(const DescriptorLayoutBinding& other) const noexcept {
        return binding == other.binding &&
            type == other.type &&
            count == other.count &&
            visibility == other.visibility;
    }
};

struct DescriptorLayoutDescription {
    std::vector<DescriptorLayoutBinding> bindings;

    DescriptorLayoutDescription& add_binding(uint32_t binding, DescriptorType type, uint32_t count = 1, ShaderStageFlags visibility = ShaderStageFlags::ALL) {
        for (const DescriptorLayoutBinding& b : bindings)
            ENGINE_ASSERT(b.binding != binding, "Duplicate descriptor binding index in layout description");

        bindings.emplace_back(
            DescriptorLayoutBinding{}
                .set_binding(binding)
                .set_type(type)
                .set_count(count)
                .set_visibility(visibility)
        );
        return *this;
    }

    DescriptorLayoutDescription& add_binding(DescriptorLayoutBinding b) {
        for (const DescriptorLayoutBinding& bi : bindings)
            ENGINE_ASSERT(bi.binding != b.binding, "Duplicate descriptor binding index in layout description");

        bindings.emplace_back(
            DescriptorLayoutBinding{}
                .set_binding(b.binding)
                .set_type(b.type)
                .set_count(b.count)
                .set_visibility(b.visibility)
        );
        return *this;
    }

    bool operator==(const DescriptorLayoutDescription& other) const noexcept {
        return bindings == other.bindings;
    }

    size_t hash() const {
        size_t h = 0;
        for (const auto& b : bindings) {
            // chat gpt generated hash
            h ^= std::hash<uint32_t>{}(b.binding) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<uint32_t>{}(static_cast<uint32_t>(b.type)) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<uint32_t>{}(b.count) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<uint32_t>{}(static_cast<uint32_t>(b.visibility)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};

}

#endif