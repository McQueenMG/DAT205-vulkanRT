#include "asset_manager.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "log.hpp"
#include "ogt_vox.hpp"

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

int ParseObjVertexIndex(const std::string& token, int vertex_count)
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

std::map<std::string, Material> LoadMtlFile(const std::filesystem::path& mtl_path)
{
    std::map<std::string, Material> materials;
    std::ifstream input(mtl_path);
    if (!input)
    {
        LOG(WARNING) << "Failed to open MTL file: " << mtl_path.string() << "\n";
        return materials;
    }

    std::string line;
    std::string current_name;
    while (std::getline(input, line))
    {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream line_stream(line);
        std::string prefix;
        line_stream >> prefix;

        if (prefix == "newmtl")
        {
            line_stream >> current_name;
            materials[current_name] = MakeDefaultMaterial();
            continue;
        }

        if (current_name.empty()) continue;

        auto& material = materials[current_name];
        if (prefix == "Kd")
        {
            line_stream >> material.color.x >> material.color.y >> material.color.z;
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
    }

    return materials;
}
}

AssetManager::VoxAsset AssetManager::LoadVoxAsset(const std::string &filename) 
{
    ///////////////////////////////////////////////////////////////////////////
    // A .vox file can contain many variations of the same asset or several 
    // frames in an animation, etc. 
    ///////////////////////////////////////////////////////////////////////////
    if (vox_asset_id_by_filename.count(filename) > 0)
    {
        return vox_asset_by_id[vox_asset_id_by_filename[filename]].second;
    }
    else
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        uint32_t asset_id = next_asset_id++;
        vox_asset_by_id[asset_id] = make_pair(filename, VoxAsset());
        VoxAsset &asset = vox_asset_by_id[asset_id].second; 
        vox_asset_by_id[asset_id].second.data.resize(size);
        vox_asset_by_id[asset_id].second.asset_id = asset_id; 
        vox_asset_id_by_filename[filename] = vox_asset_by_id[asset_id].second.asset_id;


        if (!file.read((char *)vox_asset_by_id[asset_id].second.data.data(), size))
        {
            LOG(ERROR) << "Failed to read file: " << filename << std::endl;
        }
        const ogt_vox_scene *scene = ogt_vox_read_scene(asset.data.data(), (uint32_t)asset.data.size());
        asset.num_variations = scene->num_models;
        ogt_vox_destroy_scene(scene);
        return asset;
    }
}

AssetManager::VoxAsset AssetManager::GetVoxAsset(uint32_t asset_id)
{
    if (vox_asset_by_id.count(asset_id) == 0) 
        LOG(ERROR) << "GetAsset() requested non-existing asset " << asset_id << "\n";
    return vox_asset_by_id[asset_id].second;
}

AssetManager::MeshAsset AssetManager::RegisterMeshAsset(const std::string& name, const TriangleMesh& mesh)
{
    return RegisterMeshAsset(name, std::vector<TriangleMesh>{mesh});
}

AssetManager::MeshAsset AssetManager::RegisterMeshAsset(const std::string& name, const std::vector<TriangleMesh>& variations)
{
    if (mesh_asset_id_by_name.count(name) > 0)
    {
        return mesh_asset_by_id[mesh_asset_id_by_name[name]].second;
    }

    uint32_t asset_id = next_asset_id++;
    mesh_asset_by_id[asset_id] = std::make_pair(name, MeshAsset());
    MeshAsset& asset = mesh_asset_by_id[asset_id].second;
    asset.asset_id = asset_id;
    asset.variations = variations;
    mesh_asset_id_by_name[name] = asset_id;
    return asset;
}

