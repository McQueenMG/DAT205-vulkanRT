#include "magica.hpp"
#include <tuple>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <string>
#include <iostream>
#include "log.hpp"
#include "ogt_vox.hpp"
#include <functional>

namespace magica
{
    using namespace std;
    ///////////////////////////////////////////////////////////////////////////
    // Load voxel data from file
    ///////////////////////////////////////////////////////////////////////////
    struct compare_ivec3
    {
        bool operator()(const glm::ivec3 &lhs, const glm::ivec3 &rhs) const
        {
            if (lhs.x == rhs.x)
            {
                if (lhs.y == rhs.y)
                {
                    return lhs.z < rhs.z; 
                }
                else
                    return lhs.y < rhs.y;
            }
            else
                return lhs.x < rhs.x;
        }
    };

    std::map<glm::ivec3, uint32_t, compare_ivec3> compact_vertex_indices;

    tuple<vector<glm::ivec3> /* coords */, vector<Material> /* materials */, vector<uint32_t> /* material indices*/> 
        LoadMagicaModel(
        const std::vector<uint8_t> &data, int model_nr)
    {
        const ogt_vox_scene *scene = ogt_vox_read_scene(data.data(), (uint32_t)data.size());
        assert(scene->num_models > 0);

        auto model = scene->models[model_nr];
        auto m = scene->materials;

        std::vector<glm::ivec3> coords;

        std::map<uint32_t, Material> material_map; 
        std::vector<uint32_t> material_indices; 

        for (uint32_t z = 0; z < model->size_z; z++)
            for (uint32_t y = 0; y < model->size_y; y++) 
                for (uint32_t x = 0; x < model->size_x; x++)
                {
                    uint32_t idx = z * model->size_x * model->size_y + y * model->size_x + x; 
                    uint32_t color_idx = model->voxel_data[idx];
                    if (color_idx == 0) continue; // Empty voxel
                    auto color = scene->palette.color[color_idx];
                    glm::vec3 fcolor = glm::vec3(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);                    
                    material_indices.push_back(color_idx);
                    material_map[color_idx].color = glm::vec4(fcolor, 1.0f);
                    material_map[color_idx].emittance = scene->materials.matl[color_idx].emit;
                    float r = scene->materials.matl[color_idx].rough;
                    // A shininess of 0 means diffuse. A shininess of -1 means perfect specular.
                    if (scene->materials.matl[color_idx].type == ogt_matl_type_diffuse)
                        material_map[color_idx].shininess = 0.0f;
                    else if (r == 0.0)
                        material_map[color_idx].shininess = -1.0f;
                    else
                        material_map[color_idx].shininess = 2.0f / (r*r*r*r) - 2.0f; 
                    // Negative metalness will mean glass
                    material_map[color_idx].metalness = scene->materials.matl[color_idx].metal;
                    if (scene->materials.matl[color_idx].type == ogt_matl_type_glass)
                        material_map[color_idx].metalness = -1.0f;

                    coords.push_back(glm::ivec3(x, y, z));                    
                }

        // Compact materials
        std::map<uint32_t, uint32_t> matmap; 
        std::vector<Material> materials; 
        for (auto &m : material_map)
        {
            matmap[m.first] = (uint32_t)materials.size();
            materials.push_back(m.second);
        } 
        for (auto &id : material_indices)
            id = matmap[id];

        ogt_vox_destroy_scene(scene);
        return make_tuple(coords, materials, material_indices);
    }

