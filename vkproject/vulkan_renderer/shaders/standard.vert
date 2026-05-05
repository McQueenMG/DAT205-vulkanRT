#version 460
#extension GL_ARB_shader_draw_parameters : enable

vec3 vertices[6] = {
{-1.0f, -1.0f, 0.0f},
{ 1.0f, -1.0f, 0.0f},
{-1.0f,  1.0f, 0.0f},
{ 1.0f, -1.0f, 0.0f},
{ 1.0f,  1.0f, 0.0f},
{ 0.0f,  1.0f, 0.0f},
};

struct PushConstants
{
  mat4 view, proj; 
};
layout(push_constant) uniform _PushConstants { PushConstants push_constants; };

struct ModelUniforms
{
    mat4 model_matrix;
    uint mat_idx_offset; 
    uint dummy0, dummy1, dummy2; 
};

layout(std430, binding = 0) buffer PerModelUniforms
{
    ModelUniforms model_uniforms[];
};

layout(location = 0) in vec4 inPosition;
layout(location = 0) out flat uint mat_idx_offset; 

void main() {
    //gl_Position = push_constants.proj * push_constants.view * vec4(vertices[gl_VertexIndex] * 16.0 + vec3(20 * 16,20 * 16,0.0 ), 1.0);
    mat_idx_offset = model_uniforms[gl_DrawIDARB].mat_idx_offset;
    gl_Position = push_constants.proj * push_constants.view * model_uniforms[gl_DrawIDARB].model_matrix * vec4(inPosition.xyz, 1.0);
}