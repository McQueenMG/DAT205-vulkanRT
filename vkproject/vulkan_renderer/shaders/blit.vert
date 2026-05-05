#version 460

vec3 vertices[6] = {
{-1.0f, -1.0f, 0.0f},
{1.0f, -1.0f, 0.0f},
{-1.0f, 1.0f, 0.0f},
{1.0f, -1.0f, 0.0f},
{-1.0f, 1.0f, 0.0f},
{1.0f, 1.0f, 0.0f}
};

void main() {
    gl_Position = vec4(vertices[gl_VertexIndex], 1.0);
}