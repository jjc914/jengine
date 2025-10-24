#include "editor_renderer.hpp"

#include "engine/drivers/vulkan/vulkan_instance.hpp"
#include "engine/import/mesh.hpp"

#include "engine/core/renderer/frame_graph/render_pass.hpp"
#include "engine/core/renderer/frame_graph/attachment.hpp"

#include "engine/core/debug/logger.hpp"
#include "engine/core/debug/assert.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace engine::core;
using namespace engine::core::graphics;
using namespace engine::core::renderer;
using namespace engine::core::renderer::framegraph;

namespace editor::renderer {

EditorRenderer::EditorRenderer(const window::Window& main_window) {
    _width = main_window.width();
    _height = main_window.height();

    // create core graphics objects
    _instance = std::make_unique<engine::drivers::vulkan::VulkanInstance>();
    _device = _instance->create_device(main_window);

    // create caches
    _mesh_cache = std::make_unique<engine::core::renderer::cache::MeshCache>(*_device);
    _shader_cache = std::make_unique<engine::core::renderer::cache::ShaderCache>(*_device);
    _material_cache = std::make_unique<engine::core::renderer::cache::MaterialCache>(*_device);
    _pipeline_cache = std::make_unique<engine::core::renderer::cache::PipelineCache>(*_device);

    // default meshes
    import::ObjModel sphere_model = import::ReadObj("res/sphere.obj");
    _mesh_cache_entries["sphere"] = _mesh_cache->register_mesh(sphere_model);

    import::ObjModel cube_model = import::ReadObj("res/cube.obj");
    _mesh_cache_entries["cube"] = _mesh_cache->register_mesh(cube_model);

    // register shaders
    engine::core::renderer::cache::ShaderCacheId imgui_vid = _shader_cache->register_shader(
        ShaderStageFlags::VERTEX,
        "shaders/imgui.vert.spv"
    );
    engine::core::renderer::cache::ShaderCacheId imgui_fid = _shader_cache->register_shader(
        ShaderStageFlags::FRAGMENT,
        "shaders/imgui.frag.spv"
    );

    // create gui pipeline
    std::vector<engine::core::graphics::ImageAttachmentInfo> swapchain_attachments = {
        { _device->present_format(), engine::core::graphics::TextureUsage::PRESENT_SRC }
    };

    VertexBindingDescription imgui_binding =
        VertexBindingDescription{}
            .set_binding(0)
            .set_stride(sizeof(ImDrawVert))
            .set_attributes({
                VertexAttribute{}.set_location(0).set_format(VertexFormat::FLOAT_2).set_offset(offsetof(ImDrawVert, pos)),
                VertexAttribute{}.set_location(1).set_format(VertexFormat::FLOAT_2).set_offset(offsetof(ImDrawVert, uv)),
                VertexAttribute{}.set_location(2).set_format(VertexFormat::UNORM8_4).set_offset(offsetof(ImDrawVert, col))
            });

    engine::core::graphics::PipelineConfig imgui_config = PipelineConfig{}
        .set_depth_test(false)
        .set_depth_write(false)
        .set_blending(true)
        .set_cull_mode(CullMode::NONE)
        .set_push_constant(sizeof(glm::mat4), ShaderStageFlags::VERTEX);
    
    _imgui_layout = _device->create_descriptor_set_layout(
        engine::core::graphics::DescriptorLayoutDescription{}
            .add_binding(engine::core::graphics::DescriptorLayoutBinding{}
                .set_binding(0)
                .set_type(engine::core::graphics::DescriptorType::COMBINED_IMAGE_SAMPLER)
                .set_visibility(engine::core::graphics::ShaderStageFlags::FRAGMENT))
    );

    _imgui_pipeline_id = _pipeline_cache->register_pipeline(
        *_shader_cache->get(imgui_vid),
        *_shader_cache->get(imgui_fid),
        *_imgui_layout,
        imgui_binding,
        swapchain_attachments,
        imgui_config
    );

    const engine::core::graphics::Pipeline* imgui_pipeline = _pipeline_cache->get(_imgui_pipeline_id);

    // create main swapchain
    _main_swapchain = _device->create_swapchain_render_target(main_window, *imgui_pipeline, 3, false);

    // initialize gui
    _editor_gui = std::make_unique<gui::EditorGui>(
        *_instance,
        *_device,
        *imgui_pipeline,
        main_window
    );

    // create frame graph
    _frame_graph = std::make_unique<FrameGraph>(
        _device.get(),
        *_shader_cache.get(),
        *_pipeline_cache.get()
    );

    AttachmentDescription swapchain_attachment_desc{};
    swapchain_attachment_desc.width  = _width;
    swapchain_attachment_desc.height = _height;
    swapchain_attachment_desc.format = _device->present_format();
    swapchain_attachment_desc.texture_override = _main_swapchain->frame_color_texture(_main_swapchain->frame_index());
    _swapchain_attachment = _frame_graph->register_attachment(swapchain_attachment_desc);

    // present pass
    engine::core::renderer::framegraph::RenderPass gui_pass;

    gui_pass.set_clear_color(glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));
    gui_pass.set_clear_depth(glm::vec2(1.0f, 0.0f));
    gui_pass.add_write_color(_swapchain_attachment);

    gui_pass.set_execute([this](engine::core::renderer::framegraph::RenderPassContext& context) {
        gui::GuiContext gui_context;
        gui_context.command_buffer = context.command_buffer;

        _editor_gui->on_gui(gui_context);
    });

    gui_pass.set_vertex_shader(imgui_vid);
    gui_pass.set_vertex_shader(imgui_fid);
    gui_pass.set_vertex_binding(imgui_binding);

    gui_pass.set_pipeline_override(_imgui_pipeline_id);
    gui_pass.set_render_target_override(_main_swapchain.get());
    
    _frame_graph->add_pass(gui_pass);

    _frame_graph->bake();
}

void EditorRenderer::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (width == _width && height == _height) return;

    _width = width;
    _height = height;

    // rebake frame graph
    _frame_graph->bake();
}

void EditorRenderer::render_scene(engine::core::scene::Scene& scene) {
    // execute frame graph
    _frame_graph->update_attachment_texture(_swapchain_attachment, _main_swapchain->frame_color_texture(_main_swapchain->frame_index()));
    _frame_graph->execute();
    _main_swapchain->present();
}

} // namespace editor::renderer
