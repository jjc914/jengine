#ifndef engine_core_graphics_VERTEX_TYPES_HPP
#define engine_core_graphics_VERTEX_TYPES_HPP

#include <cstdint>
#include <vector>

namespace engine::core::graphics {

enum class VertexFormat {
    UNDEFINED = 0,

    FLOAT1,
    FLOAT2,
    FLOAT3,
    FLOAT4,

    INT1,
    INT2,
    INT3,
    INT4,

    UINT1,
    UINT2,
    UINT3,
    UINT4,

    HALF1,
    HALF2,
    HALF3,
    HALF4
};

struct VertexAttribute {
    uint32_t location;
    VertexFormat format;
    uint32_t offset;

    VertexAttribute& set_location(uint32_t l) { location = l; return *this; }
    VertexAttribute& set_format(VertexFormat f) { format = f; return *this; }
    VertexAttribute& set_offset(uint32_t o ) { offset = o; return *this; }
};

struct VertexBinding {
    uint32_t binding;
    uint32_t stride;
    std::vector<VertexAttribute> attributes;

    VertexBinding& set_binding(uint32_t b) { binding = b; return *this; }
    VertexBinding& set_stride(uint32_t s) { stride = s; return *this; }
    VertexBinding& add_attribute(uint32_t l, VertexFormat f, uint32_t o) { attributes.emplace_back(l, f, o); return *this; }
    VertexBinding& set_attributes(const std::vector<VertexAttribute>& a) { attributes = a; return *this; }
};

}

#endif