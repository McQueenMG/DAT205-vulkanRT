#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "magica.hpp"

namespace triangle_asset
{
	// Represents a loaded texture image (e.g., from PNG, TGA, etc.)
	struct TextureMap
	{
		std::vector<uint8_t> data;  // Raw pixel data (RGBA)
		int width = 0;
		int height = 0;
		int channels = 4;  // Typically 4 for RGBA after loading
		std::string source_path;  // Path where it was loaded from
	};

	// Material texture references for a single material
	struct MaterialTextures
	{
		TextureMap diffuse_map;      // map_Kd
		TextureMap metalness_map;    // map_Pm
		TextureMap roughness_map;    // map_Pr
		TextureMap normal_map;       // norm
		bool has_diffuse = false;
		bool has_metalness = false;
		bool has_roughness = false;
		bool has_normal = false;
	};

	struct TriangleMesh
	{
		std::vector<glm::vec3> vertices;
		std::vector<glm::ivec3> indices;
		std::vector<Material> materials;
		std::vector<uint32_t> material_indices;
		std::vector<MaterialTextures> material_textures;  // Textures per material
        std::vector<glm::vec2> uv_list;
        std::vector<glm::ivec3> uv_indices;
	};

	TriangleMesh NormalizeTriangleMesh(const TriangleMesh& mesh);
	TriangleMesh OrientTriangleMeshOutwards(const TriangleMesh& mesh);
	TriangleMesh CreateQuadMesh(const glm::vec3& min_corner, const glm::vec3& max_corner, const Material& material);
	TriangleMesh LoadObjMesh(const std::string& filename);
}  // namespace triangle_asset
