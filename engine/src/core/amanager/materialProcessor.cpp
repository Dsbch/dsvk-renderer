#include <pch.h>
#include "materialProcessor.h"

#include <cgltf.h>
#include <stb_image.h>

namespace engine
{
	static float toRGB(float color)
	{
		return pow(color, 2.2f);
	}

	static float toSRGB(float color)
	{
		return pow(color, 1.0f / 2.2f);
	}

	withError<imageInfo> getImageInfo(const cgltf_texture* texture, const std::filesystem::path& baseDir)
	{
		imageInfo result{};

		if (texture->image->uri)
		{
			auto relativePath = baseDir / texture->image->uri;

			if (!stbi_info(relativePath.string().c_str(), &result.w, &result.h, &result.channels))
				return error{ "can't get image info: {}", relativePath.string() };
		}
		else if (auto bufferView = texture->image->buffer_view; bufferView && bufferView->buffer->data && bufferView->size != 0)
		{
			uint8_t* ptr = static_cast<uint8_t*>(bufferView->buffer->data);
			ptr += bufferView->offset;

			if (!stbi_info_from_memory(ptr, int(bufferView->size), &result.w, &result.h, &result.channels))
				return error{ "can't get image info" };
		}

		return result;
	}

	withError<image> processTexture(const cgltf_texture* texture, const std::filesystem::path& baseDir)
	{
		image img{};

		uint8_t* data = nullptr;

		if (texture->image->uri)
		{
			auto relativePath = baseDir / texture->image->uri;

			int factChannels = 0;

			data = stbi_load(relativePath.string().c_str(), &img.w, &img.h, &factChannels, 4);
			if (!data)
				return error{ "can't load texture with path: {}", relativePath.string() };

			img.channels = 4;
		}
		else if (auto bufferView = texture->image->buffer_view; bufferView && bufferView->buffer->data && bufferView->size != 0)
		{
			uint8_t* ptr = static_cast<uint8_t*>(bufferView->buffer->data);
			ptr += bufferView->offset;

			int factChannels = 0;

			data = stbi_load_from_memory(ptr, int(bufferView->size), &img.w, &img.h, &factChannels, 4);
			if (!data)
				return error{ "can't load texture" };

			img.channels = 4;
		}

		img.padding = std::max(img.w, img.h) / 128;

		img.data.resize(img.w * img.h * img.channels);

		std::memcpy(img.data.data(), data, img.w * img.h * img.channels);

		stbi_image_free(data);

		return img;
	}

	error applyBaseFactor(image& img, float factor[4])
	{
		if (img.channels != 4)
			return error{ "applyBaseFactor: not RGBA" };

		if (factor[0] == 1.0f && factor[1] == 1.0f && factor[2] == 1.0f && factor[3] == 1.0f)
			return error{};

		auto ptr = img.data.data();
		for (int i = 0; i < img.h * img.w * img.channels; i += img.channels)
		{
			ptr[0] = uint8_t(toSRGB(toRGB(ptr[0] / 255.0f) * factor[0]) * 255.0f);
			ptr[1] = uint8_t(toSRGB(toRGB(ptr[1] / 255.0f) * factor[1]) * 255.0f);
			ptr[2] = uint8_t(toSRGB(toRGB(ptr[2] / 255.0f) * factor[2]) * 255.0f);
			ptr[3] = uint8_t(ptr[3] / 255.0f * factor[3] * 255.0f);

			ptr += img.channels;
		}

		return {};
	}

	void applyMetallicRoughnessFactor(image& img, float metallic, float roughness)
	{
		if (metallic == 1.0f && roughness == 1.0f)
			return;

		auto ptr = img.data.data();
		for (int i = 0; i < img.h * img.w * img.channels; i += img.channels)
		{
			ptr[1] = uint8_t(toSRGB(toRGB(ptr[1] / 255.0f) * roughness) * 255.0f);
			ptr[2] = uint8_t(toSRGB(toRGB(ptr[2] / 255.0f) * metallic) * 255.0f);

			ptr += img.channels;
		}
	}

