#include "triangle_asset.hpp"

#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "log.hpp"

namespace triangle_asset
{
    // Forward declaration - implemented in stb_image_impl.cpp
    TextureMap LoadImage(const std::filesystem::path &image_path);

    namespace
    {
        Material MakeDefaultMaterial()
        {
            Material material{};
            material.color = glm::vec3(1.0f, 1.0f, 1.0f);
            material.emittance = 0.0f;
            material.metalness = 0.0f;
            material.shininess = 0.0f;
            return material;
        }

        int ParseObjUVIndex(const std::string &token, int uv_count)
        {
            const size_t slash_pos = token.find('/');
            if (slash_pos == std::string::npos || slash_pos == token.size() - 1)
            {
                return -1;
            }

            const std::string index_str = token.substr(slash_pos + 1);
            if (index_str.empty())
            {
                throw std::runtime_error("Malformed OBJ UV index");
            }

            const int obj_index = std::stoi(index_str);
            if (obj_index > 0)
            {
                return obj_index - 1;
            }
            if (obj_index < 0)
            {
                return uv_count + obj_index;
            }
            throw std::runtime_error("OBJ indices are 1-based and cannot be zero");
        }

        int ParseObjVertexIndex(const std::string &token, int vertex_count)
        {
            const size_t slash_pos = token.find('/');
            const std::string index_str = (slash_pos == std::string::npos) ? token : token.substr(0, slash_pos);
            if (index_str.empty())
            {
                throw std::runtime_error("Malformed OBJ face index");
            }

            const int obj_index = std::stoi(index_str);
            if (obj_index > 0)
            {
                return obj_index - 1;
            }
            if (obj_index < 0)
            {
                return vertex_count + obj_index;
            }
            throw std::runtime_error("OBJ indices are 1-based and cannot be zero");
        }

        float RoughnessToShininess(float roughness)
        {
            if (roughness <= 0.0f)
            {
                return -1.0f;
            }
            return 2.0f / (roughness * roughness * roughness * roughness) - 2.0f;
        }

        struct MtlLoadResult
        {
            std::map<std::string, Material> materials;
            std::map<std::string, MaterialTextures> textures;
        };

        MtlLoadResult LoadMtlFile(const std::filesystem::path &mtl_path)
        {
            MtlLoadResult result;
            std::ifstream input(mtl_path);
            if (!input)
            {
                LOG(WARNING) << "Failed to open MTL file: " << mtl_path.string() << "\n";
                return result;
            }

            std::string line;
            std::string current_name;
            while (std::getline(input, line))
            {
                if (line.empty() || line[0] == '#')
                    continue;

                std::istringstream line_stream(line);
                std::string prefix;
                line_stream >> prefix;

                if (prefix == "newmtl")
                {
                    line_stream >> current_name;
                    result.materials[current_name] = MakeDefaultMaterial();
                    result.textures[current_name] = MaterialTextures();
                    continue;
                }

                if (current_name.empty())
                    continue;

                auto &material = result.materials[current_name];
                auto &tex = result.textures[current_name];

                if (prefix == "Kd")
                {
                    line_stream >> material.color.x >> material.color.y >> material.color.z;
                }
                else if (prefix == "Ka")
                {
                    if (material.color == glm::vec3(1.0f, 1.0f, 1.0f))
                    {
                        line_stream >> material.color.x >> material.color.y >> material.color.z;
                    }
                }
                else if (prefix == "Ke")
                {
                    glm::vec3 emission{};
                    line_stream >> emission.x >> emission.y >> emission.z;
                    material.emittance = std::max(emission.x, std::max(emission.y, emission.z));
                }
                else if (prefix == "Pm")
                {
                    line_stream >> material.metalness;
                }
                else if (prefix == "Pr")
                {
                    float roughness = 0.0f;
                    line_stream >> roughness;
                    material.shininess = RoughnessToShininess(roughness);
                }
                else if (prefix == "d")
                {
                    float opacity = 1.0f;
                    line_stream >> opacity;
                    if (opacity <= 0.0f)
                    {
                        material.emittance = 0.0f;
                    }
                }
                else if (prefix == "Tr")
                {
                    float transmission = 0.0f;
                    line_stream >> transmission;
                    if (transmission >= 1.0f)
                    {
                        material.emittance = 0.0f;
                    }
                }
                else if (prefix == "map_Kd")
                {
                    std::string texture_filename;
                    line_stream >> texture_filename;
                    std::filesystem::path texture_path = mtl_path.parent_path() / texture_filename;
                    tex.diffuse_map = LoadImage(texture_path);
                    tex.has_diffuse = !tex.diffuse_map.data.empty();
                }
                else if (prefix == "map_Pm")
                {
                    std::string texture_filename;
                    line_stream >> texture_filename;
                    std::filesystem::path texture_path = mtl_path.parent_path() / texture_filename;
                    tex.metalness_map = LoadImage(texture_path);
                    tex.has_metalness = !tex.metalness_map.data.empty();
                }
                else if (prefix == "map_Pr")
                {
                    std::string texture_filename;
                    line_stream >> texture_filename;
                    std::filesystem::path texture_path = mtl_path.parent_path() / texture_filename;
                    tex.roughness_map = LoadImage(texture_path);
                    tex.has_roughness = !tex.roughness_map.data.empty();
                }
                else if (prefix == "norm")
                {
                    std::string texture_filename;
                    line_stream >> texture_filename;
                    std::filesystem::path texture_path = mtl_path.parent_path() / texture_filename;
                    tex.normal_map = LoadImage(texture_path);
                    tex.has_normal = !tex.normal_map.data.empty();
                }
            }

            return result;
        }

    } // anonymous namespace

