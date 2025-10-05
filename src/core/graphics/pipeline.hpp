#ifndef CORE_GRAPHICS_PIPELINE_HPP
#define CORE_GRAPHICS_PIPELINE_HPP

#include "material.hpp"

#include <memory>

namespace core::graphics {

class Pipeline {
public:
    virtual ~Pipeline() = default;

    virtual void bind(void* command_buffer) const = 0;

    virtual std::unique_ptr<Material> create_material(uint32_t uniform_buffer_size) const = 0;
};

}

#endif