	withError<std::vector<textures>> processMaterials(const std::filesystem::path& baseDir, const cgltf_material* materialsPtr, int materialCount)
	{
		std::vector<textures> result;

		for (int i = 0; i < materialCount; i++)
		{
			const cgltf_material* material = materialsPtr + i;

			if (material->has_pbr_metallic_roughness)
			{
				// In case if all materials doesn't have textures.
				int w = 64, h = 64, padding = 0;
				if (auto metalicRoughnessTexture = material->pbr_metallic_roughness.metallic_roughness_texture.texture; metalicRoughnessTexture && metalicRoughnessTexture->image)
				{
					auto info = getImageInfo(metalicRoughnessTexture, baseDir);
					if (!info)
						return info.err();

					w = info.value().w, h = info.value().h;
					padding = std::max(w, h) / 128;
				}
				else if (auto albedoTexture = material->pbr_metallic_roughness.base_color_texture.texture; albedoTexture && albedoTexture->image)
				{
					auto info = getImageInfo(albedoTexture, baseDir);
					if (!info)
						return info.err();

					w = info.value().w, h = info.value().h;
					padding = std::max(w, h) / 128;
				}
				else if (auto normalTexture = material->normal_texture.texture; normalTexture && normalTexture->image)
				{
					auto info = getImageInfo(normalTexture, baseDir);
					if (!info)
						return info.err();

					w = info.value().w, h = info.value().h;
					padding = std::max(w, h) / 128;
				}

				float albedoFactor[4] = {
					material->pbr_metallic_roughness.base_color_factor[0],
					material->pbr_metallic_roughness.base_color_factor[1],
					material->pbr_metallic_roughness.base_color_factor[2],
					material->pbr_metallic_roughness.base_color_factor[3],
				};

				image albedo{};
				image normal{};
				image metallicRoughness{};
				textures tex{};

				// albedo.
				if (auto albedoTexture = material->pbr_metallic_roughness.base_color_texture.texture; albedoTexture && albedoTexture->image)
				{
					auto rawTexture = processTexture(albedoTexture, baseDir);
					if (!rawTexture)
						return rawTexture.err();

					error err = applyBaseFactor(rawTexture.value(), albedoFactor);
					if (err)
						return err;

					albedo = rawTexture.value();
				}
				else
				{
					albedo = image{
						.data = {},
						.w = w,
						.h = h,
						.padding = padding,
						.channels = 4,
					};
					albedo.data.resize(albedo.w * albedo.h * albedo.channels);

					auto ptr = albedo.data.begin();
					for (int y = 0; y < albedo.h; y++)
					{
						for (int w = 0; w < albedo.w; w++)
						{
							ptr[0] = uint8_t(toSRGB(albedoFactor[0]) * 255.0f);
							ptr[1] = uint8_t(toSRGB(albedoFactor[1]) * 255.0f);
							ptr[2] = uint8_t(toSRGB(albedoFactor[2]) * 255.0f);
							ptr[3] = uint8_t(albedoFactor[3] * 255.0f);

							ptr += 4;
						}
					}
				}

				// metallic-roughness.
				if (auto metalicRoughnesTexture = material->pbr_metallic_roughness.metallic_roughness_texture.texture; metalicRoughnesTexture && metalicRoughnesTexture->image)
				{
					auto rawTexture = processTexture(metalicRoughnesTexture, baseDir);
					if (!rawTexture)
						return rawTexture.err();

					applyMetallicRoughnessFactor(rawTexture.value(), material->pbr_metallic_roughness.metallic_factor, material->pbr_metallic_roughness.roughness_factor);

					metallicRoughness = rawTexture.value();
				}
				else
				{
					metallicRoughness = image{
						.data = {},
						.w = w,
						.h = h,
						.padding = padding,
						.channels = 4,
					};
					metallicRoughness.data.resize(metallicRoughness.w * metallicRoughness.h * metallicRoughness.channels);

					auto ptr = metallicRoughness.data.begin();
					for (int y = 0; y < metallicRoughness.h; y++)
					{
						for (int w = 0; w < metallicRoughness.w; w++)
						{
							ptr[0] = 0;
							ptr[1] = uint8_t(toSRGB(material->pbr_metallic_roughness.roughness_factor) * 255.0f);
							ptr[2] = uint8_t(toSRGB(material->pbr_metallic_roughness.metallic_factor) * 255.0f);
							ptr[3] = 0;

							ptr += 4;
						}
					}
				}

				// normal.
				if (auto normalTexture = material->normal_texture.texture; normalTexture && normalTexture->image)
				{
					auto rawTexture = processTexture(normalTexture, baseDir);
					if (!rawTexture)
						return rawTexture.err();

					normal = rawTexture.value();
				}
				else
				{
					normal = image{
						.data = {},
						.w = w,
						.h = h,
						.padding = padding,
						.channels = 4,
					};
					normal.data.resize(normal.w * normal.h * normal.channels);

					auto ptr = normal.data.begin();
					for (int y = 0; y < normal.h; y++)
					{
						for (int w = 0; w < normal.w; w++)
						{
							ptr[0] = 128;
							ptr[1] = 128;
							ptr[2] = 255;
							ptr[3] = 0;

							ptr += 4;
						}
					}
				}

				tex.albedo = albedo;
				tex.normal = normal;
				tex.metallicRoughness = metallicRoughness;

				result.push_back(tex);
			}
			else
				return error{ "metalicRoughnes texture isn't defined for model: {}", baseDir.string() };
		}

		return result;
	}
}