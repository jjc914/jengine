#ifndef engine_core_graphics_PIPELINE_HPP
#define engine_core_graphics_PIPELINE_HPP

#include "material.hpp"
#include "descriptor_set_layout.hpp"
#include "image_types.hpp"

#include <string>
#include <memory>

namespace engine::core::graphics {

class Pipeline {
public:
    virtual ~Pipeline() = default;

    virtual void bind(void* cb) const = 0;
    
    virtual core::graphics::ImageFormat color_format() const = 0;
    virtual core::graphics::ImageFormat depth_format() const = 0;
    
    virtual std::unique_ptr<Material> create_material(
        const DescriptorSetLayout& layout,
        uint32_t uniform_buffer_size
    ) const = 0;

    virtual void* native_pipeline() const = 0;
    virtual void* native_render_pass() const { return nullptr; }
    virtual std::string backend_name() const = 0;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_RENDER_PASS_HPP
