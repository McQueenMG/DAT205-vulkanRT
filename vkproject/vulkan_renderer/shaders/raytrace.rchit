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


void main()
{
  const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
  uint index_offset = (object_info[gl_InstanceCustomIndexEXT].starting_primitive + gl_PrimitiveID) * 3;
  uvec3 idx = uvec3(indices[index_offset + 0], indices[index_offset + 1], indices[index_offset + 2]) + uvec3(object_info[gl_InstanceCustomIndexEXT].starting_vertex);
  vec3 v0 = positions[idx.x].xyz;
  vec3 v1 = positions[idx.y].xyz;
  vec3 v2 = positions[idx.z].xyz;
  vec3 local_position = barycentrics.x * v0 + barycentrics.y * v1 + barycentrics.z * v2; 
  mat4 model_matrix = object_info[gl_InstanceCustomIndexEXT].model_matrix; 
  vec3 world_position = (model_matrix * vec4(local_position, 1.0)).xyz;
  vec3 normal = -normalize(cross((v1-v0), (v2-v0)));
  prd.hit = true; 
  prd.mat_id =  material_indices[object_info[gl_InstanceCustomIndexEXT].starting_primitive + gl_PrimitiveID];
  prd.normal = normal; 
  prd.position = world_position; 
  if(!object_info[gl_InstanceCustomIndexEXT].is_static)
    prd.prev_position = (object_info[gl_InstanceCustomIndexEXT].prev_model_matrix * vec4(local_position, 1.0)).xyz;
  else prd.prev_position = world_position; 
}
