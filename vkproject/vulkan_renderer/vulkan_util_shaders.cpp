#include "vulkan_util_shaders.hpp"
#include <vector>
#include <string>
#include <fstream>
#include "../log.hpp"
#include <filesystem>

std::vector<char> ReadBinaryFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        LOG(ERROR) << "Failed to open file: " << filename << ". Working dir is: " << std::filesystem::current_path()
                   << "\n";
        throw std::runtime_error("failed to open file!");
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

vk::ShaderModule CreateShaderModule(vk::Device & device, const std::vector<char>& shader_binary)
{
    vk::ShaderModuleCreateInfo create_info;
    create_info.codeSize = shader_binary.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(shader_binary.data());
    vk::ShaderModule shader_module = device.createShaderModule(create_info);
    return shader_module;
};
