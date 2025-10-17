#ifndef editor_core_scene_EDITOR_CAMERA_HPP
#define editor_core_scene_EDITOR_CAMERA_HPP

#include "engine/core/scene/perspective_camera.hpp"

#include <glm/gtx/quaternion.hpp>

#include <algorithm>

namespace editor::scene {

class EditorCamera : public engine::core::scene::PerspectiveCamera {
public:
    EditorCamera(float fov = 45.0f, float near_plane = 0.1f, float far_plane = 100.0f)
        : PerspectiveCamera(fov)
    {
        _near = near_plane;
        _far = far_plane;
        update_camera_vectors();
        update_projection();
    }

    void update_input(float dt, bool pan_active, bool orbit_active,
        const glm::vec2& mouse_delta, float scroll_delta)
    {
        if (orbit_active)
            orbit(-mouse_delta.x * _orbit_speed, mouse_delta.y * _orbit_speed);
        if (pan_active)
            pan(mouse_delta.x * _pan_speed * _distance,
                -mouse_delta.y * _pan_speed * _distance);
        if (std::abs(scroll_delta) > 0.0001f)
            zoom(scroll_delta * _zoom_speed);
    }

    void orbit(float delta_yaw, float delta_pitch) {
        _yaw += delta_yaw;
        _pitch += delta_pitch;
        _pitch = std::clamp(_pitch, -glm::radians(89.0f), glm::radians(89.0f));
        update_camera_vectors();
    }

    void pan(float delta_x, float delta_y) {
        glm::vec3 right = glm::normalize(glm::cross(_forward, _up));
        glm::vec3 up = glm::normalize(_up);
        _target += -right * delta_x + up * delta_y;
        update_camera_vectors();
    }

    void zoom(float delta) {
        _distance *= std::pow(0.95f, delta);
        _distance = std::clamp(_distance, 0.1f, 100.0f);
        update_camera_vectors();
    }

    void update_camera_vectors() {
        // spherical to cartesian
        glm::vec3 direction;
        direction.x = cos(_pitch) * sin(_yaw);
        direction.y = sin(_pitch);
        direction.z = cos(_pitch) * cos(_yaw);
        _forward = glm::normalize(direction);

        _position = _target - _forward * _distance;

        // right and up
        glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);
        _right = glm::normalize(glm::cross(world_up, _forward));
        _up = glm::normalize(glm::cross(_forward, _right));

        // update
        glm::mat3 rot_matrix(_right, _up, -_forward);
        _rotation = glm::quat_cast(rot_matrix);
        update_view();
    }

    void update_view() override {
        _view = glm::lookAt(_position, _target, _up);
    }

private:
    glm::vec3 _target{0.0f, 0.0f, 0.0f};

    glm::vec3 _forward{0.0f, 0.0f, -1.0f};
    glm::vec3 _right{1.0f, 0.0f, 0.0f};
    glm::vec3 _up{0.0f, 1.0f, 0.0f};

    float _yaw   = glm::radians(180.0f); // start facing -Z
    float _pitch = 0.0f;
    float _distance = 5.0f;

    float _orbit_speed = 0.01f;
    float _pan_speed   = 0.001f;
    float _zoom_speed  = 1.0f;
};

} // namespace editor::scene

#endif
