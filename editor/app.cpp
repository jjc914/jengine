#include "app.hpp"

#include "engine/drivers/vulkan/vulkan_instance.hpp"
#include "engine/drivers/vulkan/vulkan_viewport.hpp"
#include "engine/drivers/vulkan/vulkan_texture_render_target.hpp"
#include "engine/drivers/glfw/glfw_window.hpp"

#include "engine/core/debug/logger.hpp"
#include "engine/core/graphics/vertex_types.hpp"
#include "engine/core/graphics/image_types.hpp"
#include "editor/gui/imgui_layer.hpp"

#include <imgui.h>
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <memory>

namespace editor {

App::App() {}
App::~App() {}

int App::run() {
    uint32_t width = 900;
    uint32_t height = 600;

    // init glfw
    if (!glfwInit()) {
        engine::core::debug::Logger::get_singleton().fatal("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // vulkan instance
    _instance = std::make_unique<engine::drivers::vulkan::VulkanInstance>();

    // window
    _window = std::make_unique<engine::drivers::glfw::GlfwWindow>(
        _instance->native_instance(), "jengine", width, height);

    // device
    _device = _instance->create_device(*_window);
    _window->query_support(*_device);

    // shaders
    _vertex_shader = _device->create_shader(engine::core::graphics::ShaderStageFlags::VERTEX, "shaders/triangle.vert.spv");
    _fragment_shader = _device->create_shader(engine::core::graphics::ShaderStageFlags::FRAGMENT, "shaders/triangle.frag.spv");

    // descriptor layout
    engine::core::graphics::DescriptorLayoutDescription layout_description =
        engine::core::graphics::DescriptorLayoutDescription{}
            .add_binding(engine::core::graphics::DescriptorLayoutBinding{}
                .set_binding(0)
                .set_type(engine::core::graphics::DescriptorType::UNIFORM_BUFFER)
                .set_visibility(engine::core::graphics::ShaderStageFlags::VERTEX));

    _descriptor_layout = _device->create_descriptor_set_layout(layout_description);

    // vertex binding
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

    // pipelines
    std::vector<engine::core::graphics::ImageAttachmentInfo> present_attachments = {
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(_window->color_format())
            .set_usage(engine::core::graphics::ImageUsage::PRESENT),
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(_window->depth_format())
            .set_usage(engine::core::graphics::ImageUsage::DEPTH)
    };

    std::vector<engine::core::graphics::ImageAttachmentInfo> editor_camera_attachments = {
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(engine::core::graphics::ImageFormat::RGBA8_UNORM)
            .set_usage(engine::core::graphics::ImageUsage::COLOR | engine::core::graphics::ImageUsage::SAMPLING),
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(_window->depth_format())
            .set_usage(engine::core::graphics::ImageUsage::DEPTH)
    };

    _present_pipeline = _device->create_pipeline(
        *_vertex_shader, *_fragment_shader,
        *_descriptor_layout, vertex_binding, present_attachments);

    _editor_camera_pipeline = _device->create_pipeline(
        *_vertex_shader, *_fragment_shader,
        *_descriptor_layout, vertex_binding, editor_camera_attachments);

    // gui layer
    ui::ImGuiLayer gui_layer(*_instance, *_device, *_present_pipeline, *_window);

    // render targets
    _viewport = _device->create_viewport(*_window, *_present_pipeline, width, height);

    _editor_camera_target = _device->create_texture_render_target(*_editor_camera_pipeline, width, height);

    // imgui register offscreen textures
    std::vector<ImTextureID> editor_camera_preview_textures;
    for (uint32_t i = 0; i < 1; ++i) {
        editor_camera_preview_textures.push_back(
            gui_layer.register_texture(
                static_cast<VkImageView>(_editor_camera_target->native_frame_image_view(i)))
        );
    }

    // geometry and material
    import::ObjModel mesh = import::ReadObj("res/sphere.obj");
    _geometry = _device->create_mesh_buffer(
        mesh.meshes[0].vertices.data(), sizeof(import::ObjVertex), mesh.meshes[0].vertices.size(),
        mesh.meshes[0].indices.data(), sizeof(uint32_t), mesh.meshes[0].indices.size()
    );

    _material = _editor_camera_pipeline->create_material(*_descriptor_layout, sizeof(UniformBufferObject));

    // main loop
    auto last_time = std::chrono::high_resolution_clock::now();
    float t = 0;
    bool is_dirty = false;

    while (!_window->should_close()) {
        auto current_time = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(current_time - last_time).count();
        last_time = current_time;

        _window->poll();

        if (width != _window->width() || height != _window->height()) {
            width = _window->width();
            height = _window->height();

            if (width == 0 || height == 0) {
                _window->wait_events();
                continue;
            }
            is_dirty = true;
        }

        if (is_dirty) {
            _viewport->resize(width, height);
            _editor_camera_target->resize(width, height);
            is_dirty = false;
            continue;
        }

        // editor camera pass
        void* pre_cb = _editor_camera_target->begin_frame(*_editor_camera_pipeline);
        if (!pre_cb) {
            is_dirty = true;
            continue;
        }

        t += dt;
        if (t > 2 * glm::pi<float>()) t -= 2 * glm::pi<float>();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0.1f, 1.0f, 0.4f));
        ubo.view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f),
            width / (float)height, 0.1f, 100.0f);
        ubo.proj[1][1] *= -1;
        _material->update_uniform_buffer(&ubo);

        _editor_camera_pipeline->bind(pre_cb);
        _geometry->bind(pre_cb);
        _material->bind(pre_cb);
        _editor_camera_target->submit_draws(static_cast<uint32_t>(mesh.meshes[0].indices.size()));
        _editor_camera_target->end_frame();

        // editor gui pass
        void* present_cb = _viewport->begin_frame(*_present_pipeline);
        if (!present_cb) {
            is_dirty = true;
            continue;
        }

        gui_layer.begin_frame();
        ImGui::Begin("Editor");
        ImGui::Text("FPS: %.1f", 1.0f / dt);
        ImGui::Text("Time: %.2f", t);
        ImGui::Text("Window Size: %ux%u", width, height);

        uint32_t cur_index = _editor_camera_target->frame_index();
        cur_index = std::min<uint32_t>(cur_index, static_cast<uint32_t>(editor_camera_preview_textures.size() - 1));

        ImGui::Image(
            editor_camera_preview_textures[cur_index],
            ImVec2(width * 0.6f, height * 0.6f));

        ImGui::End();
        gui_layer.end_frame(present_cb);

        _viewport->end_frame();
    }

    _device->wait_idle();
    return 0;
}

} // namespace editor
