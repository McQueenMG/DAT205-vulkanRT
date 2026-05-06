#include "asset_manager.hpp"
#include <fstream>
#include <sstream>
#include "log.hpp"
#include "ogt_vox.hpp"

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