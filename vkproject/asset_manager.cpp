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
        uint32_t asset_id = num_loaded_vox_assets++;
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