    ///////////////////////////////////////////////////////////////////////
    // Remove voxels that cannot be seen from outside tile
    ///////////////////////////////////////////////////////////////////////
    void RemoveInsideVoxels(vector<glm::ivec3> &coords, vector<uint32_t> &material_indices, const int block_size)
    {
        // Helper
        auto IsOutside = [&](const glm::ivec3 &coord)
        {
            return (coord.x < 0 || coord.x >= block_size || coord.y < 0 || coord.y >= block_size || coord.z < 0 ||
                    coord.z >= block_size);
        };
        // Mark all occupied voxels (for fast lookup)
        vector<int> occupied(block_size * block_size * block_size, 0);
        auto IsOccupied = [&](const glm::ivec3 &coord)
        {
            if (IsOutside(coord)) return false;
            return occupied[coord.z * block_size * block_size + coord.y * block_size + coord.x] != 0;
        };
        for (const auto &coord : coords)
        {
            occupied[coord.z * block_size * block_size + coord.y * block_size + coord.x] = 1;
        }

        // Add all border voxels to queue and mark as visible
        vector<glm::ivec3> queue;
        vector<int> visible(block_size * block_size * block_size, 0);
        auto IsVisible = [&](const glm::ivec3 &coord)
        {
            if (IsOutside(coord)) return true;
            return visible[coord.z * block_size * block_size + coord.y * block_size + coord.x] != 0;
        };
        auto SetVisible = [&](const glm::ivec3 &coord)
        {
            if (IsOutside(coord)) return;
            if (IsOccupied(coord)) return;
            if (!IsVisible(coord))
            {
                queue.push_back(coord);
            }
            visible[coord.z * block_size * block_size + coord.y * block_size + coord.x] = 1;
        };

        for (int x = 0; x < block_size; x++)
        {
            for (int y = 0; y < block_size; y++)
            {
                SetVisible({x, y, 0});
                SetVisible({x, y, block_size - 1});
            }
            for (int z = 0; z < block_size; z++)
            {
                SetVisible({x, 0, z});
                SetVisible({x, block_size - 1, z});
            }
        }
        for (int y = 0; y < block_size; y++)
        {
            for (int z = 0; z < block_size; z++)
            {
                SetVisible({0, y, z});
                SetVisible({block_size - 1, y, z});
            }
        }

        // Recursively add all immediately visible neighbors
        while (queue.size() > 0)
        {
            const glm::ivec3 coord = queue.back();
            queue.pop_back();
            SetVisible({coord.x + 1, coord.y, coord.z});
            SetVisible({coord.x - 1, coord.y, coord.z});
            SetVisible({coord.x, coord.y + 1, coord.z});
            SetVisible({coord.x, coord.y - 1, coord.z});
            SetVisible({coord.x, coord.y, coord.z + 1});
            SetVisible({coord.x, coord.y, coord.z - 1});
        }

        // Mark all voxels with no immediately visible neighbours
        // to be removed
        vector<glm::ivec3> new_coords;
        vector<uint32_t> new_material_indices;
        for (int i = 0; i < coords.size(); i++)
        {
            const glm::ivec3 &coord = coords[i];
            bool has_visible_neighbor =
                IsVisible({coord.x + 1, coord.y, coord.z}) || IsVisible({coord.x - 1, coord.y, coord.z}) ||
                IsVisible({coord.x, coord.y + 1, coord.z}) || IsVisible({coord.x, coord.y - 1, coord.z}) ||
                IsVisible({coord.x, coord.y, coord.z + 1}) || IsVisible({coord.x, coord.y, coord.z - 1});
            if (has_visible_neighbor)
            {
                new_coords.push_back(coords[i]);
                new_material_indices.push_back(material_indices[i]);
            }
        }
        LOG(INFO) << "\nRemoveInsideVoxels: Removed " << coords.size() << " - " << new_coords.size() << " = "
                  << coords.size() - new_coords.size() << " inside voxels.\n";
        coords = move(new_coords);
        material_indices = move(new_material_indices);
    }



