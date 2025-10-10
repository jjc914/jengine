#include "app.hpp"

#include "engine/drivers/vulkan/vulkan_instance.hpp"
#include "engine/drivers/glfw/glfw_window.hpp"

#include "engine/core/debug/logger.hpp"
#include "engine/core/graphics/vertex_types.hpp"
#include "engine/core/graphics/image_types.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <wk/ext/glfw/glfw_internal.hpp>

#include <memory>
#include <chrono>

namespace editor {

App::App() {}

App::~App() {}

int App::run() {
    uint32_t width = 900;
    uint32_t height = 600;

    if (!glfwInit()) {
        engine::core::debug::Logger::get_singleton().fatal("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // graphics instance
    _instance = std::make_unique<engine::drivers::vulkan::VulkanInstance>();

    // window
    _window = std::make_unique<engine::drivers::glfw::GlfwWindow>(_instance->native_handle(), "jengine", width, height);

    // device
    _device = _instance->create_device(*_window);

    // choose surface formats
    _window->query_support(*_device);
    
    // shaders
    _vertex_shader = _device->create_shader(engine::core::graphics::ShaderStageFlags::VERTEX, "shaders/triangle.vert.spv");
    _fragment_shader = _device->create_shader(engine::core::graphics::ShaderStageFlags::FRAGMENT, "shaders/triangle.frag.spv");

    // descriptor set layout
    engine::core::graphics::DescriptorLayoutDescription layout_description = engine::core::graphics::DescriptorLayoutDescription{}
        .add_binding(engine::core::graphics::DescriptorLayoutBinding{}
            .set_binding(0)
            .set_type(engine::core::graphics::DescriptorType::UNIFORM_BUFFER)
            .set_visibility(engine::core::graphics::ShaderStageFlags::VERTEX));

    _descriptor_layout = _device->create_descriptor_set_layout(layout_description);

    // create viewport and display pipeline
    engine::core::graphics::VertexBinding vertex_binding = engine::core::graphics::VertexBinding{}
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
    
    std::vector<engine::core::graphics::ImageAttachmentInfo> attachment_info = {
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(_window->color_format())
            .set_usage(engine::core::graphics::ImageUsage::PRESENT),
        engine::core::graphics::ImageAttachmentInfo{}
            .set_format(_window->depth_format())
            .set_usage(engine::core::graphics::ImageUsage::DEPTH)
    };

    _present_pipeline = _device->create_pipeline(
        *_vertex_shader, *_fragment_shader,
        *_descriptor_layout, vertex_binding, attachment_info
    );
    _viewport = _device->create_viewport(*_window, *_present_pipeline, width, height);

    // create geometry and material
    import::ObjModel mesh = import::ReadObj("res/sphere.obj");
    _geometry = _device->create_mesh_buffer(
        mesh.meshes[0].vertices.data(), sizeof(import::ObjVertex), mesh.meshes[0].vertices.size(),
        mesh.meshes[0].indices.data(), sizeof(uint32_t), mesh.meshes[0].indices.size()
    );
    _material = _present_pipeline->create_material(*_descriptor_layout, sizeof(UniformBufferObject));

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
            is_dirty = false;
            continue;
            // _renderer->resize(width, height);
            // is_dirty = false;
            // continue;
        }

        void* cb;
        if (!(cb = _viewport->begin_frame(*_present_pipeline))) {
            is_dirty = true;
            continue;
        }

        // update scene
        t += dt;
        if (t > 2 * glm::pi<float>()) t -= 2 * glm::pi<float>();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0.1f, 1.0f, 0.4f));
        ubo.view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        ubo.proj = glm::perspective(glm::radians(45.0f), width / (float)height, 0.1f, 100.0f);
        ubo.proj[1][1] *= -1; // flip y for vulkan
        _material->update_uniform_buffer(static_cast<void*>(&ubo));

        _present_pipeline->bind(cb);
        _geometry->bind(cb);
        _material->bind(cb);
        _viewport->submit_draws(mesh.meshes[0].indices.size());

        _viewport->end_frame();
    }
    _device->wait_idle();

    return 0;
}

} // namespace editor