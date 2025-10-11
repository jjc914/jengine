#ifndef editor_APP_HPP
#define editor_APP_HPP

#include "engine/core/graphics/instance.hpp"
#include "engine/core/graphics/device.hpp"
#include "engine/core/graphics/shader.hpp"
#include "engine/core/graphics/pipeline.hpp"
#include "engine/core/graphics/frame_graph.hpp"
#include "engine/core/graphics/mesh_buffer.hpp"
#include "engine/core/graphics/material.hpp"
#include "engine/core/graphics/render_target.hpp"
#include "engine/core/graphics/descriptor_set_layout.hpp"

#include "engine/core/window/window.hpp"

#include "engine/import/mesh.hpp"

#include <wk/wulkan.hpp>
#include <wk/ext/glfw/glfw_internal.hpp>
#include <wk/ext/glfw/surface.hpp>

#include <glm/glm.hpp>

namespace editor {

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

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
    std::unique_ptr<engine::core::graphics::Instance> _instance;
    std::unique_ptr<engine::core::window::Window> _window;
    std::shared_ptr<engine::core::graphics::Device> _device;

    std::shared_ptr<engine::core::graphics::Shader> _vertex_shader;
    std::shared_ptr<engine::core::graphics::Shader> _fragment_shader;
    std::shared_ptr<engine::core::graphics::DescriptorSetLayout> _descriptor_layout;

    std::shared_ptr<engine::core::graphics::Pipeline> _present_pipeline;
    std::shared_ptr<engine::core::graphics::Pipeline> _editor_camera_pipeline;

    std::shared_ptr<engine::core::graphics::RenderTarget> _viewport;
    std::shared_ptr<engine::core::graphics::RenderTarget> _editor_camera_target;

    std::shared_ptr<engine::core::graphics::MeshBuffer> _geometry;
    std::shared_ptr<engine::core::graphics::Material> _material;
};

} // namespace editor

#endif // editor_APP_HPP