#ifndef CORE_GRAPHICS_VERTEX_LAYOUT_HPP
#define CORE_GRAPHICS_VERTEX_LAYOUT_HPP

#include <cstdint>
#include <vector>

namespace core::graphics {

enum class VertexFormat {
    UNDEFINED = 0,
    R32_FLOAT,
    RG32_FLOAT,
    RGB32_FLOAT,
    RGBA32_FLOAT,

    R8_UNORM,
    RG8_UNORM,
    RGB8_UNORM,
    RGBA8_UNORM,

    R16_FLOAT,
    RG16_FLOAT,
    RGB16_FLOAT,
    RGBA16_FLOAT,

    R32_UINT,
    RG32_UINT,
    RGB32_UINT,
    RGBA32_UINT,
};

struct VertexAttribute {
    uint32_t location;
    uint32_t binding;
    VertexFormat format;
    uint32_t offset;
};

struct VertexBinding {
    uint32_t binding;
    uint32_t stride;
    enum class InputRate { VERTEX, INSTANCE } input_rate = InputRate::VERTEX;
};

struct VertexLayout {
    std::vector<VertexBinding> bindings;
    std::vector<VertexAttribute> attributes;
};

} // namespace core::graphics

#endif // CORE_GRAPHICS_VERTEX_LAYOUT_HPP
