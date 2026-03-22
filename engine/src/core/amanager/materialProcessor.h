#pragma once

#include <pch.h>
#include "platform/renderer/texture.h"

struct cgltf_texture;
struct cgltf_material;

namespace engine
{
	struct imageInfo
	{
		int w, h, channels;
	};
	
	struct images
	{
		image albedo;
		image normal;
		image metallicRoughness;
	};

	struct mippedImages
	{
		imageWithMipLevels albedo;
		imageWithMipLevels normal;
		imageWithMipLevels metallicRoughness;
	};

	withError<imageWithMipLevels> generateMipLevels(const image& img);
	withError<std::vector<mippedImages>> generateMipLevels(const std::vector<images>& images);

	image compressTextureBC7(const image& img);

	withError<imageInfo> getImageInfo(const cgltf_texture* texture, const std::filesystem::path& baseDir);

	withError<image> processTexture(const cgltf_texture* texture, const std::filesystem::path& baseDir);
	
	error applyBaseFactor(image& img, float factor[4]);

	void applyMetallicRoughnessFactor(image& img, float metallic, float roughness);


	image generateNormalImage(int w, int h, int ch);
	image generateAlbedoImage(int w, int h, int ch, const float albedoFactor[4]);
	image generateMetallicRoughnessImage(int w, int h, int ch, float metallicFactor, float roughnessFactor);
	images genDefaultMaterial(int w, int h, int ch);

	withError<std::vector<images>> processMaterials(const std::filesystem::path& baseDir, const cgltf_material* materialsPtr, int materialCount);
}