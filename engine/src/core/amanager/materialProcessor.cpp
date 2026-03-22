#include <pch.h>
#include "materialProcessor.h"

#include <cgltf.h>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <basisu_transcoder.h>

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

	withError<imageWithMipLevels> generateMipLevels(const image& img)
	{
		imageWithMipLevels result{
			.main = img,
		};

		if (img.w <= 1 && img.h <= 1)
			return result;

		if (img.compressed)
			return result;

		uint32_t numMips = img.mipLevels();
		result.mipLevels.reserve(numMips - 1);

		const uint8_t* srcData = img.data.data();
		int srcWidth = img.w;
		int srcHeight = img.h;

		int mipWidth = img.w;
		int mipHeight = img.h;
		for (uint32_t i = 1; i < numMips; i++)
		{
			mipWidth = std::max(1, mipWidth / 2);
			mipHeight = std::max(1, mipHeight / 2);

			image mip{};
			mip.w = mipWidth;
			mip.h = mipHeight;
			mip.channels = img.channels;
			mip.compressed = false;
			mip.data.resize(mip.w * mip.h * mip.channels);

			uint8_t* resizeRes = stbir_resize_uint8_linear(
				srcData, srcWidth, srcHeight, 0,
				mip.data.data(), mipWidth, mipHeight, 0,
				(stbir_pixel_layout)img.channels
			);
			if (!resizeRes)
				return error{ "generateMipLevels stbir_resize_uint8_linear err" };

			result.mipLevels.push_back(std::move(mip));
		}

		return result;
	}

	withError<std::vector<mippedImages>> generateMipLevels(const std::vector<images>& images)
	{
		std::vector<mippedImages> result{};

		for (auto& imgs : images)
		{
			mippedImages crnt{};

			auto albedo = generateMipLevels(imgs.albedo);
			if (!albedo)
				return albedo.err();

			auto normal = generateMipLevels(imgs.normal);
			if (!normal)
				return normal.err();

			auto metallicRoughness = generateMipLevels(imgs.metallicRoughness);
			if (!metallicRoughness)
				return metallicRoughness.err();

			crnt.albedo = albedo.value();
			crnt.normal = normal.value();
			crnt.metallicRoughness = metallicRoughness.value();

			result.push_back(std::move(crnt));
		}

		return result;
	}

	image compressTextureBC7(const image& img)
	{
		image result{
			.channels = img.channels,
			.compressed = true,
		};

		result.w = img.w;
		result.h = img.h;

		int blocksX = (img.w + 3) / 4;
		int blocksY = (img.h + 3) / 4;
		int totalBlocks = blocksX * blocksY;

		result.data.resize(totalBlocks * 16);

		unsigned int srcStride = img.w * img.channels;
		basist::color_rgba pixels[16];

		int blockIndex = 0;
		for (int by = 0; by < img.h; by += 4)
		{
			for (int bx = 0; bx < img.w; bx += 4)
			{
				for (int py = 0; py < 4; py++)
				{
					for (int px = 0; px < 4; px++)
					{
						int srcX = bx + px;
						int srcY = by + py;

						int pixelIndex = py * 4 + px;

						if (srcX < img.w && srcY < img.h)
						{
							const uint8_t* srcPixel = img.data.data() + (srcY * srcStride) + (srcX * 4);
							pixels[pixelIndex].r = srcPixel[0];
							pixels[pixelIndex].g = srcPixel[1];
							pixels[pixelIndex].b = srcPixel[2];
							pixels[pixelIndex].a = srcPixel[3];
						}
						else
						{
							pixels[pixelIndex].set(0, 0, 0, 255);
						}
					}
				}

				uint8_t* cmpBlock = &result.data[blockIndex * 16];

				basist::bc7f::fast_pack_bc7_auto_rgba(cmpBlock, pixels, basist::bc7f::cPackBC7FlagDefault);

				blockIndex++;
			}
		}

		return result;
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

	image generateNormalImage(int w, int h, int ch)
	{
		image result = {
			.data = {},
			.w = w,
			.h = h,
			.channels = ch,
		};

		result.data.resize(result.w * result.h * result.channels);

		auto ptr = result.data.begin();
		for (int y = 0; y < result.h; y++)
		{
			for (int w = 0; w < result.w; w++)
			{
				ptr[0] = 128;
				ptr[1] = 128;
				ptr[2] = 255;
				ptr[3] = 255;

				ptr += 4;
			}
		}

		return result;
	}

	image generateAlbedoImage(int w, int h, int ch, const float albedoFactor[4])
	{
		image result = {
			.data = {},
			.w = w,
			.h = h,
			.channels = ch,
		};
		result.data.resize(result.w * result.h * result.channels);

		auto ptr = result.data.begin();
		for (int y = 0; y < result.h; y++)
		{
			for (int w = 0; w < result.w; w++)
			{
				ptr[0] = uint8_t(toSRGB(albedoFactor[0]) * 255.0f);
				ptr[1] = uint8_t(toSRGB(albedoFactor[1]) * 255.0f);
				ptr[2] = uint8_t(toSRGB(albedoFactor[2]) * 255.0f);
				ptr[3] = uint8_t(albedoFactor[3] * 255.0f);

				ptr += 4;
			}
		}

		return result;
	}

	image generateMetallicRoughnessImage(int w, int h, int ch, float metallicFactor, float roughnessFactor)
	{
		image result = {
			.data = {},
			.w = w,
			.h = h,
			.channels = ch,
		};
		result.data.resize(result.w * result.h * result.channels);

		auto ptr = result.data.begin();
		for (int y = 0; y < result.h; y++)
		{
			for (int w = 0; w < result.w; w++)
			{
				ptr[0] = 0;
				ptr[1] = uint8_t(toSRGB(roughnessFactor) * 255.0f);
				ptr[2] = uint8_t(toSRGB(metallicFactor) * 255.0f);
				ptr[3] = 0;

				ptr += 4;
			}
		}

		return result;
	}

	images genDefaultMaterial(int w, int h, int ch)
	{
		constexpr float albedoFactor[4] = {0.7f, 0.5f, 0.45f, 1.0f};

		images result{
			.albedo = generateAlbedoImage(w, h, ch, albedoFactor),
			.normal = generateNormalImage(w, h, ch),
			.metallicRoughness = generateMetallicRoughnessImage(w, h, ch, 0.5f, 0.5f),
		};

		return result;
	};

	withError<std::vector<images>> processMaterials(
		const std::filesystem::path& baseDir,
		const cgltf_material* materialsPtr,
		int materialCount
	)
	{
		std::vector<images> result;

		if (materialsPtr == nullptr || materialCount <= 0)
		{
			result.push_back(genDefaultMaterial(1024, 1024, 4));

			return result;
		}

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
				images tex{};

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
					albedo = generateAlbedoImage(w, h, 4, albedoFactor);
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
					metallicRoughness = generateMetallicRoughnessImage(w, h, 4, material->pbr_metallic_roughness.metallic_factor, material->pbr_metallic_roughness.roughness_factor);
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
					normal = generateNormalImage(w, h, 4);
				}

				// oclussion, put in normal aplha channel.
				if (auto oclussionTexture = material->occlusion_texture.texture; oclussionTexture && oclussionTexture->image)
				{
					auto rawTexture = processTexture(oclussionTexture, baseDir);
					if (!rawTexture)
						return rawTexture.err();

					auto normalPtr = normal.data.begin();
					auto oclussionPtr = rawTexture.value().data.begin();
					for (int y = 0; y < normal.h; y++)
					{
						for (int w = 0; w < normal.w; w++)
						{
							normalPtr[3] = oclussionPtr[0];

							normalPtr += 4;
							oclussionPtr += 4;
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