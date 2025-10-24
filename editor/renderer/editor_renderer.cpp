#include "editor_renderer.hpp"

#include "engine/drivers/vulkan/vulkan_instance.hpp"
#include "engine/import/mesh.hpp"

#include "engine/core/renderer/frame_graph/render_pass.hpp"
#include "engine/core/renderer/frame_graph/attachment.hpp"

#include "engine/core/debug/logger.hpp"
#include "engine/core/debug/assert.hpp"

#include "editor/gui/editor_gui.hpp"

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

    // register imgui shaders
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

    // present pass
    engine::core::renderer::framegraph::RenderPass gui_pass;
    gui_pass.set_clear_color(glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));

    gui_pass.set_execute([this](engine::core::renderer::framegraph::RenderPassContext& context) {
        gui::GuiContext gui_context;
        gui_context.command_buffer = context.command_buffer;

        _editor_gui->on_gui(gui_context);
    });

    gui_pass.set_vertex_shader(imgui_vid);
    gui_pass.set_fragment_shader(imgui_fid);
    gui_pass.set_vertex_binding(imgui_binding);

    gui_pass.set_pipeline_override(_imgui_pipeline_id);
    gui_pass.set_render_target_override(_main_swapchain.get());
    
    _frame_graph->add_pass(gui_pass);

    _frame_graph->bake();

    register_default_descriptor_layouts();
    register_default_shaders();
    register_default_pipelines();
}

EditorRenderer::~EditorRenderer() {
    _device->wait_idle();
}

void EditorRenderer::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (width == _width && height == _height) return;

    _width = width;
    _height = height;

    _device->wait_idle();
    _main_swapchain->resize(width, height);
    _frame_graph->bake();
}

void EditorRenderer::render() {
    // execute frame graph
    _frame_graph->execute();

    // present swapchain
    _main_swapchain->present();
}

void EditorRenderer::register_default_descriptor_layouts() {
    _named_descriptor_layouts["per_object_ubo"] = &_material_cache->get_or_create_layout(
            engine::core::graphics::DescriptorLayoutDescription{}
                .add_binding(
                    engine::core::graphics::DescriptorLayoutBinding{}
                        .set_binding(0)
                        .set_type(DescriptorType::UNIFORM_BUFFER)
                        .set_visibility(ShaderStageFlags::VERTEX)
                )
    );

    _named_descriptor_layouts["material_sampler"] = &_material_cache->get_or_create_layout(
            engine::core::graphics::DescriptorLayoutDescription{}
                .add_binding(
                    engine::core::graphics::DescriptorLayoutBinding{}
                        .set_binding(0)
                        .set_type(DescriptorType::COMBINED_IMAGE_SAMPLER)
                        .set_visibility(ShaderStageFlags::FRAGMENT)
                )
    );

    _named_descriptor_layouts["global_ubo"] = &_material_cache->get_or_create_layout(
            DescriptorLayoutDescription{}
                .add_binding(
                    DescriptorLayoutBinding{}
                        .set_binding(0)
                        .set_type(DescriptorType::UNIFORM_BUFFER)
                        .set_visibility(ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT)
                )
    );
}

void EditorRenderer::register_default_shaders() {
    _named_shaders["mesh_vert"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::VERTEX, "shaders/mesh.vert.spv");
    _named_shaders["mesh_frag"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::FRAGMENT, "shaders/mesh.frag.spv");

    _named_shaders["mesh_outline_vert"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::VERTEX, "shaders/mesh_outline.vert.spv");
    _named_shaders["mesh_outline_frag"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::FRAGMENT, "shaders/mesh_outline.frag.spv");

    _named_shaders["mesh_pick_vert"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::VERTEX, "shaders/mesh_pick.vert.spv");
    _named_shaders["mesh_pick_frag"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::FRAGMENT, "shaders/mesh_pick.frag.spv");

    _named_shaders["gizmo_vert"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::VERTEX, "shaders/gizmo.vert.spv");
    _named_shaders["gizmo_frag"] = _shader_cache->register_shader(engine::core::graphics::ShaderStageFlags::FRAGMENT, "shaders/gizmo.frag.spv");
}

