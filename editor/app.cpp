#include "app.hpp"

#include "engine/drivers/vulkan/vulkan_instance.hpp"
#include "engine/drivers/vulkan/vulkan_viewport.hpp"
#include "engine/drivers/vulkan/vulkan_texture_render_target.hpp"
#include "engine/drivers/glfw/glfw_window.hpp"

#include "engine/core/graphics/vertex_types.hpp"
#include "engine/core/graphics/image_types.hpp"

#include "engine/core/debug/logger.hpp"

#include "editor/components/transform.hpp"
#include "editor/components/mesh_renderer.hpp"

#include <imgui.h>
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <memory>

namespace editor {

App::App() {}
App::~App() {}

int App::run() {
    uint32_t width = 1280;
    uint32_t height = 720;

    // init glfw
    if (!glfwInit()) {
        engine::core::debug::Logger::get_singleton().fatal("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    _window = std::make_unique<engine::drivers::glfw::GlfwWindow>("jengine", width, height);
    width = _window->width();
    height = _window->height();

    // editor renderer
    _renderer = std::make_unique<renderer::EditorRenderer>(*_window);

    // create scene // TODO: move to a helper function
    _default_scene = std::move(engine::core::scene::Scene());
    engine::core::scene::Entity default_entity = _default_scene.create_entity();

    _default_scene.add_component<components::Transform>(default_entity);
    _default_scene.add_component<components::MeshRenderer>(default_entity,
        _renderer->mesh_id("sphere"),
        _renderer->material_id("editor_view_default")
    );

    // main loop
    auto last_time = std::chrono::high_resolution_clock::now();
    float t = 0;
    bool is_dirty = false;

    while (!_window->should_close()) {
        // time delta
        auto current_time = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(current_time - last_time).count();
        last_time = current_time;

        _window->poll();

        // handle resize
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
        
        _renderer->render_scene(_default_scene);
    }
    return 0;
}

} // namespace editor