    TriangleMesh NormalizeTriangleMesh(const TriangleMesh &input_mesh)
    {
        TriangleMesh mesh = input_mesh;
        if (mesh.vertices.empty())
            return mesh;

        glm::vec3 min_v(std::numeric_limits<float>::max());
        glm::vec3 max_v(std::numeric_limits<float>::lowest());
        for (const auto &v : mesh.vertices)
        {
            min_v = glm::min(min_v, v);
            max_v = glm::max(max_v, v);
        }

        const glm::vec3 center = 0.5f * (min_v + max_v);
        const glm::vec3 extents = max_v - min_v;
        const float largest_extent = std::max(extents.x, std::max(extents.y, extents.z));
        const float scale = (largest_extent > 0.0f) ? (4.0f / largest_extent) : 1.0f;

        for (auto &v : mesh.vertices)
        {
            v = (v - center) * scale;
        }
        return mesh;
    }

    TriangleMesh OrientTriangleMeshOutwards(const TriangleMesh &input_mesh)
    {
        TriangleMesh mesh = input_mesh;
        if (mesh.vertices.empty() || mesh.indices.empty())
            return mesh;

        glm::vec3 centroid(0.0f);
        for (const auto &v : mesh.vertices)
        {
            centroid += v;
        }
        centroid /= static_cast<float>(mesh.vertices.size());

        for (auto &tri : mesh.indices)
        {
            const glm::vec3 &v0 = mesh.vertices[tri.x];
            const glm::vec3 &v1 = mesh.vertices[tri.y];
            const glm::vec3 &v2 = mesh.vertices[tri.z];
            const glm::vec3 face_center = (v0 + v1 + v2) / 3.0f;
            const glm::vec3 face_normal = glm::cross(v1 - v0, v2 - v0);

            if (glm::dot(face_normal, face_center - centroid) < 0.0f)
            {
                std::swap(tri.y, tri.z);
            }
        }

        return mesh;
    }

    TriangleMesh CreateQuadMesh(const glm::vec3 &min_corner, const glm::vec3 &max_corner, const Material &material)
    {
        TriangleMesh mesh;
        mesh.vertices = {
            {min_corner.x, min_corner.y, min_corner.z},
            {max_corner.x, min_corner.y, min_corner.z},
            {max_corner.x, max_corner.y, max_corner.z},
            {min_corner.x, max_corner.y, max_corner.z}};
        mesh.indices = {glm::ivec3(0, 1, 2), glm::ivec3(0, 2, 3)};
        mesh.materials = {material};
        mesh.material_indices = {0, 0};
        mesh.material_textures = {MaterialTextures()};
        return mesh;
    }