void EditorRenderer::register_default_pipelines() {
    // vertex bindings
    engine::core::graphics::VertexBindingDescription position_binding =
        engine::core::graphics::VertexBindingDescription{}
            .set_binding(0)
            .set_stride(sizeof(engine::import::ObjVertex))
            .set_attributes({
                engine::core::graphics::VertexAttribute{}
                    .set_location(0)
                    .set_format(VertexFormat::FLOAT_3)
                    .set_offset(offsetof(engine::import::ObjVertex, engine::import::ObjVertex::position))
            });

    VertexBindingDescription position_normal_binding =
        VertexBindingDescription{}
            .set_binding(0)
            .set_stride(sizeof(engine::import::ObjVertex))
            .set_attributes({
                VertexAttribute{}
                    .set_location(0)
                    .set_format(VertexFormat::FLOAT_3)
                    .set_offset(offsetof(engine::import::ObjVertex, engine::import::ObjVertex::position)),
                VertexAttribute{}
                    .set_location(1)
                    .set_format(VertexFormat::FLOAT_3)
                    .set_offset(offsetof(engine::import::ObjVertex, engine::import::ObjVertex::normal))
            });

    // attachment formats
    std::vector<ImageAttachmentInfo> color_depth_attachments = {
        { ImageFormat::RGBA8_UNORM, TextureUsage::COLOR_ATTACHMENT | TextureUsage::SAMPLED_IMAGE },
        { ImageFormat::D32_FLOAT,   TextureUsage::DEPTH_ATTACHMENT }
    };

    // configs
    PipelineConfig mesh_cfg = PipelineConfig{}
        .set_depth_test(true)
        .set_depth_write(true)
        .set_blending(false)
        .set_cull_mode(CullMode::BACK)
        .set_push_constant(sizeof(glm::mat4), ShaderStageFlags::VERTEX);

    PipelineConfig outline_cfg = PipelineConfig{}
        .set_depth_test(true)
        .set_depth_write(false)
        .set_blending(true)
        .set_cull_mode(CullMode::FRONT)
        .set_push_constant(sizeof(glm::mat4), ShaderStageFlags::VERTEX);

    PipelineConfig gizmo_cfg = PipelineConfig{}
        .set_depth_test(true)
        .set_depth_write(false)
        .set_blending(true)
        .set_cull_mode(CullMode::NONE)
        .set_polygon_mode(PolygonMode::LINE)
        .set_push_constant(sizeof(glm::mat4), ShaderStageFlags::VERTEX | ShaderStageFlags::FRAGMENT);

    // register pipelines
    _named_pipelines["mesh"] = _pipeline_cache->register_pipeline(
        *_shader_cache->get(_named_shaders["mesh_vert"]),
        *_shader_cache->get(_named_shaders["mesh_frag"]),
        *_named_descriptor_layouts["per_object_ubo"],
        position_normal_binding,
        color_depth_attachments,
        mesh_cfg
    );

    _named_pipelines["outline"] = _pipeline_cache->register_pipeline(
        *_shader_cache->get(_named_shaders["mesh_outline_vert"]),
        *_shader_cache->get(_named_shaders["mesh_outline_frag"]),
        *_named_descriptor_layouts["per_object_ubo"],
        position_normal_binding,
        color_depth_attachments,
        outline_cfg
    );

    _named_pipelines["gizmo"] = _pipeline_cache->register_pipeline(
        *_shader_cache->get(_named_shaders["gizmo_vert"]),
        *_shader_cache->get(_named_shaders["gizmo_frag"]),
        *_named_descriptor_layouts["per_object_ubo"],
        position_binding,
        color_depth_attachments,
        gizmo_cfg
    );
}

std::unordered_map<std::string, engine::core::renderer::cache::MeshCacheId> EditorRenderer::register_default_meshes() {
    engine::import::ObjModel sphere_model = engine::import::ReadObj("res/sphere.obj");
    _named_meshes["sphere"] = _mesh_cache->register_mesh(sphere_model);

    engine::import::ObjModel cube_model = engine::import::ReadObj("res/cube.obj");
    _named_meshes["cube"] = _mesh_cache->register_mesh(cube_model);

    return _named_meshes;
}

std::unordered_map<std::string, engine::core::renderer::cache::MaterialCacheId> EditorRenderer::register_default_materials() {
    {
        const graphics::Pipeline* pipeline = _pipeline_cache->get(_named_pipelines["unlit"]);
        const engine::core::graphics::DescriptorLayoutDescription layout_desc = engine::core::graphics::DescriptorLayoutDescription{}
            .add_binding(
                engine::core::graphics::DescriptorLayoutBinding{}
                    .set_binding(0)
                    .set_type(DescriptorType::UNIFORM_BUFFER)
                    .set_visibility(ShaderStageFlags::VERTEX)
            );

        size_t ubo_size = sizeof(glm::mat4) * 2;
        _named_materials["unlit"] = _material_cache->register_material(*pipeline, layout_desc, ubo_size);
    }
    return _named_shaders;
}

} // namespace editor::renderer
