#pragma once

#include <pch.h>

struct cgltf_texture;
struct cgltf_material;

namespace engine
{
	struct imageInfo
	{
		int w, h, channels;
	};
	
	struct image
	{
		std::vector<uint8_t> data;
		int w, h;
		int padding;
		int channels;
	};

	struct textures
	{
		image albedo;
		image normal;
		image metallicRoughness;
	};

	withError<imageInfo> getImageInfo(const cgltf_texture* texture, const std::filesystem::path& baseDir);

	withError<image> processTexture(const cgltf_texture* texture, const std::filesystem::path& baseDir);
	
	error applyBaseFactor(image& img, float factor[4]);

	void applyMetallicRoughnessFactor(image& img, float metallic, float roughness);

	withError<std::vector<textures>> processMaterials(const std::filesystem::path& baseDir, const cgltf_material* materialsPtr, int materialCount);
}