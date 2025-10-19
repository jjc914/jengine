#version 450
layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 v_position;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(v_position, 1.0);
}
