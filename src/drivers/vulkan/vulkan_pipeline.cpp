#include "vulkan_pipeline.hpp"
#include "vulkan_renderer.hpp"
#include "vulkan_material.hpp"

namespace drivers::vulkan {

static VkFormat ToVkFormat(core::graphics::VertexFormat format) {
    switch (format) {
        case core::graphics::VertexFormat::R32_FLOAT:      return VK_FORMAT_R32_SFLOAT;
        case core::graphics::VertexFormat::RG32_FLOAT:     return VK_FORMAT_R32G32_SFLOAT;
        case core::graphics::VertexFormat::RGB32_FLOAT:    return VK_FORMAT_R32G32B32_SFLOAT;
        case core::graphics::VertexFormat::RGBA32_FLOAT:   return VK_FORMAT_R32G32B32A32_SFLOAT;

        case core::graphics::VertexFormat::R8_UNORM:       return VK_FORMAT_R8_UNORM;
        case core::graphics::VertexFormat::RG8_UNORM:      return VK_FORMAT_R8G8_UNORM;
        case core::graphics::VertexFormat::RGB8_UNORM:     return VK_FORMAT_R8G8B8_UNORM;
        case core::graphics::VertexFormat::RGBA8_UNORM:    return VK_FORMAT_R8G8B8A8_UNORM;

        case core::graphics::VertexFormat::R16_FLOAT:      return VK_FORMAT_R16_SFLOAT;
        case core::graphics::VertexFormat::RG16_FLOAT:     return VK_FORMAT_R16G16_SFLOAT;
        case core::graphics::VertexFormat::RGB16_FLOAT:    return VK_FORMAT_R16G16B16_SFLOAT;
        case core::graphics::VertexFormat::RGBA16_FLOAT:   return VK_FORMAT_R16G16B16A16_SFLOAT;

        case core::graphics::VertexFormat::R32_UINT:       return VK_FORMAT_R32_UINT;
        case core::graphics::VertexFormat::RG32_UINT:      return VK_FORMAT_R32G32_UINT;
        case core::graphics::VertexFormat::RGB32_UINT:     return VK_FORMAT_R32G32B32_UINT;
        case core::graphics::VertexFormat::RGBA32_UINT:    return VK_FORMAT_R32G32B32A32_UINT;

        default:                           return VK_FORMAT_UNDEFINED;
    }
}

VulkanPipeline::VulkanPipeline(const VulkanRenderer& renderer, const wk::Device& device, const core::graphics::VertexLayout& layout)
    : _device(device), _render_pass(renderer.render_pass()),
      _allocator(renderer.allocator()), _descriptor_pool(renderer.descriptor_pool())
{
    uint32_t height = renderer.height();
    uint32_t width = renderer.width();
    // ---------- descriptor set layout ----------
    VkDescriptorSetLayoutBinding bindings[] = {
        wk::DescriptorSetLayoutBinding{}
            .set_binding(0)
            .set_descriptor_type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            .set_descriptor_count(1)
            .set_stage_flags(VK_SHADER_STAGE_VERTEX_BIT)
            .to_vk()
    };

    _descriptor_set_layout = wk::DescriptorSetLayout(_device.handle(),
        wk::DescriptorSetLayoutCreateInfo{}
            .set_bindings(1, bindings)
            .to_vk()
    );

    VkDescriptorSetLayout layouts[] = { _descriptor_set_layout.handle() };

    _pipeline_layout = wk::PipelineLayout(_device.handle(),
        wk::PipelineLayoutCreateInfo{}
            .set_set_layouts(1, layouts)
            .to_vk()
    );

    // ---------- shaders ----------
    std::vector<uint8_t> vert_byte_code = wk::ReadSpirvShader("shaders/triangle.vert.spv");
    std::vector<uint8_t> frag_byte_code = wk::ReadSpirvShader("shaders/triangle.frag.spv");

    wk::Shader vert(_device.handle(),
        wk::ShaderCreateInfo{}
            .set_byte_code(vert_byte_code.size(), vert_byte_code.data())
            .to_vk()
    );
    wk::Shader frag(_device.handle(),
        wk::ShaderCreateInfo{}
            .set_byte_code(frag_byte_code.size(), frag_byte_code.data())
            .to_vk()
    );

    VkPipelineShaderStageCreateInfo shader_stages[2] = {
        wk::PipelineShaderStageCreateInfo{}
            .set_stage(VK_SHADER_STAGE_VERTEX_BIT)
            .set_module(vert.handle())
            .set_p_name("main")
            .to_vk(),
        wk::PipelineShaderStageCreateInfo{}
            .set_stage(VK_SHADER_STAGE_FRAGMENT_BIT)
            .set_module(frag.handle())
            .set_p_name("main")
            .to_vk()
    };

    // ---------- viewport and scissor ----------
    VkViewport viewport = wk::Viewport{}
        .set_x(0.0f)
        .set_y(0.0f)
        .set_width(static_cast<float>(width))
        .set_height(static_cast<float>(height))
        .set_min_depth(0.0f)
        .set_max_depth(1.0f)
        .to_vk();

    VkRect2D scissor = wk::Rect2D{}
        .set_offset({0, 0})
        .set_extent(wk::Extent{}.set_width(width).set_height(height).to_vk_extent_2d())
        .to_vk();

    // ---------- vertex input ----------
    std::vector<VkVertexInputBindingDescription> vk_bindings;
    vk_bindings.reserve(layout.bindings.size());
    for (const auto& b : layout.bindings) {
        VkVertexInputRate rate =
            (b.input_rate == core::graphics::VertexBinding::InputRate::INSTANCE)
            ? VK_VERTEX_INPUT_RATE_INSTANCE
            : VK_VERTEX_INPUT_RATE_VERTEX;
        vk_bindings.push_back({ b.binding, b.stride, rate });
    }

    std::vector<VkVertexInputAttributeDescription> vk_attributes;
    vk_attributes.reserve(layout.attributes.size());
    for (const auto& a : layout.attributes) {
        vk_attributes.push_back({
            a.location,
            a.binding,
            ToVkFormat(a.format),
            a.offset
        });
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_ci =
        wk::PipelineVertexInputStateCreateInfo{}
            .set_vertex_binding_descriptions((uint32_t)vk_bindings.size(), vk_bindings.data())
            .set_vertex_attribute_descriptions((uint32_t)vk_attributes.size(), vk_attributes.data())
            .to_vk();

    // ---------- other pipeline states ----------
    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci =
        wk::PipelineInputAssemblyStateCreateInfo{}
            .set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .set_primitive_restart_enable(VK_FALSE)
            .to_vk();

    VkPipelineViewportStateCreateInfo viewport_state_ci =
        wk::PipelineViewportStateCreateInfo{}
            .set_viewports(1, &viewport)
            .set_scissors(1, &scissor)
            .to_vk();

    VkPipelineRasterizationStateCreateInfo raster_ci =
        wk::PipelineRasterizationStateCreateInfo{}
            .set_polygon_mode(VK_POLYGON_MODE_FILL)
            .set_cull_mode(VK_CULL_MODE_BACK_BIT)
            .set_front_face(VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .set_line_width(1.0f)
            .to_vk();

    VkPipelineMultisampleStateCreateInfo multisample_ci =
        wk::PipelineMultisampleStateCreateInfo{}
            .set_rasterization_samples(VK_SAMPLE_COUNT_1_BIT)
            .to_vk();

    VkPipelineDepthStencilStateCreateInfo depth_stencil_ci =
        wk::PipelineDepthStencilStateCreateInfo{}
            .set_depth_test_enable(VK_TRUE)
            .set_depth_write_enable(VK_TRUE)
            .set_depth_compare_op(VK_COMPARE_OP_LESS)
            .to_vk();

    VkPipelineColorBlendAttachmentState color_blend_attachment =
        wk::PipelineColorBlendAttachmentState{}
            .set_blend_enable(VK_TRUE)
            .set_color_write_mask(
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)
            .set_src_color_blend_factor(VK_BLEND_FACTOR_SRC_ALPHA)
            .set_dst_color_blend_factor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
            .set_color_blend_op(VK_BLEND_OP_ADD)
            .to_vk();

    VkPipelineColorBlendStateCreateInfo color_blend_state_ci =
        wk::PipelineColorBlendStateCreateInfo{}
            .set_attachments(1, &color_blend_attachment)
            .to_vk();

    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_ci =
        wk::PipelineDynamicStateCreateInfo{}
            .set_dynamic_states(2, dynamic_states)
            .to_vk();

    // ---------- create pipeline ----------
    _pipeline = wk::Pipeline(_device.handle(),
        wk::PipelineCreateInfo{}
            .set_stages(2, shader_stages)
            .set_p_vertex_input_state(&vertex_input_ci)
            .set_p_input_assembly_state(&input_assembly_ci)
            .set_p_viewport_state(&viewport_state_ci)
            .set_p_rasterization_state(&raster_ci)
            .set_p_multisample_state(&multisample_ci)
            .set_p_depth_stencil_state(&depth_stencil_ci)
            .set_p_color_blend_state(&color_blend_state_ci)
            .set_p_dynamic_state(&dynamic_ci)
            .set_layout(_pipeline_layout.handle())
            .set_render_pass(_render_pass.handle())
            .set_subpass(0)
            .to_vk()
    );
}

void VulkanPipeline::bind(void* command_buffer) const {
    vkCmdBindPipeline(static_cast<VkCommandBuffer>(command_buffer), VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.handle());
}

std::unique_ptr<core::graphics::Material> VulkanPipeline::create_material(uint32_t uniform_buffer_size) const {
    return std::make_unique<VulkanMaterial>(
        *this,
        _device, _allocator, _descriptor_pool, _descriptor_set_layout,
        uniform_buffer_size
    );
}

} // namespace drivers::vulkan