AssetManager::MeshAsset AssetManager::LoadObjAsset(const std::string& filename)
{
    if (mesh_asset_id_by_name.count(filename) > 0)
    {
        return mesh_asset_by_id[mesh_asset_id_by_name[filename]].second;
    }

    std::ifstream input(filename);
    if (!input)
    {
        LOG(ERROR) << "Failed to open OBJ file: " << filename << "\n";
        throw std::runtime_error("Failed to open OBJ file");
    }

    TriangleMesh mesh;
    std::map<std::string, Material> mtl_materials;
    std::map<std::string, uint32_t> mesh_material_indices_by_name;
    std::filesystem::path obj_path(filename);
    uint32_t current_material_index = 0;
    std::string line;
    while (std::getline(input, line))
    {
        if (line.empty()) continue;
        if (line[0] == '#') continue;

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

        if (prefix == "mtllib")
        {
            std::string mtl_filename;
            line_stream >> mtl_filename;
            mtl_materials = LoadMtlFile(obj_path.parent_path() / mtl_filename);
            continue;
        }

        if (prefix == "usemtl")
        {
            std::string current_material_name;
            line_stream >> current_material_name;
            if (mtl_materials.count(current_material_name) > 0)
            {
                if (mesh_material_indices_by_name.count(current_material_name) == 0)
                {
                    mesh_material_indices_by_name[current_material_name] = static_cast<uint32_t>(mesh.materials.size());
                    mesh.materials.push_back(mtl_materials[current_material_name]);
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
            std::vector<int> face_indices;
            std::string vertex_token;
            while (line_stream >> vertex_token)
            {
                const int idx = ParseObjVertexIndex(vertex_token, static_cast<int>(mesh.vertices.size()));
                if (idx < 0 || idx >= static_cast<int>(mesh.vertices.size()))
                {
                    LOG(ERROR) << "OBJ face references out-of-range vertex index in " << filename << "\n";
                    throw std::runtime_error("OBJ face index out of range");
                }
                face_indices.push_back(idx);
            }

            if (face_indices.size() < 3)
            {
                continue;
            }

            for (size_t i = 1; i + 1 < face_indices.size(); i++)
            {
                mesh.indices.push_back(glm::ivec3(face_indices[0], face_indices[i], face_indices[i + 1]));
                mesh.material_indices.push_back(current_material_index);
            }
        }
    }

    if (mesh.materials.empty())
    {
        mesh.materials.push_back(MakeDefaultMaterial());
    }

    if (mesh.vertices.empty() || mesh.indices.empty())
    {
        LOG(ERROR) << "OBJ file has no renderable geometry: " << filename << "\n";
        throw std::runtime_error("OBJ file has no renderable geometry");
    }

    return RegisterMeshAsset(filename, mesh);
}

AssetManager::MeshAsset AssetManager::GetMeshAsset(uint32_t asset_id)
{
    if (mesh_asset_by_id.count(asset_id) == 0)
        LOG(ERROR) << "GetMeshAsset() requested non-existing asset " << asset_id << "\n";
    return mesh_asset_by_id[asset_id].second;
}

bool AssetManager::HasVoxAsset(uint32_t asset_id) const
{
    return vox_asset_by_id.count(asset_id) > 0;
}

bool AssetManager::HasMeshAsset(uint32_t asset_id) const
{
    return mesh_asset_by_id.count(asset_id) > 0;
}

uint32_t AssetManager::GetAssetVariationCount(uint32_t asset_id) const
{
    if (HasVoxAsset(asset_id))
    {
        return vox_asset_by_id.at(asset_id).second.num_variations;
    }
    if (HasMeshAsset(asset_id))
    {
        return static_cast<uint32_t>(mesh_asset_by_id.at(asset_id).second.variations.size());
    }
    throw std::runtime_error("Unknown asset id");
}

AssetManager::AssetType AssetManager::GetAssetType(uint32_t asset_id) const
{
    if (HasVoxAsset(asset_id))
        return AssetType::Vox;
    if (HasMeshAsset(asset_id))
        return AssetType::TriangleMesh;
    throw std::runtime_error("Unknown asset id");
}

std::string AssetManager::ReadShaderSource(const std::string &filename) 
{
    const std::ifstream input_stream(filename, std::ios_base::binary);
    if (input_stream.fail()) {
        LOG(ERROR) << "Failed to read file: " << filename << std::endl;
    }
    std::stringstream buffer;
    buffer << input_stream.rdbuf();
    return buffer.str();
}