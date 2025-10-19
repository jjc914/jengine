#version 450
layout(location = 0) out uint out_id;

layout(push_constant) uniform PC {
    uint entity_id;
} pc;

void main() {
    out_id = pc.entity_id;
}
