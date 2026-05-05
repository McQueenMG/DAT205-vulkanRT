#version 460
#extension GL_ARB_shader_draw_parameters : enable

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout(location = 0) in flat uint v_mat_idx_offset[];
layout(location = 1) in vec3 v_world_position[]; 

struct Material
{
  vec3 color; 
  float emittance;
  float metalness;
  float shininess;
  float dummy0, dummy1;
};

layout(std430, set = 0, binding = 1) buffer MaterialIndices
{
    uint material_indices[];
};


layout(std430, set = 0, binding = 2) buffer Materials
{
    Material materials[];
};



layout(location = 0) out flat uint material_idx; 
layout(location = 1) out vec3 world_position; 
layout(location = 2) out flat vec3 world_normal; 

void main() {
    vec3 color = materials[material_indices[v_mat_idx_offset[0] + gl_PrimitiveIDIn]].color; 

    vec3 v0 = v_world_position[0];
    vec3 v1 = v_world_position[1];
    vec3 v2 = v_world_position[2];
    world_normal = normalize(cross(normalize(v2 - v0), normalize(v1 - v0)));
    // Enforcing axis aligned normals here, since they are screwed up
    // by a hack to avoid z fighting issues as explained in magica.cpp
    vec3 abs_normal = abs(world_normal); 
    abs_normal = vec3(
        abs_normal.x > 0.5 ? 1.0f : 0.0f, 
        abs_normal.y > 0.5 ? 1.0f : 0.0f, 
        abs_normal.z > 0.5 ? 1.0f : 0.0f);
    world_normal = sign(world_normal) * abs_normal; 

    material_idx = material_indices[v_mat_idx_offset[0] + gl_PrimitiveIDIn]; 
    for(int i=0; i<3; i++) {
        gl_Position = gl_in[i].gl_Position; 
        world_position = v_world_position[i];
        EmitVertex();
    }
    EndPrimitive();
}