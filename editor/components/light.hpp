#ifndef editor_components_LIGHT_HPP
#define editor_components_LIGHT_HPP

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
    float inner_cutoff = glm::radians(15.0f);
    float outer_cutoff = glm::radians(25.0f);
};

}

#endif