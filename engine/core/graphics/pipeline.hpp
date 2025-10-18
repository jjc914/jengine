#ifndef engine_core_graphics_PIPELINE_HPP
#define engine_core_graphics_PIPELINE_HPP

#include "material.hpp"
#include "descriptor_set_layout.hpp"
#include "image_types.hpp"
#include "descriptor_types.hpp"

#include <string>
#include <memory>

namespace engine::core::graphics {

struct PipelineConfig {
    bool blending_enabled = true;
    bool depth_test_enabled = true;
    bool depth_write_enabled = true;
    CullMode cull_mode = CullMode::BACK;
    struct {
        uint32_t size = 0;
        ShaderStageFlags stage_flags = ShaderStageFlags::NONE;
    } push_constant;

    PipelineConfig& set_blending(bool b) { blending_enabled = b; return *this; }
    PipelineConfig& set_depth_test(bool b) { depth_test_enabled = b; return *this; }
    PipelineConfig& set_depth_write(bool b) { depth_write_enabled = b; return *this; }
    PipelineConfig& set_cull_mode(CullMode m) { cull_mode = m; return *this; }
    PipelineConfig& set_push_constant(uint32_t s, ShaderStageFlags f) { push_constant.size = s; push_constant.stage_flags = f; return *this; }
};

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
    virtual void* native_pipeline_layout() const = 0;
    virtual void* native_render_pass() const { return nullptr; }
    virtual std::string backend_name() const = 0;
};

} // namespace engine::core::graphics

#endif // engine_core_graphics_RENDER_PASS_HPP
