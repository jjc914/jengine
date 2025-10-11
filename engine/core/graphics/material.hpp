#ifndef engine_core_graphics_MATERIAL_HPP
#define engine_core_graphics_MATERIAL_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace engine::core::graphics {

class Material {
public:
    virtual ~Material() = default;

    virtual void bind(void* cb) const = 0;

    virtual void update_uniform_buffer(const void* data) = 0;

    virtual void* native_descriptor_set() const = 0;
    virtual std::string backend_name() const = 0;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_MATERIAL_HPP
