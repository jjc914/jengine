#include "editor_scene_view_renderer.hpp"

#include "editor/components/transform.hpp"
#include "editor/components/mesh_renderer.hpp"
#include "engine/core/debug/logger.hpp"
#include "editor_renderer.hpp"

#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtx/transform.hpp>
#include <algorithm>

namespace editor::renderer {

EditorSceneViewRenderer::EditorSceneViewRenderer(
    EditorRendererContext& context,
    uint32_t width,
    uint32_t height
)
    : _device(context.device),
      _pipeline_cache(context.pipeline_cache),
      _material_cache(context.material_cache),
      _mesh_cache(context.mesh_cache),
      _gui(context.gui),
      _width(width),
      _height(height)
{
    // --- Camera ---
    _camera = std::make_unique<editor::scene::EditorCamera>();
    _camera->resize(width, height);
    _camera->look_at(glm::vec3(0.0f));

    // --- Render targets ---
    _view_target = _device->create_texture_render_target(
        _pipeline_cache.get(context.view_pipeline),
        width,
        height
    );
    _pick_target = _device->create_texture_render_target(
        _pipeline_cache.get(context.pick_pipeline),
        width,
        height
    );

    // --- Sampler ---
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

    // --- ImGui texture registration ---
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

    _view_pipeline_id = context.view_pipeline;
    _pick_pipeline_id = context.pick_pipeline;
    _pick_material_id = context.pick_material;
}

void EditorSceneViewRenderer::register_passes(
    engine::core::scene::Scene& scene,
    engine::core::renderer::FrameGraph& graph
) {
    // scene color pass
    graph.add_pass(
        &_pipeline_cache.get(_view_pipeline_id),
        _view_target.get(),
        [this, &scene](engine::core::renderer::RenderPassContext& ctx, engine::core::renderer::RenderPassId) {
            void* cb = ctx.command_buffer;
            if (!cb) return;

            for (auto [entity, transform, mesh_renderer] :
                 scene.view<components::Transform, components::MeshRenderer>())
            {
                if (!mesh_renderer.visible) continue;

                auto& mesh = _mesh_cache.get(mesh_renderer.mesh_id);
                auto& material = _material_cache.get(mesh_renderer.material_id);

                UniformBufferObject ubo{};
                ubo.model = transform.matrix();
                ubo.view  = _camera->view();
                ubo.proj  = _camera->projection();
                ubo.proj[1][1] *= -1.0f;

                material.update_uniform_buffer(&ubo);
                mesh.bind(cb);
                material.bind(cb);
                _view_target->submit_draws(mesh.index_count());
            }
        }
    );

    // scene pick pass
    const uint32_t readback_index = _pick_target->frame_index();
    graph.add_pass(
        &_pipeline_cache.get(_pick_pipeline_id),
        _pick_target.get(),
        [this, &scene, readback_index](engine::core::renderer::RenderPassContext& ctx, engine::core::renderer::RenderPassId) {
            void* cb = ctx.command_buffer;
            if (!cb) return;

            for (auto [entity, transform, mesh_renderer] :
                scene.view<components::Transform, components::MeshRenderer>())
            {
                if (!mesh_renderer.visible) continue;

                auto& mesh = _mesh_cache.get(mesh_renderer.mesh_id);
                auto& material = _material_cache.get(_pick_material_id);

                UniformBufferObject ubo{};
                ubo.model = transform.matrix();
                ubo.view = _camera->view();
                ubo.proj = _camera->projection();
                ubo.proj[1][1] *= -1.0f;

                material.update_uniform_buffer(&ubo);
                mesh.bind(cb);
                material.bind(cb);

                struct { uint32_t entity_id; } pc{
                    static_cast<uint32_t>(entity & 0xFFFFFFFFu)
                };
                _pick_target->push_constants(cb, ctx.pipeline_layout, &pc, sizeof(pc),
                    engine::core::graphics::ShaderStageFlags::FRAGMENT);

                _pick_target->submit_draws(mesh.index_count());
            }
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
