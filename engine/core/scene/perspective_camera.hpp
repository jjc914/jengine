#ifndef engine_core_scene_PERSPECTIVE_CAMERA_HPP
#define engine_core_scene_PERSPECTIVE_CAMERA_HPP

#include "camera.hpp"

namespace engine::core::scene {

class PerspectiveCamera : public Camera {
public:
    PerspectiveCamera(float fov = 45.0f) : _fov(fov) {}

    void resize(uint32_t width, uint32_t height) override {
        _width = width;
        _height = height;
        _aspect = static_cast<float>(width) / static_cast<float>(height);
        update_projection();
    }

    void update_projection() override {
        _projection = glm::perspective(glm::radians(_fov), _aspect, _near, _far);
    }

    void set_fov(float fov) {
        _fov = fov;
        update_projection();
    }

protected:
    float _fov;
    float _aspect = 1.0f;
};

} // namespace engine::core::scene

#endif // engine_core_scene_PERSPECTIVE_CAMERA_HPP