// This translation unit implements stb_image and LoadImage

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb/stb_image.h"

#include "triangle_asset.hpp"
#include <filesystem>
#include "log.hpp"

namespace triangle_asset
{

TextureMap LoadImage(const std::filesystem::path& image_path)
{
	TextureMap texture;
	if (!std::filesystem::exists(image_path))
	{
		LOG(WARNING) << "Texture file not found: " << image_path.string() << "\n";
		return texture;
	}

	int width, height, channels;
	stbi_uc* pixels = stbi_load(image_path.string().c_str(), &width, &height, &channels, 4);
	if (!pixels)
	{
		LOG(ERROR) << "Failed to load image: " << image_path.string() << " (" << stbi_failure_reason() << ")\n";
		return texture;
	}

	// Copy pixel data
	const size_t pixel_count = width * height * 4;
	texture.data.assign(pixels, pixels + pixel_count);
	texture.width = width;
	texture.height = height;
	texture.channels = 4;  // Always RGBA after stbi_load with 4 channel request
	texture.source_path = image_path.string();

	stbi_image_free(pixels);

	LOG(INFO) << "Loaded texture: " << image_path.string() << " (" << width << "x" << height << ")\n";
	return texture;
}

}  // namespace triangle_asset