    TriangleMesh LoadObjMesh(const std::string &filename)
    {
        std::ifstream input(filename);
        if (!input)
        {
            LOG(ERROR) << "Failed to open OBJ file: " << filename << "\n";
            throw std::runtime_error("Failed to open OBJ file");
        }

        TriangleMesh mesh;
        MtlLoadResult mtl_result;
        std::map<std::string, uint32_t> mesh_material_indices_by_name;
        std::filesystem::path obj_path(filename);
        uint32_t current_material_index = 0;
        std::string line;
        while (std::getline(input, line))
        {
            if (line.empty())
                continue;
            if (line[0] == '#')
                continue;

            std::istringstream line_stream(line);
            std::string prefix;
            line_stream >> prefix;

            if (prefix == "v")
            {
                glm::vec3 v{};
                line_stream >> v.x >> v.y >> v.z;
                mesh.vertices.push_back(v);
                continue;
            }

            if (prefix == "vt")
            {
                glm::vec2 uv{};
                line_stream >> uv.x >> uv.y;
                mesh.uv_list.push_back(uv);
                continue;
            }

            if (prefix == "mtllib")
            {
                std::string mtl_filename;
                line_stream >> mtl_filename;
                mtl_result = LoadMtlFile(obj_path.parent_path() / mtl_filename);
                continue;
            }

            if (prefix == "usemtl")
            {
                std::string current_material_name;
                line_stream >> current_material_name;
                if (mtl_result.materials.count(current_material_name) > 0)
                {
                    if (mesh_material_indices_by_name.count(current_material_name) == 0)
                    {
                        mesh_material_indices_by_name[current_material_name] = static_cast<uint32_t>(mesh.materials.size());
                        mesh.materials.push_back(mtl_result.materials[current_material_name]);
                        mesh.material_textures.push_back(mtl_result.textures[current_material_name]);
                    }
                    current_material_index = mesh_material_indices_by_name[current_material_name];
                }
                else
                {
                    current_material_index = 0;
                }
                continue;
            }

            if (prefix == "f")
            {
                std::vector<int> face_position_indices;
                std::vector<int> face_uv_indices;
                std::string vertex_token;
                while (line_stream >> vertex_token)
                {
                    const int pos_idx = ParseObjVertexIndex(vertex_token, static_cast<int>(mesh.vertices.size()));
                    if (pos_idx < 0 || pos_idx >= static_cast<int>(mesh.vertices.size()))
                    {
                        LOG(ERROR) << "OBJ face references out-of-range vertex index in " << filename << "\n";
                        throw std::runtime_error("OBJ face index out of range");
                    }
                    face_position_indices.push_back(pos_idx);


                    const int uv_idx = ParseObjUVIndex(vertex_token, static_cast<int>(mesh.uv_list.size()));
                    if (uv_idx >= 0)
                    {
                        face_uv_indices.push_back(uv_idx);
                    }
                    else
                    {
                        face_uv_indices.push_back(-1);
                    }
                }

                if (face_position_indices.size() < 3)
                {
                    continue;
                }

                for (size_t i = 1; i + 1 < face_position_indices.size(); i++)
                {
                    mesh.indices.push_back(glm::ivec3(face_position_indices[0], face_position_indices[i], face_position_indices[i + 1]));
                    mesh.material_indices.push_back(current_material_index);

                    mesh.uv_indices.push_back(glm::ivec3(face_uv_indices[0], face_uv_indices[i], face_uv_indices[i + 1]));
                }
            }
        }

        if (mesh.materials.empty())
        {
            LOG(WARNING) << "OBJ file has no materials, using default for " << filename << "\n";
            mesh.materials.push_back(MakeDefaultMaterial());
            mesh.material_textures.push_back(MaterialTextures());
        }

        if (mesh.vertices.empty() || mesh.indices.empty())
        {
            LOG(ERROR) << "OBJ file has no renderable geometry: " << filename << "\n";
            throw std::runtime_error("OBJ file has no renderable geometry");
        }

        return mesh;
    }

} // namespace triangle_asset
