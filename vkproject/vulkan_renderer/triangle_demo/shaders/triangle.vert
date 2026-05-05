#version 450
layout(location = 0) in vec4 inPosition;

layout(push_constant) uniform PushConstants {
    float aspect_ratio;
    float scale;
} pc;

void main() {
    vec4 pos = inPosition * pc.scale;
    // Correct aspect ratio: scale X by 1/aspect_ratio so the triangle stays square
    pos.x = pos.x / pc.aspect_ratio;
    gl_Position = pos;
}
