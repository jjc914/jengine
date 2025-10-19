#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec3 v_dir;

void main() {
    mat4 rot_view = mat4(mat3(ubo.view));
    v_dir = in_position;
    vec4 pos = ubo.proj * rot_view * vec4(in_position, 1.0);
    gl_Position = pos.xyww;
}
