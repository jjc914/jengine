#include "app.hpp"

#include "graphics/vertex_layout.hpp"
#include "graphics/vulkan/vk_instance.hpp"
#include "window/glfw_window.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <wk/ext/glfw/glfw_internal.hpp>

#include <memory>
#include <chrono>

namespace core {

App::App() {}

App::~App() {}

int App::run() {
    uint32_t width = 900;
    uint32_t height = 600;

    if (!glfwInit()) {
        throw std::runtime_error("failed to initialize glfw");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    _instance = std::make_unique<graphics::vulkan::VulkanInstance>();
    _window = std::make_unique<window::glfw::GlfwWindow>(_instance->native_handle(), "jengine", width, height);
    _surface = wk::ext::glfw::Surface(
        static_cast<VkInstance>(_instance->native_handle()),
        static_cast<GLFWwindow*>(_window->native_handle())
    );
    _device = _instance->create_device(static_cast<void*>(&_surface));

    _renderer = _device->create_renderer(static_cast<void*>(&_surface), width, height);

    _geometry = _device->create_mesh_buffer(
        _VERTICES.data(), sizeof(Vertex), _VERTICES.size(),
        _INDICES.data(), sizeof(uint32_t), _INDICES.size()
    );

    graphics::VertexLayout layout;
    layout.bindings = {
        { 0, sizeof(Vertex), graphics::VertexBinding::InputRate::VERTEX }
    };
    layout.attributes = {
        { 0, 0, graphics::VertexFormat::RGB32_FLOAT, offsetof(Vertex, position) },
        { 1, 0, graphics::VertexFormat::RGB32_FLOAT, offsetof(Vertex, color) }
    };

    _pipeline = _renderer->create_pipeline(layout);
    _material = _pipeline->create_material(sizeof(UniformBufferObject));

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
            _renderer->resize(width, height);
            is_dirty = false;
            continue;
        }

        void* cb;
        if (!(cb = _renderer->begin_frame())) {
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

        _pipeline->bind(cb);
        _geometry->bind(cb);
        _material->bind(cb);

        _renderer->submit_draws(_INDICES.size());
        _renderer->end_frame();
    }
    _device->wait_idle();

    return 0;
}

}