    ///////////////////////////////////////////////////////////////////////////
    // Create a mesh from voxel data
    ///////////////////////////////////////////////////////////////////////////
    std::tuple<std::vector<glm::vec3> /* vertices */, std::vector<uint32_t> /* material_indices */,
               std::vector<glm::ivec3> /* indices */>
    CreateMeshFromMagicaModel(const vector<glm::ivec3> &voxel_coords, const vector<uint32_t> &voxel_material_indices) 
    { 
        ///////////////////////////////////////////////////////////////////////
        // Mark occupied voxels, so we know which faces to discard.
        // NOTE: Assuming 16^3 blocks
        ///////////////////////////////////////////////////////////////////////
        const int BLOCK_SIZE = 16; 
        std::vector<bool> occupied(BLOCK_SIZE * BLOCK_SIZE * BLOCK_SIZE, false);
        for (auto &c : voxel_coords)
            occupied[c.z * BLOCK_SIZE * BLOCK_SIZE + c.y * BLOCK_SIZE + c.x] = true; 
        auto IsOccupied = [&](const glm::ivec3 &c)
        { 
            if (c.x < 0 || c.x >= BLOCK_SIZE || c.y < 0 || c.y >= BLOCK_SIZE || c.z < 0 ||
                c.z >= BLOCK_SIZE) /*outside*/
                return false;
            if(occupied[c.z * BLOCK_SIZE * BLOCK_SIZE + c.y * BLOCK_SIZE + c.x]) return true;
            return false;
        };


        std::vector<glm::ivec3> vertices(voxel_coords.size() * 8);
        std::vector<glm::ivec3> indices(voxel_coords.size() * 12);
        for (int i = 0; i < voxel_coords.size(); i++)
        {
            //     6--------7     Y
            //    /|       /|     ^ Z
            //   2--------3 |     |/
            //   | |      | |     O---> X
            //   | 4------|-5
            //   |/       |/
            //   0--------1
            vertices[i * 8 + 0] = voxel_coords[i] + glm::ivec3(0, 0, 0); 
            vertices[i * 8 + 1] = voxel_coords[i] + glm::ivec3(1, 0, 0); 
            vertices[i * 8 + 2] = voxel_coords[i] + glm::ivec3(0, 1, 0); 
            vertices[i * 8 + 3] = voxel_coords[i] + glm::ivec3(1, 1, 0); 
            vertices[i * 8 + 4] = voxel_coords[i] + glm::ivec3(0, 0, 1); 
            vertices[i * 8 + 5] = voxel_coords[i] + glm::ivec3(1, 0, 1); 
            vertices[i * 8 + 6] = voxel_coords[i] + glm::ivec3(0, 1, 1); 
            vertices[i * 8 + 7] = voxel_coords[i] + glm::ivec3(1, 1, 1); 

            for (int j = 0; j < 12; j++)
                indices[i * 12 + j] = glm::ivec3(-1);

            if (!IsOccupied(voxel_coords[i] + glm::ivec3(0, 0, -1)))
            {
                indices[i * 12 + 0] = glm::ivec3(0, 1, 2) + glm::ivec3(i * 8);
                indices[i * 12 + 1] = glm::ivec3(1, 3, 2) + glm::ivec3(i * 8);
            }
            if (!IsOccupied(voxel_coords[i] + glm::ivec3(1, 0, 0)))
            {
                indices[i * 12 + 2] = glm::ivec3(1, 5, 7) + glm::ivec3(i * 8);
                indices[i * 12 + 3] = glm::ivec3(1, 7, 3) + glm::ivec3(i * 8);
            }
            if (!IsOccupied(voxel_coords[i] + glm::ivec3(0, 0, 1)))
            {
                indices[i * 12 + 4] = glm::ivec3(5, 4, 6) + glm::ivec3(i * 8);
                indices[i * 12 + 5] = glm::ivec3(5, 6, 7) + glm::ivec3(i * 8);
            }
            if (!IsOccupied(voxel_coords[i] + glm::ivec3(-1, 0, 0)))
            {
                indices[i * 12 + 6] = glm::ivec3(4, 0, 2) + glm::ivec3(i * 8);
                indices[i * 12 + 7] = glm::ivec3(4, 2, 6) + glm::ivec3(i * 8);
            }
            if (!IsOccupied(voxel_coords[i] + glm::ivec3(0, 1, 0)))
            {
                indices[i * 12 + 8] = glm::ivec3(2, 3, 6) + glm::ivec3(i * 8);
                indices[i * 12 + 9] = glm::ivec3(3, 7, 6) + glm::ivec3(i * 8);
            }
            if (!IsOccupied(voxel_coords[i] + glm::ivec3(0, -1, 0)))
            {
                indices[i * 12 + 10] = glm::ivec3(0, 4, 1) + glm::ivec3(i * 8);
                indices[i * 12 + 11] = glm::ivec3(1, 4, 5) + glm::ivec3(i * 8);
            }
        }

        // Clear out all hidden faces
        std::vector<glm::ivec3> new_indices;
        std::vector<uint32_t> new_material_indices;
        for (uint32_t i = 0; i < indices.size(); i++)
        {
            if (indices[i] != glm::ivec3(-1))
            {
                new_indices.push_back(indices[i]);
                new_material_indices.push_back(voxel_material_indices[i / 12]);
            }
        }

        #if 0
        // The above approach creates a huge amount of duplicate and unused vertices. Compact.
        std::map<glm::ivec3, uint32_t, compare_ivec3> compact_vertex_indices;
        std::vector<glm::vec3> compact_vertices;
        for (uint32_t i = 0; i < new_indices.size(); i++)
        {
            for (uint32_t j = 0; j < 3; j++)
            {
                auto &v = vertices[new_indices[i][j]];
                if (compact_vertex_indices.count(v) == 0)
                {
                    compact_vertices.push_back(v);
                    compact_vertex_indices[v] = compact_vertices.size();
                }
                new_indices[i][j] = compact_vertex_indices[v];
            }
        }
        LOG(INFO) << "From " << vertices.size() << " to " << compact_vertices.size() << " vertices.\n";
        #else
        std::vector<glm::vec3> compact_vertices;
        for (uint32_t i = 0; i < vertices.size(); i++)
            compact_vertices.push_back(vertices[i]);
        #endif

        ///////////////////////////////////////////////////////////////////////
        // There is a real problem with the zbuffer not being able to handle
        // cases like the following on borders between blocks: 
        // 
        //   a      b
        // ------+------
        //       |
        //       |  c
        // 
        // Doing a horrible hack here, where border vertices are pushed out
        // from the center of the block, which changes the geometry noticably,
        // but then I enforce "shading" normals to be axis aligned in the 
        // shader. Not my proudest moment. 
        ///////////////////////////////////////////////////////////////////////
        for (auto &v : compact_vertices)
        {
            const float radius = glm::length(glm::vec3(8.0f, 8.0f, 8.0f));
            glm::vec3 dir = v - glm::vec3(8.0f, 8.0f, 8.0f);
            const float weight = (1.0f - length(dir) / radius); 
            if (v.x == 0 || v.y == 0 || v.z == 0 || v.x == 16 || v.y == 16 || v.z == 16)
            {
                v += 0.1f * normalize(dir);
            }
        }



        LOG(INFO) << "CreateMeshFromMagicaModel: Removed " << indices.size() << " - " << new_indices.size()
                  << " = " << indices.size() - new_indices.size() << " inside faces.\n";

        return make_tuple(compact_vertices, new_material_indices, new_indices);
    }

}  // namespace magica

