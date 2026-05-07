#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require
#include "common.glsl"

hitAttributeEXT vec2 attribs;

// clang-format off
layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
// clang-format on

layout(std430, set = 0, binding = 1) buffer ObjectInfo_
{
    ObjectInfo object_info[];
};

layout(std430, set = 0, binding = 2) buffer MaterialIndices
{
    uint material_indices[];
};
layout(std430, set = 0, binding = 3) buffer Indices
{
    uint indices[];
};
layout(std430, set = 0, binding = 4) buffer Positions
{
    vec4 positions[];
};
layout(std430, set = 0, binding = 5) buffer UVs
{
  vec2 uvs[];
};
layout(std430, set = 0, binding = 6) buffer Materials
{
    Material materials[];
};


void main()
{
  const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
  uint index_offset = (object_info[gl_InstanceCustomIndexEXT].starting_primitive + gl_PrimitiveID) * 3;
  uvec3 idx = uvec3(indices[index_offset + 0], indices[index_offset + 1], indices[index_offset + 2]) + uvec3(object_info[gl_InstanceCustomIndexEXT].starting_vertex);
  uvec3 uv_idx = uvec3(index_offset + 0, index_offset + 1, index_offset + 2);
  vec3 v0 = positions[idx.x].xyz;
  vec3 v1 = positions[idx.y].xyz;
  vec3 v2 = positions[idx.z].xyz;
  vec2 uv0 = uvs[uv_idx.x];
  vec2 uv1 = uvs[uv_idx.y];
  vec2 uv2 = uvs[uv_idx.z];
  vec3 local_position = barycentrics.x * v0 + barycentrics.y * v1 + barycentrics.z * v2; 
  vec2 uv = barycentrics.x * uv0 + barycentrics.y * uv1 + barycentrics.z * uv2;
  // Apply per-material UV flips
  prd.mat_id =  material_indices[object_info[gl_InstanceCustomIndexEXT].starting_primitive + gl_PrimitiveID];
  if (materials[prd.mat_id].flip_uv_x != 0) uv.x = 1.0 - uv.x;
  if (materials[prd.mat_id].flip_uv_y != 0) uv.y = 1.0 - uv.y;
  mat4 model_matrix = object_info[gl_InstanceCustomIndexEXT].model_matrix; 
  vec3 world_position = (model_matrix * vec4(local_position, 1.0)).xyz;

  // vec3 normal = -normalize(cross((v1-v0), (v2-v0)));
  // prd.hit = true; 
  // prd.normal = normal; 

  vec3 geomNormal = normalize(cross((v1 - v0), (v2 - v0)));
  mat3 normalMatrix = transpose(inverse(mat3(model_matrix)));
  vec3 worldNormal = normalize(normalMatrix * geomNormal);
  prd.hit = true;
  prd.normal = worldNormal;

  prd.position = world_position; 
  prd.uv = uv;
  if(!object_info[gl_InstanceCustomIndexEXT].is_static)
    prd.prev_position = (object_info[gl_InstanceCustomIndexEXT].prev_model_matrix * vec4(local_position, 1.0)).xyz;
  else prd.prev_position = world_position; 
}
