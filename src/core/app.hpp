#ifndef APP_HPP
#define APP_HPP

#include "graphics/instance.hpp"
#include "graphics/device.hpp"
#include "graphics/renderer.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/mesh_buffer.hpp"
#include "graphics/material.hpp"
#include "window/window.hpp"

#include <wk/wulkan.hpp>
#include <wk/ext/glfw/glfw_internal.hpp>
#include <wk/ext/glfw/surface.hpp>

#include <glm/glm.hpp>

namespace core {

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

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
    std::unique_ptr<graphics::Instance> _instance;
    std::unique_ptr<window::Window> _window;
    wk::ext::glfw::Surface _surface;
    std::unique_ptr<graphics::Device> _device;
    std::unique_ptr<graphics::Renderer> _renderer;
    std::unique_ptr<graphics::Pipeline> _pipeline;
    std::unique_ptr<graphics::MeshBuffer> _geometry;
    std::unique_ptr<graphics::Material> _material;

    // ---------- geometry ----------
    const std::vector<Vertex> _VERTICES = {
        {{-1.0f, -1.0f,  1.0f} /*xyz*/, {1, 0, 0} /*rgb*/}, // front
        {{ 1.0f, -1.0f,  1.0f},         {0, 1, 0}},
        {{ 1.0f,  1.0f,  1.0f},         {0, 0, 1}},
        {{-1.0f,  1.0f,  1.0f},         {1, 1, 0}},

        {{-1.0f, -1.0f, -1.0f},         {1, 0, 1}}, // back
        {{ 1.0f, -1.0f, -1.0f},         {0, 1, 1}},
        {{ 1.0f,  1.0f, -1.0f},         {1, 1, 1}},
        {{-1.0f,  1.0f, -1.0f},         {0, 0, 0}},
    };
    const std::vector<uint32_t> _INDICES = {
        0, 1, 2, 2, 3, 0, // front
        1, 5, 6, 6, 2, 1, // right
        7, 6, 5, 5, 4, 7, // back
        4, 0, 3, 3, 7, 4, // left
        3, 2, 6, 6, 7, 3, // top
        4, 5, 1, 1, 0, 4, // bottom
    };
};

}

#endif