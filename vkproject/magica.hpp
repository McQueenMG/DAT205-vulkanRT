#pragma once
#include <tuple>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "vulkan_renderer/shaders/common.glsl"

namespace magica
{
    std::tuple<std::vector<glm::ivec3> /* coords */, std::vector<Material> /* materials */,
               std::vector<uint32_t> /* material indices*/>
    LoadMagicaModel(const std::vector<uint8_t> &data, int model_nr);
    void RemoveInsideVoxels(std::vector<glm::ivec3> &coords, std::vector<uint32_t> &material_indices, const int block_size);
    std::tuple<std::vector<glm::vec3> /* vertices */, std::vector<uint32_t> /* material_indices */,
               std::vector<glm::ivec3> /* indices */>
    CreateMeshFromMagicaModel(const std::vector<glm::ivec3> &coords, const std::vector<uint32_t> &material_indices);
}