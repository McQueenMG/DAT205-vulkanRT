#version 460





vec2 SphericalCoordinates(vec3 v)
{
    float theta = acos(v.z/sqrt(v.x*v.x + v.y*v.y + v.z*v.z));
    float phi = sign(v.y) * acos(v.x / sqrt(v.x * v.x + v.y*v.y));
    return vec2(theta, phi);
}

layout(location = 0) in flat uint material_idx; 
layout(location = 1) in vec3 world_position; 
layout(location = 2) in flat vec3 world_normal; 
layout(location = 0) out vec4 outColor;
void main() {
    // Put position, normal, and material ID in a single vec4 output
    //outColor.xyz = world_position / 1024.0; 
    outColor.xy = SphericalCoordinates(world_normal.xyz);
    outColor.z = float(material_idx) / 20.0f; 
    outColor.xyz = world_normal.xyz; 

    //outColor.xyz = vec3(dot(world_normal.xyz, normalize(vec3(1, 10, 1))));

    //outColor.xyz = abs(world_normal);
}
