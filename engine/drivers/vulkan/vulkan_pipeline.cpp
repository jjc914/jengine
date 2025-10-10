#include "vulkan_pipeline.hpp"

#include "vulkan_device.hpp"
#include "vulkan_material.hpp"
#include "convert_vulkan.hpp"

#include "engine/core/debug/assert.hpp"
#include "engine/core/debug/logger.hpp"

namespace engine::drivers::vulkan {

VulkanPipeline::VulkanPipeline(const VulkanDevice& device,
    VkShaderModule vert, VkShaderModule frag,
    const core::graphics::VertexBinding& vertex_binding, // TODO: add support for multiple bindings
    const core::graphics::DescriptorSetLayout& layout,
    const std::vector<core::graphics::ImageAttachmentInfo>& attachment_info) 
    : _device(device.device()), _allocator(device.allocator()), _descriptor_pool(device.descriptor_pool()),
      _attachment_info(attachment_info)
{
    ENGINE_ASSERT(!attachment_info.empty(), "VulkanPipeline requires at least one image attachment");
    ENGINE_ASSERT(!vertex_binding.attributes.empty(), "VulkanPipeline requires at least one vertex attribute");

    // render pass
    std::vector<VkAttachmentDescription> attachment_descriptions;
    std::vector<VkAttachmentReference> color_attachment_references;
    bool has_depth = false;
    VkAttachmentReference depth_attachment_reference{};
    for (int i = 0; i < _attachment_info.size(); ++i) {
        attachment_descriptions.emplace_back(
            wk::AttachmentDescription{}
                .set_flags(0)
                .set_format(ToVkFormat(_attachment_info[i].format))
                .set_samples(VK_SAMPLE_COUNT_1_BIT)
                .set_load_op(VK_ATTACHMENT_LOAD_OP_CLEAR)
                .set_store_op(VK_ATTACHMENT_STORE_OP_STORE)
                .set_stencil_load_op(VK_ATTACHMENT_LOAD_OP_DONT_CARE)
                .set_stencil_store_op(VK_ATTACHMENT_STORE_OP_DONT_CARE)
                .set_initial_layout(VK_IMAGE_LAYOUT_UNDEFINED)
                .set_final_layout(ToVkImageLayout(_attachment_info[i].usage))
                .to_vk()
        );

        if (_attachment_info[i].usage == core::graphics::ImageUsage::PRESENT || _attachment_info[i].usage == core::graphics::ImageUsage::COLOR) {
            color_attachment_references.emplace_back(
            wk::AttachmentReference{}
                .set_attachment(i)
                .set_layout(ToVkImageLayout(core::graphics::ImageUsage::COLOR))
                .to_vk()
            );
        }
        if (_attachment_info[i].usage == core::graphics::ImageUsage::PRESENT) {
            _present_color_format = _attachment_info[i].format;
        } else if (_attachment_info[i].usage == core::graphics::ImageUsage::DEPTH) {
            if (!has_depth) {
                depth_attachment_reference = wk::AttachmentReference{}
                    .set_attachment(i)
                    .set_layout(ToVkImageLayout(core::graphics::ImageUsage::DEPTH))
                    .to_vk();
                _depth_format = _attachment_info[i].format;
                has_depth = true;
            } else {
                core::debug::Logger::get_singleton().warn("Multiple depth attachments are not supported. Using the first one");
            }
        }
    }

    ENGINE_ASSERT(has_depth || !color_attachment_references.empty(), "Pipeline must have at least one color or depth attachment");

    VkSubpassDescription subpass = wk::SubpassDescription{}
        .set_pipeline_bind_point(VK_PIPELINE_BIND_POINT_GRAPHICS)
        .set_color_attachments(color_attachment_references.size(), color_attachment_references.data())
        .set_depth_stencil_attachment(&depth_attachment_reference)
        .to_vk();

    VkSubpassDependency dependency = wk::SubpassDependency{}
        .set_src_subpass(VK_SUBPASS_EXTERNAL)
        .set_dst_subpass(0)
        .set_src_stage_mask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        .set_dst_stage_mask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        .set_src_access_mask(0)
        .set_dst_access_mask(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        .to_vk();

    _render_pass = wk::RenderPass(_device.handle(),
        wk::RenderPassCreateInfo{}
            .set_attachments(attachment_descriptions.size(), attachment_descriptions.data())
            .set_subpasses(1, &subpass)
            .set_dependencies(1, &dependency)
            .to_vk()
    );

    VkDescriptorSetLayout layouts[] = { static_cast<VkDescriptorSetLayout>(layout.native_descriptor_set_layout())};

    _pipeline_layout = wk::PipelineLayout(_device.handle(),
        wk::PipelineLayoutCreateInfo{}
            .set_set_layouts(1, layouts)
            .to_vk()
    );

    // shaders
    VkPipelineShaderStageCreateInfo shader_stages[2] = {
        wk::PipelineShaderStageCreateInfo{}
            .set_stage(VK_SHADER_STAGE_VERTEX_BIT)
            .set_module(vert)
            .set_p_name("main")
            .to_vk(),
        wk::PipelineShaderStageCreateInfo{}
            .set_stage(VK_SHADER_STAGE_FRAGMENT_BIT)
            .set_module(frag)
            .set_p_name("main")
            .to_vk()
    };

    // vertex input
    VkVertexInputBindingDescription vertex_input_binding = wk::VertexInputBindingDescription{}
        .set_binding(vertex_binding.binding)
        .set_stride(vertex_binding.stride)
        .set_input_rate(VK_VERTEX_INPUT_RATE_VERTEX)
        .to_vk();

    std::vector<VkVertexInputAttributeDescription> vertex_input_attributes;
    vertex_input_attributes.reserve(vertex_binding.attributes.size());
    for (const core::graphics::VertexAttribute& a : vertex_binding.attributes) {
        vertex_input_attributes.emplace_back(
            a.location,
            vertex_binding.binding,
            ToVkFormat(a.format),
            a.offset
        );
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_ci =
        wk::PipelineVertexInputStateCreateInfo{}
            .set_vertex_binding_descriptions(1, &vertex_input_binding)
            .set_vertex_attribute_descriptions(vertex_input_attributes.size(), vertex_input_attributes.data())
            .to_vk();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_ci =
        wk::PipelineInputAssemblyStateCreateInfo{}
            .set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .set_primitive_restart_enable(VK_FALSE)
            .to_vk();

    VkPipelineViewportStateCreateInfo viewport_state_ci =
        wk::PipelineViewportStateCreateInfo{}
            .set_viewport_count(1)
            .set_scissor_count(1)
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

    // pipeline
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

void VulkanPipeline::bind(void* cb) const {
    ENGINE_ASSERT(cb != nullptr, "Attempted to bind pipeline with null command buffer");

    vkCmdBindPipeline(static_cast<VkCommandBuffer>(cb), VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.handle());
}

std::unique_ptr<core::graphics::Material> VulkanPipeline::create_material(
    const core::graphics::DescriptorSetLayout& layout,
    uint32_t uniform_buffer_size
) const {
    return std::make_unique<VulkanMaterial>(
        *this, static_cast<VkDescriptorSetLayout>(layout.native_descriptor_set_layout()),
        uniform_buffer_size
    );
}

}