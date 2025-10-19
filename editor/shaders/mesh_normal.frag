#version 450

layout(location = 0) in vec3 f_normal;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(f_normal, 1.0);
}
