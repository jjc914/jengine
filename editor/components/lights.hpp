#ifndef editor_components_LIGHTS_HPP
#define editor_components_LIGHTS_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace editor::components {

class DirectionalLight {
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
};

class PointLight {
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float radius = 10.0f;
};

class SpotLight {
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float range = 10.0f;
    float angle = glm::radians(15.0f);
};

}

#endif // editor_components_LIGHTS_HPP