#include "app.hpp"

#include "engine/drivers/vulkan/vulkan_instance.hpp"
#include "engine/drivers/vulkan/vulkan_viewport.hpp"
#include "engine/drivers/vulkan/vulkan_texture_render_target.hpp"
#include "engine/drivers/glfw/glfw_window.hpp"
#include "engine/core/graphics/vertex_types.hpp"
#include "engine/core/graphics/image_types.hpp"

#include "engine/core/debug/logger.hpp"
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
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    // vulkan instance
    _instance = std::make_unique<engine::drivers::vulkan::VulkanInstance>();

    // window
    _window = std::make_unique<engine::drivers::glfw::GlfwWindow>(_instance->native_instance(), "jengine", width, height);
    width = _window->width();
    height = _window->height();

    // device
    _device = _instance->create_device(*_window);

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
            .set_format(_device->present_format())
            .set_usage(engine::core::graphics::ImageUsage::PRESENT),
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(_device->depth_format())
            .set_usage(engine::core::graphics::ImageUsage::DEPTH)
    };

    std::vector<engine::core::graphics::ImageAttachmentInfo> editor_camera_attachments = {
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(engine::core::graphics::ImageFormat::RGBA8_UNORM)
            .set_usage(engine::core::graphics::ImageUsage::COLOR | engine::core::graphics::ImageUsage::SAMPLING),
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(_device->depth_format())
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
    for (uint32_t i = 0; i < _editor_camera_target->frame_count(); ++i) {
        editor_camera_preview_textures.emplace_back(
            gui_layer.register_texture(static_cast<VkImageView>(_editor_camera_target->native_frame_image_view(i)))
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

            for (uint32_t i = 0; i < _editor_camera_target->frame_count(); ++i) {
                gui_layer.unregister_texture(editor_camera_preview_textures[i]);
            }
            editor_camera_preview_textures.clear();
            for (uint32_t i = 0; i < _editor_camera_target->frame_count(); ++i) {
                editor_camera_preview_textures.emplace_back(
                    gui_layer.register_texture(static_cast<VkImageView>(_editor_camera_target->native_frame_image_view(i)))
                );
            }

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

        // Begin ImGui frame
        gui_layer.begin_frame();

        // --- Menu bar ---
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene")) {}
                if (ImGui::MenuItem("Open...")) {}
                if (ImGui::MenuItem("Save")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    // TODO: exit logic
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                // toggle visibility later
                ImGui::MenuItem("Inspector", nullptr, true);
                ImGui::MenuItem("Debug Window", nullptr, true);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // --- Layout ---
        ImGui::SetNextWindowPos(ImVec2(0, 20)); // below the menu bar
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("EditorLayout", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoBackground
        );

        ImGui::Columns(3, "EditorColumns");

        // -------- Left Column --------
        {
            ImGui::BeginChild("LeftPanel", ImVec2(0, 0), true);
            ImGui::Text("Hierarchy");
            ImGui::Separator();
            ImGui::Text("Entity 1");
            ImGui::Text("Entity 2");
            ImGui::Text("Entity 3");

            ImGui::Separator();
            ImGui::Text("Debug Log");
            ImGui::TextWrapped("Everything looks good so far!");
            ImGui::EndChild();
        }
        ImGui::NextColumn();

        // -------- Center Column --------
        {
            ImGui::BeginChild("ViewportPanel", ImVec2(0, 0), true);
            // (Optional) Label
            ImGui::Text("Viewport");

            uint32_t cur_index = _editor_camera_target->frame_index();
            cur_index = std::min<uint32_t>(
                cur_index, static_cast<uint32_t>(editor_camera_preview_textures.size() - 1));

            ImVec2 avail = ImGui::GetContentRegionAvail();

            // --- Maintain aspect ratio ---
            float tex_width  = static_cast<float>(_editor_camera_target->width());
            float tex_height = static_cast<float>(_editor_camera_target->height());
            float tex_aspect = tex_width / tex_height;
            float panel_aspect = avail.x / avail.y;

            ImVec2 image_size;
            if (panel_aspect > tex_aspect) {
                // panel wider than texture — fit by height
                image_size.y = avail.y;
                image_size.x = avail.y * tex_aspect;
            } else {
                // panel taller than texture — fit by width
                image_size.x = avail.x;
                image_size.y = avail.x / tex_aspect;
            }

            // --- Center the image in the panel ---
            ImVec2 cursor = ImGui::GetCursorPos();
            ImVec2 offset = ImVec2(
                (avail.x - image_size.x) * 0.5f,
                (avail.y - image_size.y) * 0.5f
            );
            ImGui::SetCursorPos(ImVec2(cursor.x + offset.x, cursor.y + offset.y));

            // --- Draw the render target (flipped vertically for Vulkan) ---
            ImGui::Image(
                editor_camera_preview_textures[cur_index],
                image_size,
                ImVec2(0, 1),  // top-left UV
                ImVec2(1, 0)   // bottom-right UV
            );
            ImGui::EndChild();
        }
        ImGui::NextColumn();

        // -------- Right Column --------
        {
            ImGui::BeginChild("InspectorPanel", ImVec2(0, 0), true);
            ImGui::Text("Inspector");
            ImGui::Separator();
            ImGui::Text("Selected Entity:");
            ImGui::Text("Entity 1");
            ImGui::Separator();
            ImGui::Text("Transform");
            // ImGui::DragFloat3("Position", pos, 0.1f);
            // ImGui::DragFloat3("Rotation", rot, 0.1f);
            // ImGui::DragFloat3("Scale", scale, 0.1f);
            ImGui::EndChild();
        }

        ImGui::Columns(1);
        ImGui::End(); // EditorLayout

        // End ImGui frame
        gui_layer.end_frame(present_cb);
        _viewport->end_frame();

    }

    _device->wait_idle();
    return 0;
}

} // namespace editor
