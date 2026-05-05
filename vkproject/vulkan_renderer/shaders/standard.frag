#version 460

layout(std430, set = 0, binding = 1) buffer MaterialIndices
{
    uint material_indices[];
};

struct Material
{
  vec3 color; 
  float emittance;
  float metalness;
  float shininess;
  float dummy0, dummy1;
};

layout(std430, set = 0, binding = 2) buffer Materials
{
    Material materials[];
};


layout(location = 0) in flat uint mat_idx_offset; 
layout(location = 0) out vec4 outColor;
void main() {
    vec3 color = materials[material_indices[mat_idx_offset + gl_PrimitiveID]].color; 
    outColor.xyz = color;
}
