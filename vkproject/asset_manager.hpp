#pragma once
#include <string>
#include <map>
#include "magica.hpp"
#include <glm/glm.hpp>

///////////////////////////////////////////////////////////////////////////////
// Currently mostly a stub, the asset manager is going to be the one-stop-shop 
// for loading resources for the game. 
///////////////////////////////////////////////////////////////////////////////

struct AssetManager
{
    ///////////////////////////////////////////////////////////////////////////
    // Vox assets are voxel grids loaded from magicavoxel files
    ///////////////////////////////////////////////////////////////////////////
    struct VoxAsset
    {
        uint32_t asset_id; 
        uint32_t num_variations; 
        std::vector<uint8_t> data; 
    };
    uint32_t num_loaded_vox_assets = 0;
    std::map<uint32_t, std::pair<std::string, VoxAsset>> vox_asset_by_id;
    std::map<std::string, uint32_t> vox_asset_id_by_filename; 
    VoxAsset LoadVoxAsset(const std::string &filename);
    VoxAsset GetVoxAsset(uint32_t asset_id);

    ///////////////////////////////////////////////////////////////////////////
    // Shaders could also be more incorporated into the asset manager, but 
    // for now all we have is a little text reader function
    ///////////////////////////////////////////////////////////////////////////
    std::string ReadShaderSource(const std::string & filename);
};

extern AssetManager * asset_manager; 