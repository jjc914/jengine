#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint entity_id;
} pc;

layout(location = 0) in vec3 in_position;

void main() {
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(in_position, 1.0);
}
