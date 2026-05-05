#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>

std::vector<char> ReadBinaryFile(const std::string& filename);
vk::ShaderModule CreateShaderModule(vk::Device& device, const std::vector<char>& shader_binary);
