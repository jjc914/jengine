#ifndef CORE_GRAPHICS_MATERIAL_HPP
#define CORE_GRAPHICS_MATERIAL_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace core::graphics {

class Material {
public:
    virtual ~Material() = default;

    virtual void bind(void* command_buffer) const = 0;

    virtual void update_uniform_buffer(const void* data) = 0;

    virtual std::string backend_name() const = 0;
};

} // namespace core::graphics

#endif // CORE_GRAPHICS_MATERIAL_HPP
