#include "editor_scene_view_renderer.hpp"

#include "editor/components/transform.hpp"
#include "editor/components/mesh_renderer.hpp"
#include "editor/components/gizmo_renderer.hpp"
#include "engine/core/debug/logger.hpp"
#include "editor_renderer.hpp"

#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <algorithm>

namespace editor::renderer {

EditorSceneViewRenderer::EditorSceneViewRenderer(
    EditorRenderer& editor_renderer,
    uint32_t width,
    uint32_t height)
    : _width(width),
      _height(height),
      _device(editor_renderer.device()),
      _gui(editor_renderer.gui()),
      _pipeline_cache(editor_renderer.pipeline_cache()),
      _mesh_cache(editor_renderer.mesh_cache()),
      _material_cache(editor_renderer.material_cache())
{
    _mesh_normal_pipeline_id = editor_renderer.pipeline_id("mesh_normal");
    _mesh_pick_pipeline_id = editor_renderer.pipeline_id("mesh_pick");
    _mesh_pick_material_id = editor_renderer.material_id("mesh_pick");
    _mesh_outline_pipeline_id = editor_renderer.pipeline_id("mesh_outline");
    _mesh_outline_material_id = editor_renderer.material_id("mesh_outline");
    _cube_id = editor_renderer.mesh_id("cube");
    _skybox_pipeline_id = editor_renderer.pipeline_id("skybox");
    _skybox_material_id = editor_renderer.material_id("skybox");
    _gizmo_pipeline_id = editor_renderer.pipeline_id("gizmo");
    _gizmo_material_id = editor_renderer.material_id("gizmo");

    // camera
    _camera = std::make_unique<editor::scene::EditorCamera>();
    _camera->resize(width, height);
    _camera->look_at(glm::vec3(0.0f));

    // render targets
    _view_target = _device->create_texture_render_target(
        _pipeline_cache.get(_mesh_normal_pipeline_id),
        width,
        height
    );
    _pick_target = _device->create_texture_render_target(
        _pipeline_cache.get(_mesh_pick_pipeline_id),
        width,
        height
    );

    // sampler
    _sampler = wk::Sampler(
        static_cast<VkDevice>(_device->native_device()),
        wk::SamplerCreateInfo{}
            .set_mag_filter(VK_FILTER_LINEAR)
            .set_min_filter(VK_FILTER_LINEAR)
            .set_mipmap_mode(VK_SAMPLER_MIPMAP_MODE_LINEAR)
            .set_address_mode_u(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .set_address_mode_v(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .set_address_mode_w(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
            .set_anisotropy_enable(false)
            .set_min_lod(0.0f)
            .set_max_lod(0.0f)
            .set_border_color(VK_BORDER_COLOR_INT_OPAQUE_BLACK)
            .to_vk()
    );

    // imgui texture registration
    _imgui_texture_ids.clear();
    for (uint32_t i = 0; i < _view_target->frame_count(); ++i) {
        ImTextureID id = reinterpret_cast<ImTextureID>(
            ImGui_ImplVulkan_AddTexture(
                _sampler.handle(),
                static_cast<VkImageView>(_view_target->native_frame_image_view(i)),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            )
        );
        _imgui_texture_ids.emplace_back(id);
    }
}

void EditorSceneViewRenderer::register_passes(
    engine::core::scene::Scene& scene,
    engine::core::renderer::FrameGraph& graph
) {
    // skybox pass
    graph.add_pass(
        &_pipeline_cache.get(_skybox_pipeline_id),
        _view_target.get(),
        [this](engine::core::renderer::RenderPassContext& ctx, engine::core::renderer::RenderPassId) {
            ctx.command_buffer = ctx.target->begin_frame(*ctx.pipeline);
            ENGINE_ASSERT(ctx.command_buffer, "FrameGraph expects a valid command buffer");

            ctx.pipeline->bind(ctx.command_buffer);

            engine::core::graphics::Material& material = _material_cache.get(_skybox_material_id);
            engine::core::graphics::MeshBuffer& mesh = _mesh_cache.get(_cube_id);

            UniformBufferObject ubo;
            ubo.view = glm::mat4(glm::mat3(_camera->view()));
            ubo.proj = _camera->projection();
            ubo.proj[1][1] *= -1.0f;

            material.update_uniform_buffer(&ubo);

            mesh.bind(ctx.command_buffer);
            material.bind(ctx.command_buffer);
            _view_target->submit_draws(mesh.index_count());
        }
    );

    // scene color pass
    graph.add_pass(
        &_pipeline_cache.get(_mesh_normal_pipeline_id),
        _view_target.get(),
        [this, &scene](engine::core::renderer::RenderPassContext& ctx, engine::core::renderer::RenderPassId) {
            ENGINE_ASSERT(ctx.command_buffer, "FrameGraph expects a valid command buffer");

            ctx.pipeline->bind(ctx.command_buffer);

            for (auto [entity, transform, mesh_renderer] :
                 scene.view<components::Transform, components::MeshRenderer>())
            {
                if (!mesh_renderer.visible) continue;

                engine::core::graphics::MeshBuffer& mesh = _mesh_cache.get(mesh_renderer.mesh_id);
                engine::core::graphics::Material& material = _material_cache.get(mesh_renderer.material_id);

                UniformBufferObject ubo;
                ubo.view = _camera->view();
                ubo.proj = _camera->projection();
                ubo.proj[1][1] *= -1.0f;

                PushConstants pc;
                pc.model = transform.matrix();

                material.update_uniform_buffer(&ubo);
                _view_target->push_constants(ctx.command_buffer, ctx.pipeline->native_pipeline_layout(), &pc, sizeof(pc),
                        engine::core::graphics::ShaderStageFlags::VERTEX);

                mesh.bind(ctx.command_buffer);
                material.bind(ctx.command_buffer);
                _view_target->submit_draws(mesh.index_count());
            }
        }
    );

    // selection outline pass
    graph.add_pass(
        &_pipeline_cache.get(_mesh_outline_pipeline_id),
        _view_target.get(),
        [this, &scene](engine::core::renderer::RenderPassContext& ctx, engine::core::renderer::RenderPassId) {
            ENGINE_ASSERT(ctx.command_buffer, "FrameGraph expects a valid command buffer");

            ctx.pipeline->bind(ctx.command_buffer);
            
            engine::core::graphics::Material& material = _material_cache.get(_mesh_outline_material_id);

            for (auto [entity, transform, mesh_renderer] :
                 scene.view<components::Transform, components::MeshRenderer>())
            {
                if (!mesh_renderer.visible) continue;
                if (_selected_entity != entity) continue;

                engine::core::graphics::MeshBuffer& mesh = _mesh_cache.get(mesh_renderer.mesh_id);

                UniformBufferObject ubo;
                ubo.view = _camera->view();
                ubo.proj = _camera->projection();
                ubo.proj[1][1] *= -1.0f;

                struct {
                    glm::mat4 model;
                    glm::vec4 color;
                    float width;
                } pc{
                    transform.matrix(),
                    glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
                    0.02f
                };

                material.update_uniform_buffer(&ubo);
                _view_target->push_constants(ctx.command_buffer, ctx.pipeline->native_pipeline_layout(), &pc, sizeof(pc),
                        engine::core::graphics::ShaderStageFlags::VERTEX);

                mesh.bind(ctx.command_buffer);
                material.bind(ctx.command_buffer);

                _view_target->submit_draws(mesh.index_count());
            }
        }
    );

    // scene gizmo pass
    graph.add_pass(
        &_pipeline_cache.get(_gizmo_pipeline_id),
        _view_target.get(),
        [this, &scene](engine::core::renderer::RenderPassContext& ctx, engine::core::renderer::RenderPassId) {
            ENGINE_ASSERT(ctx.command_buffer, "FrameGraph expects a valid command buffer");

            ctx.pipeline->bind(ctx.command_buffer);

            engine::core::graphics::Material& material = _material_cache.get(_gizmo_material_id);

            for (auto [entity, transform, gizmo_renderer] :
                 scene.view<components::Transform, components::GizmoRenderer>())
            {
                if (!gizmo_renderer.visible) continue;

                engine::core::graphics::MeshBuffer& mesh = _mesh_cache.get(gizmo_renderer.mesh_id);

                UniformBufferObject ubo;
                ubo.view = _camera->view();
                ubo.proj = _camera->projection();
                ubo.proj[1][1] *= -1.0f;

                struct {
                    glm::mat4 model;
                    glm::vec4 color;
                } pc{
                    transform.matrix(),
                    glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
                };

                material.update_uniform_buffer(&ubo);
                _view_target->push_constants(ctx.command_buffer, ctx.pipeline->native_pipeline_layout(), &pc, sizeof(pc),
                        engine::core::graphics::ShaderStageFlags::VERTEX | engine::core::graphics::ShaderStageFlags::FRAGMENT);

                mesh.bind(ctx.command_buffer);
                material.bind(ctx.command_buffer);
                _view_target->submit_draws(mesh.index_count());
            }
            
            ctx.target->end_frame();
        }
    );

    // scene pick pass
    const uint32_t readback_index = _pick_target->frame_index();
    graph.add_pass(
        &_pipeline_cache.get(_mesh_pick_pipeline_id),
        _pick_target.get(),
        [this, &scene, readback_index](engine::core::renderer::RenderPassContext& ctx, engine::core::renderer::RenderPassId) {
            ctx.command_buffer = ctx.target->begin_frame(*ctx.pipeline);
            ENGINE_ASSERT(ctx.command_buffer, "FrameGraph expects a valid command buffer");
            
            ctx.pipeline->bind(ctx.command_buffer);
            
            engine::core::graphics::Material& material = _material_cache.get(_mesh_pick_material_id);

            for (auto [entity, transform, mesh_renderer] :
                scene.view<components::Transform, components::MeshRenderer>())
            {
                if (!mesh_renderer.visible) continue;

                engine::core::graphics::MeshBuffer& mesh = _mesh_cache.get(mesh_renderer.mesh_id);

                UniformBufferObject ubo;
                ubo.view = _camera->view();
                ubo.proj = _camera->projection();
                ubo.proj[1][1] *= -1.0f;

                struct {
                    glm::mat4 model;
                    uint32_t entity_id;
                } pc{
                    transform.matrix(),
                    static_cast<uint32_t>(entity & 0xFFFFFFFFu)
                };

                material.update_uniform_buffer(&ubo);
                _pick_target->push_constants(ctx.command_buffer, ctx.pipeline->native_pipeline_layout(), &pc, sizeof(pc),
                        engine::core::graphics::ShaderStageFlags::VERTEX | engine::core::graphics::ShaderStageFlags::FRAGMENT);

                mesh.bind(ctx.command_buffer);
                material.bind(ctx.command_buffer);
                _pick_target->submit_draws(mesh.index_count());
            }

            for (auto [entity, transform, gizmo_renderer] :
                scene.view<components::Transform, components::GizmoRenderer>())
            {
                if (!gizmo_renderer.visible) continue;

                engine::core::graphics::MeshBuffer& mesh = _mesh_cache.get(gizmo_renderer.mesh_id);

                UniformBufferObject ubo;
                ubo.view = _camera->view();
                ubo.proj = _camera->projection();
                ubo.proj[1][1] *= -1.0f;

                struct {
                    glm::mat4 model;
                    uint32_t entity_id;
                } pc{
                    transform.matrix(),
                    static_cast<uint32_t>(entity & 0xFFFFFFFFu)
                };

                material.update_uniform_buffer(&ubo);
                _pick_target->push_constants(ctx.command_buffer, ctx.pipeline->native_pipeline_layout(), &pc, sizeof(pc),
                        engine::core::graphics::ShaderStageFlags::VERTEX | engine::core::graphics::ShaderStageFlags::FRAGMENT);

                mesh.bind(ctx.command_buffer);
                material.bind(ctx.command_buffer);
                _pick_target->submit_draws(mesh.index_count());
            }

            ctx.target->end_frame();
        }
    );

    // handle resize
    if (_pending_resize) {
        _device->wait_idle();

        _width  = _pending_resize->x;
        _height = _pending_resize->y;

        _camera->resize(_width, _height);
        _view_target->resize(_width, _height);
        _pick_target->resize(_width, _height);

        for (uint32_t i = 0; i < _view_target->frame_count(); ++i)
            ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(_imgui_texture_ids[i]));

        _imgui_texture_ids.clear();
        for (uint32_t i = 0; i < _view_target->frame_count(); ++i) {
            ImTextureID id = reinterpret_cast<ImTextureID>(
                ImGui_ImplVulkan_AddTexture(
                    _sampler.handle(),
                    static_cast<VkImageView>(_view_target->native_frame_image_view(i)),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                )
            );
            _imgui_texture_ids.emplace_back(id);
        }

        _pending_resize.reset();
    }
}

void EditorSceneViewRenderer::handle_picking(uint32_t frame_index, glm::vec2 view_pos, glm::vec2 view_size) {
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

    _view_pos = view_pos;
    _view_size = view_size;

    ImVec2 mouse = ImGui::GetMousePos();

    if (mouse.x >= _view_pos.x && mouse.x <= _view_pos.x + _view_size.x &&
        mouse.y >= _view_pos.y && mouse.y <= _view_pos.y + _view_size.y)
    {
        int px = static_cast<int>((mouse.x - _view_pos.x) * (static_cast<float>(_width) / _view_size.x));
        int py = static_cast<int>(((_view_pos.y + _view_size.y) - mouse.y) * (static_cast<float>(_height) / _view_size.y));

        px = std::clamp(px, 0, static_cast<int>(_width - 1));
        py = std::clamp(py, 0, static_cast<int>(_height - 1));

        std::vector<uint32_t> pixels = _pick_target->copy_color_to_cpu(frame_index);
        uint32_t picked_id = pixels[py * _width + px];

        _selected_entity = static_cast<engine::core::scene::Entity>(picked_id);
    }
}

void EditorSceneViewRenderer::resize(uint32_t width, uint32_t height) {
    if (width != _width || height != _height) {
        _pending_resize = glm::vec2(width, height);
    }
}

} // namespace editor::renderer
