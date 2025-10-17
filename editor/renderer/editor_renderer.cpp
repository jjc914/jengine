#include "editor_renderer.hpp"

#include "engine/drivers/vulkan/vulkan_instance.hpp"

#include "engine/import/mesh.hpp"

#include "editor/components/transform.hpp"
#include "editor/components/mesh_renderer.hpp"

#include "editor/gui/imgui_layer.hpp"

#include <glm/gtx/transform.hpp>

namespace editor::renderer {

EditorRenderer::EditorRenderer(const engine::core::window::Window& main_window) {
    _width = main_window.width();
    _height = main_window.height();

    // instance
    _instance = std::make_unique<engine::drivers::vulkan::VulkanInstance>();
    _device = _instance->create_device(main_window);

    // mesh cache
    _mesh_cache = std::make_unique<engine::core::renderer::cache::MeshCache>(*_device);
    _mesh_cache_entries.clear();

    // default meshes
    import::ObjModel model = import::ReadObj("res/sphere.obj");
    engine::core::renderer::cache::MeshCacheId sphere_mesh =_mesh_cache->register_mesh(model);
    _mesh_cache_entries["sphere"] = sphere_mesh;

    // shader cache
    _shader_cache = std::make_unique<engine::core::renderer::cache::ShaderCache>(*_device);
    _shader_cache_entries.clear();

    // default shaders
    engine::core::renderer::cache::ShaderCacheId editor_view_color_vert_shader = _shader_cache->register_shader(
        engine::core::graphics::ShaderStageFlags::VERTEX,
        "shaders/editor_view_color.vert.spv"
    );
    _shader_cache_entries["editor_view_color_vert"] = editor_view_color_vert_shader;

    engine::core::renderer::cache::ShaderCacheId editor_view_color_frag_shader = _shader_cache->register_shader(
        engine::core::graphics::ShaderStageFlags::FRAGMENT,
        "shaders/editor_view_color.frag.spv"
    );
    _shader_cache_entries["editor_view_color_frag"] = editor_view_color_frag_shader;

    engine::core::renderer::cache::ShaderCacheId editor_present_vert_shader = _shader_cache->register_shader(
        engine::core::graphics::ShaderStageFlags::VERTEX,
        "shaders/editor_present.vert.spv"
    );
    _shader_cache_entries["editor_present_vert"] = editor_present_vert_shader;

    engine::core::renderer::cache::ShaderCacheId editor_present_frag_shader = _shader_cache->register_shader(
        engine::core::graphics::ShaderStageFlags::FRAGMENT,
        "shaders/editor_present.frag.spv"
    );
    _shader_cache_entries["editor_present_frag"] = editor_present_frag_shader;

    // material cache
    _material_cache = std::make_unique<engine::core::renderer::cache::MaterialCache>(*_device);
    _material_cache_entries.clear();

    // pipeline cache
    _pipeline_cache = std::make_unique<engine::core::renderer::cache::PipelineCache>(*_device);
    _pipeline_cache_entries.clear();

    // scene geometry vertex binding
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

    // offscreen editor view attachments
    std::vector<engine::core::graphics::ImageAttachmentInfo> view_attachments = {
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(engine::core::graphics::ImageFormat::RGBA8_UNORM)
            .set_usage(engine::core::graphics::ImageUsage::COLOR | engine::core::graphics::ImageUsage::SAMPLING),
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(engine::core::graphics::ImageFormat::D32_FLOAT)
            .set_usage(engine::core::graphics::ImageUsage::DEPTH)
    };

    // present attachments
    std::vector<engine::core::graphics::ImageAttachmentInfo> present_attachments = {
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(_device->present_format())
            .set_usage(engine::core::graphics::ImageUsage::PRESENT),
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(_device->depth_format())
            .set_usage(engine::core::graphics::ImageUsage::DEPTH)
    };

    // scene descriptor layout
    engine::core::graphics::DescriptorLayoutDescription layout_desc =
        engine::core::graphics::DescriptorLayoutDescription{}
            .add_binding(engine::core::graphics::DescriptorLayoutBinding{}
                .set_binding(0)
                .set_type(engine::core::graphics::DescriptorType::UNIFORM_BUFFER)
                .set_visibility(engine::core::graphics::ShaderStageFlags::VERTEX));
    
    // present descriptor layout
    engine::core::graphics::DescriptorLayoutDescription present_layout_desc =
        engine::core::graphics::DescriptorLayoutDescription{}
            .add_binding(engine::core::graphics::DescriptorLayoutBinding{}
                .set_binding(0)
                .set_type(engine::core::graphics::DescriptorType::COMBINED_IMAGE_SAMPLER)
                .set_visibility(engine::core::graphics::ShaderStageFlags::FRAGMENT));

    // offscreen editor view pipeline
    engine::core::renderer::cache::PipelineCacheId editor_view_color_pipeline = _pipeline_cache->register_pipeline(
        _shader_cache->get(editor_view_color_vert_shader),
        _shader_cache->get(editor_view_color_frag_shader),
        _material_cache->get_or_create_layout(layout_desc),
        vertex_binding,
        view_attachments
    );
    _pipeline_cache_entries["editor_view_color"] = editor_view_color_pipeline;

    // present pipeline
    engine::core::renderer::cache::PipelineCacheId editor_present_pipeline = _pipeline_cache->register_pipeline(
        _shader_cache->get(editor_present_vert_shader),
        _shader_cache->get(editor_present_frag_shader),
        _material_cache->get_or_create_layout(present_layout_desc),
        engine::core::graphics::VertexBinding{},
        present_attachments
    );
    _pipeline_cache_entries["editor_present"] = editor_present_pipeline;

    // default material
    engine::core::renderer::cache::MaterialCacheId editor_camera_default_material = _material_cache->register_material(
        _pipeline_cache->get(editor_view_color_pipeline),
        layout_desc,
        sizeof(UniformBufferObject)
    );
    _material_cache_entries["editor_view_default"] = editor_camera_default_material;
    
    // render targets
    _editor_view_target = _device->create_texture_render_target(_pipeline_cache->get(pipeline_id("editor_view_color")), _width, _height);
    _main_viewport = _device->create_viewport(main_window, _pipeline_cache->get(pipeline_id("editor_present")), _width, _height);

    // gui layer
    _gui_layer = std::make_unique<gui::ImGuiLayer>(*_instance, *_device, _pipeline_cache->get(pipeline_id("editor_present")), main_window);

    // imgui register offscreen textures
    _editor_camera_preview_textures.clear();
    for (uint32_t i = 0; i < _editor_view_target->frame_count(); ++i) {
        _editor_camera_preview_textures.emplace_back(
            _gui_layer->register_texture(static_cast<VkImageView>(_editor_view_target->native_frame_image_view(i)))
        );
    }
}

void EditorRenderer::resize(uint32_t width, uint32_t height) {
    _width = width;
    _height = height;

    rebuild();
}

void EditorRenderer::render_scene(engine::core::scene::Scene& scene) {
    _frame_graph.clear();

    // scene pass
    _scene_pass_id = _frame_graph.add_pass(
        &_pipeline_cache->get(pipeline_id("editor_view_color")),
        _editor_view_target.get(),
        [this, &scene](engine::core::renderer::RenderPassContext& ctx, engine::core::renderer::RenderPassId) {
            ENGINE_ASSERT(ctx.command_buffer, "FrameGraph expects a valid command buffer.");
            void* cb = ctx.command_buffer;

            for (auto [entity, transform, mesh_renderer] : scene.view<components::Transform, components::MeshRenderer>()) {
                 if (!mesh_renderer.visible) continue;

                engine::core::graphics::MeshBuffer& mesh = _mesh_cache->get(mesh_renderer.mesh_id);
                engine::core::graphics::Material& material = _material_cache->get(mesh_renderer.material_id);

                UniformBufferObject ubo{};
                ubo.model = transform.matrix();
                ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f),
                                       glm::vec3(0.0f, 0.0f, 0.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
                ubo.proj = glm::perspective(glm::radians(45.0f), _width / (float)_height, 0.1f, 100.0f);
                ubo.proj[1][1] *= -1.0f;

                material.update_uniform_buffer(&ubo);

                mesh.bind(cb);
                material.bind(cb);
                _editor_view_target->submit_draws(mesh.index_count());
            }
        }
    );

    // present pass
    _present_pass_id = _frame_graph.add_pass(
        &_pipeline_cache->get(pipeline_id("editor_present")),
        _main_viewport.get(),
        [this](engine::core::renderer::RenderPassContext& ctx, engine::core::renderer::RenderPassId) {
            ENGINE_ASSERT(ctx.command_buffer, "FrameGraph expects a valid command buffer.");
            void* cb = ctx.command_buffer;
            _gui_layer->begin_frame();

            // scene view panel
            ImGui::Begin("Scene View");

            // pick correct imgui target
            uint32_t cur = _editor_view_target->frame_index();
            if (!_editor_camera_preview_textures.empty()) {
                cur = std::min<uint32_t>(cur, (uint32_t)_editor_camera_preview_textures.size() - 1);

                // fit to available region
                ImVec2 avail = ImGui::GetContentRegionAvail();
                float tex_w = (float)_editor_view_target->width();
                float tex_h = (float)_editor_view_target->height();
                float tex_aspect = tex_w / tex_h;
                float panel_aspect = (avail.y > 0.f) ? (avail.x / avail.y) : tex_aspect;

                ImVec2 size;
                if (panel_aspect > tex_aspect) {
                    size.y = avail.y;
                    size.x = avail.y * tex_aspect;
                } else {
                    size.x = avail.x;
                    size.y = (tex_aspect > 0.f) ? (avail.x / tex_aspect) : avail.y;
                }

                // center image
                ImVec2 cursor = ImGui::GetCursorPos();
                ImVec2 offset = ImVec2((avail.x - size.x) * 0.5f, (avail.y - size.y) * 0.5f);
                ImGui::SetCursorPos(ImVec2(cursor.x + offset.x, cursor.y + offset.y));

                ImGui::Image(_editor_camera_preview_textures[cur], size, ImVec2(0,1), ImVec2(1,0));
            } else {
                ImGui::TextUnformatted("No editor view textures registered.");
            }

            ImGui::End();

            _gui_layer->end_frame(ctx.command_buffer);
        }
    );

    engine::core::renderer::RenderPassContext ctx{};
    _frame_graph.execute(ctx);
}

void EditorRenderer::rebuild() {
    _main_viewport->resize(_width, _height);
    _editor_view_target->resize(_width, _height);

    for (uint32_t i = 0; i < _editor_view_target->frame_count(); ++i) {
        _gui_layer->unregister_texture(_editor_camera_preview_textures[i]);
    }
    _editor_camera_preview_textures.clear();
    for (uint32_t i = 0; i < _editor_view_target->frame_count(); ++i) {
        _editor_camera_preview_textures.emplace_back(
            _gui_layer->register_texture(static_cast<VkImageView>(_editor_view_target->native_frame_image_view(i)))
        );
    }
}

} // namespace editor::renderer