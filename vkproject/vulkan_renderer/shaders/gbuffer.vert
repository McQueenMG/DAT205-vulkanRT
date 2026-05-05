#version 460
#extension GL_ARB_shader_draw_parameters : enable

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
layout(location = 0) out flat uint v_mat_idx_offset;
layout(location = 1) out vec3 v_world_position; 

void main() {
    v_mat_idx_offset = model_uniforms[gl_DrawIDARB].mat_idx_offset;
    v_world_position = (model_uniforms[gl_DrawIDARB].model_matrix * vec4(inPosition.xyz, 1.0)).xyz;
    gl_Position = push_constants.proj * push_constants.view * model_uniforms[gl_DrawIDARB].model_matrix * vec4(inPosition.xyz, 1.0);
}