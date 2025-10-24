#version 450

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec4 in_col;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec4 v_col;

layout(push_constant) uniform PushConstants {
    mat4 proj;
} pc;

void main() {
    v_uv  = in_uv;
    v_col = in_col;
    gl_Position = pc.proj * vec4(in_pos, 0.0, 1.0);
}
