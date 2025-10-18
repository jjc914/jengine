#version 450
layout(location = 0) in vec3 v_dir;
layout(location = 0) out vec4 out_color;

void main() {
    float t = normalize(v_dir).y * 0.5 + 0.5;
    vec3 top = vec3(0.3, 0.5, 0.8);
    vec3 bottom = vec3(0.7, 0.9, 1.0);
    vec3 color = mix(bottom, top, t);
    out_color = vec4(color, 1.0);
}
