#ifndef engine_core_graphics_TEXTURE_HPP
#define engine_core_graphics_TEXTURE_HPP

#include "image_types.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace engine::core::graphics {

enum class TextureLayout {
    UNDEFINED = 0,
    GENERAL,
    COLOR,
    DEPTH,
    SAMPLE,
    TRANSFER_SRC,
    TRANSFER_DST,
    PRESENT
};

struct TextureBarrier {
    TextureLayout old_layout;
    TextureLayout new_layout;
    TextureUsage usage_before;
    TextureUsage usage_after;
};

class Texture {
public:
    virtual ~Texture() = default;

    virtual void bind(void* command_buffer, uint32_t binding = 0) const = 0;

    virtual void copy_to_cpu(std::vector<uint8_t>& out_pixels) const = 0;

    virtual void transition(const TextureBarrier& layout) = 0;
    virtual void resize(uint32_t width, uint32_t height) = 0;

    virtual uint32_t width() const { return _width; };
    virtual uint32_t height() const {return _height; };
    virtual uint32_t layers() const = 0;
    virtual uint32_t mip_levels() const = 0;
    virtual ImageFormat format() const = 0;

    virtual TextureLayout layout() const = 0;
    virtual void set_layout(TextureLayout layout) = 0;

    virtual void* native_image() const = 0;
    virtual void* native_image_view() const = 0;

protected:
    uint32_t _width;
    uint32_t _height;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_TEXTURE_HPP
