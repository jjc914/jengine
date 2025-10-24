#version 450

layout (set = 0, binding = 0) uniform sampler2D tex_sampler;

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec4 v_col;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = v_col * texture(tex_sampler, v_uv);
}