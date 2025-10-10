#ifndef engine_core_graphics_SHADER_HPP
#define engine_core_graphics_SHADER_HPP

#include "descriptor_types.hpp"

#include <string>
#include <vector>
#include <memory>

namespace engine::core::graphics {

class Shader {
public:
    virtual ~Shader() = default;
    
    virtual ShaderStageFlags stage() const = 0;
    
    virtual void* native_handle() const = 0;
    virtual std::string backend_name() const = 0;
};

using ShaderPtr = std::shared_ptr<Shader>;

} // namespace engine::core::graphics

#endif // engine_core_graphics_SHADER_HPP