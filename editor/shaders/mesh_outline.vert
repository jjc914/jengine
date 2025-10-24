#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout(location = 0) out vec3 v_color;

void main() {
    vec4 color = vec4(0.9f);
    float width = 0.02f;

    gl_Position = ubo.proj * ubo.view * pc.model * vec4(in_position + normalize(in_normal) * width, 1.0);
    v_color = color.xyz;
}
