#ifndef engine_core_scene_ORTHOGRAPHICS_CAMERA_HPP
#define engine_core_scene_ORTHOGRAPHICS_CAMERA_HPP

#include "camera.hpp"

namespace engine::core::scene {

class OrthographicCamera : public Camera {
public:
    OrthographicCamera(float size = 10.0f) : _size(size) {}

    void resize(uint32_t width, uint32_t height) override {
        _width = width; _height = height;
        _aspect = static_cast<float>(width) / static_cast<float>(height);
        update_projection();
    }

    void update_projection() override {
        float half_width = _size * 0.5f * _aspect;
        float half_height = _size * 0.5f;
        _projection = glm::ortho(-half_width, half_width, -half_height, half_height, _near, _far);
    }

    void set_size(float size) { _size = size; update_projection(); }

private:
    float _size;
    float _aspect = 1.0f;
};

} // namespace engine::core::scene

#endif // engine_core_scene_ORTHOGRAPHICS_CAMERA_HPP