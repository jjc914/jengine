#include "editor_renderer.hpp"
#include "editor_scene_view_renderer.hpp"

#include "engine/drivers/vulkan/vulkan_instance.hpp"
#include "engine/import/mesh.hpp"

#include "engine/core/debug/logger.hpp"
#include "editor/components/transform.hpp"
#include "editor/components/mesh_renderer.hpp"
#include "editor/scene/editor_camera.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <glm/gtx/transform.hpp>

namespace editor::renderer {

EditorRenderer::EditorRenderer(const engine::core::window::Window& main_window) {
    _width = main_window.width();
    _height = main_window.height();

    // driver setup
    _instance = std::make_unique<engine::drivers::vulkan::VulkanInstance>();
    _device = _instance->create_device(main_window);

    // caches
    _mesh_cache = std::make_unique<engine::core::renderer::cache::MeshCache>(*_device);
    _shader_cache = std::make_unique<engine::core::renderer::cache::ShaderCache>(*_device);
    _material_cache = std::make_unique<engine::core::renderer::cache::MaterialCache>(*_device);
    _pipeline_cache = std::make_unique<engine::core::renderer::cache::PipelineCache>(*_device);

    _mesh_cache_entries.clear();
    _shader_cache_entries.clear();
    _material_cache_entries.clear();
    _pipeline_cache_entries.clear();

    // default meshes
    import::ObjModel sphere_model = import::ReadObj("res/sphere.obj");
    _mesh_cache_entries["sphere"] = _mesh_cache->register_mesh(sphere_model);

    import::ObjModel cube_model = import::ReadObj("res/cube.obj");
    _mesh_cache_entries["cube"] = _mesh_cache->register_mesh(cube_model);

    // default shaders
    _shader_cache_entries["editor_view_color_vert"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::VERTEX, "shaders/editor_view_color.vert.spv");
    _shader_cache_entries["editor_view_color_frag"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::FRAGMENT, "shaders/editor_view_color.frag.spv");
    
    _shader_cache_entries["editor_present_vert"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::VERTEX, "shaders/editor_present.vert.spv");
    _shader_cache_entries["editor_present_frag"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::FRAGMENT, "shaders/editor_present.frag.spv");
   
    _shader_cache_entries["editor_pick_vert"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::VERTEX, "shaders/editor_pick.vert.spv");
    _shader_cache_entries["editor_pick_frag"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::FRAGMENT, "shaders/editor_pick.frag.spv");
    
    _shader_cache_entries["skybox_vert"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::VERTEX, "shaders/skybox.vert.spv");
    _shader_cache_entries["skybox_frag"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::FRAGMENT, "shaders/skybox.frag.spv");

    // vertex bindings
    engine::core::graphics::VertexBinding vertex_binding =
        engine::core::graphics::VertexBinding{}
            .set_binding(0)
            .set_stride(sizeof(import::ObjVertex))
            .set_attributes({
                engine::core::graphics::VertexAttribute{}
                    .set_location(0)
                    .set_format(engine::core::graphics::VertexFormat::FLOAT3)
                    .set_offset(offsetof(import::ObjVertex, import::ObjVertex::position)),
                engine::core::graphics::VertexAttribute{}
                    .set_location(1)
                    .set_format(engine::core::graphics::VertexFormat::FLOAT3)
                    .set_offset(offsetof(import::ObjVertex, import::ObjVertex::normal))
            });

    engine::core::graphics::VertexBinding pick_binding =
        engine::core::graphics::VertexBinding{}
            .set_binding(0)
            .set_stride(sizeof(import::ObjVertex))
            .set_attributes({
                engine::core::graphics::VertexAttribute{}
                    .set_location(0)
                    .set_format(engine::core::graphics::VertexFormat::FLOAT3)
                    .set_offset(offsetof(import::ObjVertex, import::ObjVertex::position))
            });

    // attachment formats
    std::vector<engine::core::graphics::ImageAttachmentInfo> skybox_attachments = {
        { engine::core::graphics::ImageFormat::RGBA8_UNORM, engine::core::graphics::ImageUsage::COLOR | engine::core::graphics::ImageUsage::SAMPLING },
        { engine::core::graphics::ImageFormat::D32_FLOAT,   engine::core::graphics::ImageUsage::DEPTH },
    };

    std::vector<engine::core::graphics::ImageAttachmentInfo> view_attachments = {
        { engine::core::graphics::ImageFormat::RGBA8_UNORM, engine::core::graphics::ImageUsage::COLOR | engine::core::graphics::ImageUsage::SAMPLING },
        { engine::core::graphics::ImageFormat::D32_FLOAT,   engine::core::graphics::ImageUsage::DEPTH },
    };

    std::vector<engine::core::graphics::ImageAttachmentInfo> pick_attachments = {
        { engine::core::graphics::ImageFormat::R32_UINT, engine::core::graphics::ImageUsage::COLOR | engine::core::graphics::ImageUsage::COPY_SRC },
        { engine::core::graphics::ImageFormat::D32_FLOAT, engine::core::graphics::ImageUsage::DEPTH }
    };

    std::vector<engine::core::graphics::ImageAttachmentInfo> present_attachments = {
        { _device->present_format(), engine::core::graphics::ImageUsage::PRESENT },
        { _device->depth_format(),   engine::core::graphics::ImageUsage::DEPTH }
    };

    // descriptor layouts
    engine::core::graphics::DescriptorLayoutDescription scene_layout_desc =
        engine::core::graphics::DescriptorLayoutDescription{}
            .add_binding(engine::core::graphics::DescriptorLayoutBinding{}
                .set_binding(0)
                .set_type(engine::core::graphics::DescriptorType::UNIFORM_BUFFER)
                .set_visibility(engine::core::graphics::ShaderStageFlags::VERTEX));
    
    engine::core::graphics::DescriptorLayoutDescription skybox_layout_desc =
        engine::core::graphics::DescriptorLayoutDescription{}
            .add_binding(engine::core::graphics::DescriptorLayoutBinding{}
                .set_binding(0)
                .set_type(engine::core::graphics::DescriptorType::UNIFORM_BUFFER)
                .set_visibility(engine::core::graphics::ShaderStageFlags::VERTEX));

    engine::core::graphics::DescriptorLayoutDescription present_layout_desc =
        engine::core::graphics::DescriptorLayoutDescription{}
            .add_binding(engine::core::graphics::DescriptorLayoutBinding{}
                .set_binding(0)
                .set_type(engine::core::graphics::DescriptorType::COMBINED_IMAGE_SAMPLER)
                .set_visibility(engine::core::graphics::ShaderStageFlags::FRAGMENT));

    // default pipelines
    _pipeline_cache_entries["editor_view_color"] =
        _pipeline_cache->register_pipeline(
            _shader_cache->get(_shader_cache_entries["editor_view_color_vert"]),
            _shader_cache->get(_shader_cache_entries["editor_view_color_frag"]),
            _material_cache->get_or_create_layout(scene_layout_desc),
            vertex_binding,
            view_attachments,
            engine::core::graphics::PipelineConfig{}
                .set_blending(true)
                .set_depth_test(true)
                .set_depth_write(true)
        );

    _pipeline_cache_entries["editor_pick"] =
        _pipeline_cache->register_pipeline(
            _shader_cache->get(shader_id("editor_pick_vert")),
            _shader_cache->get(shader_id("editor_pick_frag")),
            _material_cache->get_or_create_layout(scene_layout_desc),
            pick_binding,
            pick_attachments,
            engine::core::graphics::PipelineConfig{}
                .set_depth_test(true)
                .set_depth_write(true)
                .set_blending(false)
                .set_push_constant(sizeof(uint32_t), engine::core::graphics::ShaderStageFlags::FRAGMENT)
        );
    
    _pipeline_cache_entries["skybox"] =
        _pipeline_cache->register_pipeline(
            _shader_cache->get(shader_id("skybox_vert")),
            _shader_cache->get(shader_id("skybox_frag")),
            _material_cache->get_or_create_layout(skybox_layout_desc),
            vertex_binding,
            skybox_attachments,
            engine::core::graphics::PipelineConfig{}
                .set_depth_test(true)
                .set_depth_write(false)
                .set_blending(false)
                .set_cull_mode(engine::core::graphics::CullMode::FRONT)
        );

    _pipeline_cache_entries["editor_present"] =
        _pipeline_cache->register_pipeline(
            _shader_cache->get(_shader_cache_entries["editor_present_vert"]),
            _shader_cache->get(_shader_cache_entries["editor_present_frag"]),
            _material_cache->get_or_create_layout(present_layout_desc),
            engine::core::graphics::VertexBinding{},
            present_attachments,
            engine::core::graphics::PipelineConfig{}
                .set_depth_test(true)
                .set_depth_write(true)
                .set_blending(true)
        );

    // default materials
    _material_cache_entries["editor_view_default"] =
        _material_cache->register_material(
            _pipeline_cache->get(_pipeline_cache_entries["editor_view_color"]),
            scene_layout_desc,
            sizeof(UniformBufferObject)
        );

    _material_cache_entries["editor_view_pick"] =
        _material_cache->register_material(
            _pipeline_cache->get(_pipeline_cache_entries["editor_pick"]),
            scene_layout_desc,
            sizeof(UniformBufferObject)
        );
    
    _material_cache_entries["skybox"] =
        _material_cache->register_material(
            _pipeline_cache->get(_pipeline_cache_entries["skybox"]),
            skybox_layout_desc,
            sizeof(glm::mat4) * 2
        );
    
    _material_cache_entries["editor_present"] =
        _material_cache->register_material(
            _pipeline_cache->get(_pipeline_cache_entries["editor_present"]),
            present_layout_desc,
            0
        );

    // gui
    _editor_gui = std::make_unique<gui::EditorGui>(
        *_instance,
        *_device,
        _pipeline_cache->get(_pipeline_cache_entries["editor_present"]),
        main_window
    );

    // scene view renderer
    EditorRendererContext context{
        _device.get(),
        *_pipeline_cache,
        *_material_cache,
        *_mesh_cache,
        *_editor_gui,
        mesh_id("cube"),
        pipeline_id("skybox"), material_id("skybox"),
        pipeline_id("editor_view_color"),
        pipeline_id("editor_pick"), material_id("editor_view_pick")
    };

    _scene_view_renderer = std::make_unique<EditorSceneViewRenderer>(context, _width, _height);

    // present viewport
    _main_viewport = _device->create_viewport(
        main_window,
        _pipeline_cache->get(_pipeline_cache_entries["editor_present"]),
        _width,
        _height
    );
}

void EditorRenderer::resize(uint32_t width, uint32_t height) {
    _width = width;
    _height = height;
    _main_viewport->resize(_width, _height);
    _scene_view_renderer->resize(_width, _height);
}

void EditorRenderer::render_scene(engine::core::scene::Scene& scene) {
    _frame_graph.clear();

    // register scene view renderer passes to frame graph
    _scene_view_renderer->register_passes(scene, _frame_graph);

    // present pass
    _present_pass_id = _frame_graph.add_pass(
        &_pipeline_cache->get(pipeline_id("editor_present")),
        _main_viewport.get(),
        [this, &scene](engine::core::renderer::RenderPassContext& ctx, engine::core::renderer::RenderPassId) {
            ctx.command_buffer = ctx.target->begin_frame(*ctx.pipeline);
            ENGINE_ASSERT(ctx.command_buffer, "FrameGraph expects a valid command buffer");

            gui::GuiContext gui_context;
            gui_context.command_buffer = ctx.command_buffer;
            gui_context.scene_view.texture_id = _scene_view_renderer->texture_id(_scene_view_renderer->frame_index());
            gui_context.scene_view.camera = &_scene_view_renderer->camera();
            gui_context.scene_view.scene = &scene;
            gui_context.scene_view.selected_entity = _scene_view_renderer->selected_entity();

            _editor_gui->on_gui(gui_context);

            _scene_view_renderer->resize(gui_context.scene_view.out_size.x, gui_context.scene_view.out_size.y);

            _scene_view_renderer->handle_picking(_scene_view_renderer->frame_index(),
                glm::vec2(gui_context.scene_view.out_pos.x, gui_context.scene_view.out_pos.y),
                glm::vec2(gui_context.scene_view.out_size.x, gui_context.scene_view.out_size.y)
            );

            ctx.target->end_frame();
        }
    );

    // execute frame graph
    engine::core::renderer::RenderPassContext context{};
    _frame_graph.execute(context);
}

} // namespace editor::renderer
