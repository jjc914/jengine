#ifndef engine_core_graphics_TYPES_HPP
#define engine_core_graphics_TYPES_HPP

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

enum class ImageUsage : uint32_t {
    UNDEFINED = 0,

    COLOR     = 1 << 0,
    DEPTH     = 1 << 1,
    PRESENT   = 1 << 2,
    STORAGE   = 1 << 3,
    SAMPLING  = 1 << 4
};

inline ImageUsage operator|(ImageUsage a, ImageUsage b) {
    return static_cast<ImageUsage>(
        static_cast<std::underlying_type_t<ImageUsage>>(a) |
        static_cast<std::underlying_type_t<ImageUsage>>(b)
    );
}
inline ImageUsage& operator|=(ImageUsage& a, ImageUsage b) { a = a | b; return a; }

inline ImageUsage operator&(ImageUsage a, ImageUsage b) {
    return static_cast<ImageUsage>(
        static_cast<std::underlying_type_t<ImageUsage>>(a) &
        static_cast<std::underlying_type_t<ImageUsage>>(b)
    );
}

struct ImageAttachmentInfo {
    ImageFormat format;
    ImageUsage usage;

    ImageAttachmentInfo& set_format(ImageFormat f) {
        ENGINE_ASSERT(f != ImageFormat::UNDEFINED, "Image format must be defined (cannot be UNDEFINED)");
        format = f;
        return *this;
    }

    ImageAttachmentInfo& set_usage(ImageUsage u) {
        ENGINE_ASSERT(u != ImageUsage::UNDEFINED, "Image usage must be defined (cannot be UNDEFINED)");
        usage = u;
        return *this;
    }
};

};

#endif