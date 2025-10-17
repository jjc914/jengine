#ifndef editor_APP_HPP
#define editor_APP_HPP

#include "engine/core/window/window.hpp"
#include "engine/core/scene/scene.hpp"

#include "editor/renderer/editor_renderer.hpp"

#include "editor/gui/panels/scene_view_panel.hpp"

#include <wk/wulkan.hpp>
#include <wk/ext/glfw/glfw_internal.hpp>
#include <wk/ext/glfw/surface.hpp>

#include <glm/glm.hpp>

namespace editor {

class App {
public:
    App();
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&& other) noexcept = default;
    App& operator=(App&& other) noexcept = default;

    int run();

private:
    std::unique_ptr<engine::core::window::Window> _window;
    std::unique_ptr<renderer::EditorRenderer> _renderer;
    engine::core::scene::Scene _default_scene;
};

} // namespace editor

#endif // editor_APP_HPP