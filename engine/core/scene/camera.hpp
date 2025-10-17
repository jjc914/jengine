#ifndef engine_core_scene_CAMERA_HPP
#define engine_core_scene_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <cstdint>

namespace engine::core::scene {

class Camera {
public:
    virtual ~Camera() = default;

    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual void update_projection() = 0;

    virtual void update_view() {
        glm::mat4 rot = glm::mat4_cast(_rotation);
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), -_position);
        _view = rot * trans;
    }

    virtual void look_at(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
        glm::vec3 forward = glm::normalize(target - _position);

        glm::vec3 right = glm::normalize(glm::cross(forward, up));
        glm::vec3 cam_up = glm::normalize(glm::cross(right, forward));

        glm::mat3 rotation_matrix(right, cam_up, -forward);

        _rotation = glm::quat_cast(rotation_matrix);

        update_view();
    }

    const glm::mat4& view() const { return _view; }
    const glm::mat4& projection() const { return _projection; }

    void set_position(const glm::vec3& pos) {
        _position = pos;
        update_view();
    }

    void set_rotation(const glm::quat& rot) {
        _rotation = rot;
        update_view();
    }

    const glm::vec3& position() const { return _position; }
    const glm::quat& rotation() const { return _rotation; }

protected:
    glm::vec3 _position{0.0f, 0.0f, 5.0f};
    glm::quat _rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::mat4 _view{1.0f};
    glm::mat4 _projection{1.0f};

    uint32_t _width = 1;
    uint32_t _height = 1;
    float _near = 0.1f;
    float _far  = 100.0f;
};

} // namespace engine::core::scene

#endif // engine_core_scene_CAMERA_HPP