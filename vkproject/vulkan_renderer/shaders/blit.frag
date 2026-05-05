#version 460
layout(set = 0, binding = 0) uniform sampler2D rt_texture;
layout(location = 0) out vec4 outColor;
void main() {
    //vec2 size = textureSize(rt_texture, 0);
    outColor = texelFetch(rt_texture, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
}
