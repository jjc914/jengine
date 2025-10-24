#ifndef engine_core_graphics_IMAGE_TYPES_HPP
#define engine_core_graphics_IMAGE_TYPES_HPP

#include "engine/core/debug/assert.hpp"

namespace engine::core::graphics {

enum class ImageFormat {
    UNDEFINED = 0,

    // color formats
    R8_UNORM,
    RG8_UNORM,
    RGB8_UNORM,
    RGBA8_UNORM,
    BGRA8_UNORM,
    SRGB8,
    SRGBA8,
    SBGRA8,

    // integer color formats
    R8_UINT,
    R16_UINT,
    R32_UINT,
    RG8_UINT,
    RG16_UINT,
    RG32_UINT,
    RGBA8_UINT,
    RGBA16_UINT,
    RGBA32_UINT,

    // hdr / float formats
    RGBA16_FLOAT,
    RGBA32_FLOAT,

    // depth / stencil formats
    D16_UNORM,
    D24_UNORM_S8_UINT,
    D32_FLOAT,
    D32_FLOAT_S8_UINT
};

enum class ColorSpace {
    SRGB_NONLINEAR,
    LINEAR,
    HDR10_ST2084,
    HDR10_HLG,
    DISPLAY_P3,
    UNKNOWN
};

enum class TextureUsage : uint32_t {
    UNDEFINED = 0,

    COLOR_ATTACHMENT = 1 << 0,
    DEPTH_ATTACHMENT = 1 << 1,
    PRESENT_SRC      = 1 << 2,
    STORAGE_IMAGE    = 1 << 3,
    SAMPLED_IMAGE    = 1 << 4,

    COPY_SRC = 1 << 5,
    COPY_DST = 1 << 6
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(
        static_cast<std::underlying_type_t<TextureUsage>>(a) |
        static_cast<std::underlying_type_t<TextureUsage>>(b)
    );
}
inline TextureUsage& operator|=(TextureUsage& a, TextureUsage b) { a = a | b; return a; }

inline TextureUsage operator&(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(
        static_cast<std::underlying_type_t<TextureUsage>>(a) &
        static_cast<std::underlying_type_t<TextureUsage>>(b)
    );
}

struct ImageAttachmentInfo {
    ImageFormat format;
    TextureUsage usage;

    ImageAttachmentInfo& set_format(ImageFormat f) {
        ENGINE_ASSERT(f != ImageFormat::UNDEFINED, "Image format must be defined (cannot be UNDEFINED)");
        format = f;
        return *this;
    }

    ImageAttachmentInfo& set_usage(TextureUsage u) {
        ENGINE_ASSERT(u != TextureUsage::UNDEFINED, "Image usage must be defined (cannot be UNDEFINED)");
        usage = u;
        return *this;
    }
};

static bool IsDepthFormat(graphics::ImageFormat fmt) {
    using IF = graphics::ImageFormat;
    switch (fmt) {
        case IF::D16_UNORM: case IF::D32_FLOAT:
        case IF::D24_UNORM_S8_UINT: case IF::D32_FLOAT_S8_UINT:
            return true;
        default:
            return false;
    }
}

} // namespace engine::core::graphics

#endif // engine_core_graphics_IMAGE_TYPES_HPP