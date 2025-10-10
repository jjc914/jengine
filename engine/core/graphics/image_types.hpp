#ifndef engine_core_graphics_TYPES_HPP
#define engine_core_graphics_TYPES_HPP

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

enum class ImageUsage {
    UNDEFINED = 0,
    COLOR,
    DEPTH,
    PRESENT,
    STORAGE,
};

struct ImageAttachmentInfo {
    ImageFormat format;
    ImageUsage usage;

    ImageAttachmentInfo& set_format(ImageFormat f) { format = f; return *this; }
    ImageAttachmentInfo& set_usage(ImageUsage u) { usage = u; return *this; }
};

};

#endif