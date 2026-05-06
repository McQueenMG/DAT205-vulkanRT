#pragma once
#include <string>
#include <map>
#include <vector>
#include "magica.hpp"

#include "triangle_asset.hpp"

///////////////////////////////////////////////////////////////////////////////
// Currently mostly a stub, the asset manager is going to be the one-stop-shop 
// for loading resources for the game. 
///////////////////////////////////////////////////////////////////////////////

struct AssetManager
{
    enum class AssetType
    {
        Vox,
        TriangleMesh,
    };

    using TriangleMesh = triangle_asset::TriangleMesh;

    ///////////////////////////////////////////////////////////////////////////
    // Vox assets are voxel grids loaded from magicavoxel files
    ///////////////////////////////////////////////////////////////////////////
    struct VoxAsset
    {
        uint32_t asset_id; 
        uint32_t num_variations; 
        std::vector<uint8_t> data; 
    };

    struct MeshAsset
    {
        uint32_t asset_id;
        std::vector<TriangleMesh> variations;
    };

    uint32_t next_asset_id = 0;
    std::map<uint32_t, std::pair<std::string, VoxAsset>> vox_asset_by_id;
    std::map<uint32_t, std::pair<std::string, MeshAsset>> mesh_asset_by_id;
    std::map<std::string, uint32_t> vox_asset_id_by_filename; 
    std::map<std::string, uint32_t> mesh_asset_id_by_name;

    VoxAsset LoadVoxAsset(const std::string &filename);
    VoxAsset GetVoxAsset(uint32_t asset_id);

    // Register triangle meshes directly (for imported or procedural assets).
    MeshAsset RegisterMeshAsset(const std::string& name, const TriangleMesh& mesh);
    MeshAsset RegisterMeshAsset(const std::string& name, const std::vector<TriangleMesh>& variations);
    MeshAsset GetMeshAsset(uint32_t asset_id);
    bool HasVoxAsset(uint32_t asset_id) const;
    bool HasMeshAsset(uint32_t asset_id) const;
    uint32_t GetAssetVariationCount(uint32_t asset_id) const;
    AssetType GetAssetType(uint32_t asset_id) const;

    ///////////////////////////////////////////////////////////////////////////
    // Shaders could also be more incorporated into the asset manager, but 
    // for now all we have is a little text reader function
    ///////////////////////////////////////////////////////////////////////////
    std::string ReadShaderSource(const std::string & filename);
};

extern AssetManager * asset_manager; 