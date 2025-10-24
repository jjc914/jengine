#ifndef engine_core_graphics_VERTEX_TYPES_HPP
#define engine_core_graphics_VERTEX_TYPES_HPP

#include "engine/core/debug/assert.hpp"

#include <cstdint>
#include <vector>

namespace engine::core::graphics {

enum class VertexFormat {
    UNDEFINED = 0,

    FLOAT_1,
    FLOAT_2,
    FLOAT_3,
    FLOAT_4,

    INT_1,
    INT_2,
    INT_3,
    INT_4,

    UINT_1,
    UINT_2,
    UINT_3,
    UINT_4,

    HALF_1,
    HALF_2,
    HALF_3,
    HALF_4,

    UNORM8_4
};


struct VertexAttribute {
    uint32_t location = 0;
    VertexFormat format = VertexFormat::UNDEFINED;
    uint32_t offset = 0;

    VertexAttribute& set_location(uint32_t l) {
        ENGINE_ASSERT(l < 32, "Vertex attribute location index unusually large (expected <32)");
        location = l;
        return *this;
    }

    VertexAttribute& set_format(VertexFormat f) {
        ENGINE_ASSERT(f != VertexFormat::UNDEFINED, "Vertex attribute format must be defined (cannot be UNDEFINED)");
        format = f;
        return *this;
    }

    VertexAttribute& set_offset(uint32_t o) {
        ENGINE_ASSERT(o < 4096, "Vertex attribute offset unusually large (expected <4096 bytes)");
        offset = o;
        return *this;
    }
};

struct VertexBindingDescription {
    uint32_t binding = 0;
    uint32_t stride = 0;
    std::vector<VertexAttribute> attributes;

    VertexBindingDescription& set_binding(uint32_t b) {
        ENGINE_ASSERT(b < 16, "Vertex binding index unusually large (expected <16)");
        binding = b;
        return *this;
    }

    VertexBindingDescription& set_stride(uint32_t s) {
        ENGINE_ASSERT(s > 0, "Vertex stride must be greater than 0");
        ENGINE_ASSERT(s < 4096, "Vertex stride unusually large (expected <4096 bytes)");
        stride = s;
        return *this;
    }

    VertexBindingDescription& add_attribute(uint32_t l, VertexFormat f, uint32_t o) {
        for (const VertexAttribute& a : attributes)
            ENGINE_ASSERT(a.location != l, "Duplicate vertex attribute location in binding");
        attributes.emplace_back(
            VertexAttribute{}
                .set_location(l)
                .set_format(f)
                .set_offset(o)
        );
        return *this;
    }

    VertexBindingDescription& set_attributes(const std::vector<VertexAttribute>& a) {
        for (const VertexAttribute& ai : a)
            ENGINE_ASSERT(ai.format != VertexFormat::UNDEFINED, "Vertex attribute has undefined format");
        for (size_t i = 0; i < a.size(); ++i) {
            for (size_t j = i + 1; j < a.size(); ++j)
                ENGINE_ASSERT(a[i].location != a[j].location, "Duplicate vertex attribute location detected");
        }
        attributes = a;
        return *this;
    }
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_VERTEX_TYPES_HPP