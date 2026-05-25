#define PI 3.14159265359

#ifdef __cplusplus
#pragma once
typedef glm::vec4 vec4;
typedef uint32_t uint;
typedef glm::mat4 mat4;
typedef glm::vec3 vec3;
typedef glm::vec2 vec2;
#define MAX(a, b) glm::max(a, b)
#define INLINE inline
#else
#define MAX(a, b) max(a, b)
#define INLINE
#endif 

struct Light
{
    vec3 position;
    float dummy0; 
    vec3 intensity; 
    float dummy1;
};

struct PushConstants
{
  mat4 inv_view, inv_proj, prev_view_proj;
  uint frame; 
  uint num_lights;
  uint num_indirect_samples; 
};

struct hitPayload
{
  bool hit; 
  uint mat_id; 
        vec3 position, normal, prev_position;
        vec2 uv;
};


struct ObjectInfo
{
  uint starting_primitive; 			// First primitive for the corresponding blas
  uint starting_vertex;				// First vertex for the corresponding blas
  uint starting_material;
  bool is_static; 
  mat4 model_matrix;				// The instance's model matrix
  mat4 prev_model_matrix;			// The instance's model matrix in last frame, if dynamic
};

struct Material
{
  vec4 color;                  // 16 bytes
  float emittance;             // 4
  float metalness;             // 4
  float shininess;             // 4
  float specularity;           // 4  > completes 2nd 16-byte block
  float opacity;                // 4
  int diffuse_texture_index;   // 4
  int roughness_texture_index;  // 4
  int metalness_texture_index;  // 4
  int normal_texture_index;    // 4
  int flip_uv_x;               // 4  <- ADD THIS (0 or 1)
  int flip_uv_y;               // 4  <- ADD THIS (0 or 1)
        // Keep explicit 64-byte stride for SSBO array compatibility between C++ and GLSL.
  int pad1;                    // 4
};

///////////////////////////////////////////////////////////////////////////
// Generate uniform points on a disc
///////////////////////////////////////////////////////////////////////////
INLINE vec2 concentricSampleDisk(float u1, float u2)
{
        float r, theta;
        // Map uniform random numbers to $[-1,1]^2$
        float sx = 2 * u1 - 1;
        float sy = 2 * u2 - 1;
        // Map square to $(r,\theta)$
        // Handle degeneracy at the origin
        if(sx == 0.0 && sy == 0.0)
        {
                return vec2(0, 0);
        }
        if(sx >= -sy)
        {
                if(sx > sy)
                { // Handle first region of disk
                        r = sx;
                        if(sy > 0.0)
                                theta = sy / r;
                        else
                                theta = 8.0f + sy / r;
                }
                else
                { // Handle second region of disk
                        r = sy;
                        theta = 2.0f - sx / r;
                }
        }
        else
        {
                if(sx <= sy)
                { // Handle third region of disk
                        r = -sx;
                        theta = 4.0f - sy / r;
                }
                else
                { // Handle fourth region of disk
                        r = -sy;
                        theta = 6.0f + sx / r;
                }
        }
        theta *= float(PI) / 4.0f;
        return r * vec2(cos(theta), sin(theta));
}

///////////////////////////////////////////////////////////////////////////
// Generate points with a cosine distribution on the hemisphere
///////////////////////////////////////////////////////////////////////////
INLINE vec3 cosineSampleHemisphere(float u1, float u2)
{
        vec3 ret = vec3(concentricSampleDisk(u1, u2), 0.0);
        ret.z = sqrt(MAX(0.f, 1.f - ret.x * ret.x - ret.y * ret.y));
        return ret;
}

///////////////////////////////////////////////////////////////////////////
// Generate a vector that is perpendicular to another
///////////////////////////////////////////////////////////////////////////
INLINE vec3 perpendicular(vec3 v)
{
        if(abs(v.x) < abs(v.y))
        {
                return vec3(0.0f, -v.z, v.y);
        }
        return vec3(-v.z, 0.0f, v.x);
}
