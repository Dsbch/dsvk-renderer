#include <pch.h>

#include "aManager.h"
#include "platform/renderer/renderer.h"
#include "materialProcessor.h"
#include "meshletProcessor.h"

#define CGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <cgltf.h>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <basisu_transcoder.h>

namespace engine
{
	static std::string openAndRead(const std::string& name)
	{
		std::ifstream f(name, std::ifstream::in);

		std::stringstream s;

		std::string line;
		while (f)
		{

			if (std::getline(f, line))
			{
				s << line << std::endl;
			}
		}

		f.close();

		return s.str();
	}

	static uint32_t key(const std::string& path)
	{
		return crc32(reinterpret_cast<const uint8_t*>(std::filesystem::canonical(path).string().data()), std::filesystem::canonical(path).string().size());
	}

	withError<std::shared_ptr<shader>> aManager::loadShader(const std::string& path)
	{
		if (!makeShader)
			return error{ "makeShader wasn't set" };

		{
			std::lock_guard l{ mShaderMu };
			auto loadRes = mLoadedShaders.get(key(path));
			if (loadRes)
				return loadRes.value();
		}

		std::ifstream file(path, std::ios::ate | std::ios::binary);
		if (!file.is_open())
			return error{ "can't open file {}", path };

		size_t fileSize = (size_t)file.tellg();
		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);
		file.read((char*)buffer.data(), fileSize);
		file.close();

		auto shader = makeShader(buffer);
		if (!shader)
			shader.err();

		{
			std::lock_guard l{ mShaderMu };
			mLoadedShaders.put(key(path), shader.value());
		}

		return shader.value();
	}

	withError<std::shared_ptr<texture>> aManager::loadTexture(const image& img)
	{
		if (!makeTexture)
			return error{ "makeTexture wasn't set" };

		{
			std::lock_guard l{ mShaderMu };
			auto loadRes = mLoadedTextures.get(img.hash());
			if (loadRes)
				return loadRes.value();
		}

		auto texture = makeTexture(img);
		if (!texture)
			return texture.err();

		{
			std::lock_guard l{ mShaderMu };
			mLoadedTextures.put(img.hash(), texture.value());
		}

		return texture.value();
	}

	withError<std::shared_ptr<texture>> aManager::loadTexture(const imageWithMipLevels& img)
	{
		if (!makeTextureWithMips)
			return error{ "makeTextureWithMips wasn't set" };

		{
			std::lock_guard l{ mShaderMu };
			auto loadRes = mLoadedTextures.get(img.main.hash());
			if (loadRes)
				return loadRes.value();
		}

		auto texture = makeTextureWithMips(img);
		if (!texture)
			return texture.err();

		{
			std::lock_guard l{ mShaderMu };
			mLoadedTextures.put(img.main.hash(), texture.value());
		}

		return texture.value();
	}

	void aManager::clearCache()
	{
		std::lock_guard l1{ mShaderMu };

		mLoadedShaders.clear();
		mLoadedModels.clear();
		mLoadedTextures.clear();
	}

	aManager::aManager()
		: mLoadedModels(50), mLoadedShaders(100), mLoadedTextures(100)
	{
		basist::basisu_transcoder_init();
	}

	void aManager::setMakeShaderFunc(std::function<withError<std::shared_ptr<shader>>(const std::vector<uint32_t>& src)>&& func)
	{
		makeShader = std::move(func);
	}

	void aManager::setMakeTextureFunc(std::function<withError<std::shared_ptr<texture>>(const image& img)>&& func)
	{
		makeTexture = std::move(func);
	}

	void aManager::setMakeTextureWithMipsFunc(std::function<withError<std::shared_ptr<texture>>(const imageWithMipLevels& img)>&& func)
	{
		makeTextureWithMips = std::move(func);
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultCompositeTaskShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshCompositeAs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultCompositeMeshShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshCompositeMs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultCompositePixelShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshCompositePs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultAccumilateTaskShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshAccumilationAs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultAccumilateMeshShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshAccumilationMs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultAccumilatePixelShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshAccumilationPs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultTaskShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshAs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultMeshShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshMs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultPixelShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshPs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultLineVertexShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkLineVs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultLinePixelShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkLinePs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<model> aManager::loadModelGLTF(
		const std::string& path,
		size_t maxVert,
		size_t maxTriangles,
		float coneWieght,
		float errorLevel
	)
	{
		{
			std::lock_guard l{ mModelMu };

			auto found = mLoadedModels.get(key(path));
			if (found)
				return found.value();
		}

		std::filesystem::path baseDir = std::filesystem::path{ path }.parent_path();

		cgltf_options options{};
		cgltf_data* data = nullptr;

		if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
			return error{ "can't open file {}", path };

		if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
		{
			cgltf_free(data);
			return error{ "cgltf_load_buffers: {}", path };
		}

		LOGDEBUG("Loading model: {}", path.c_str());

		model result{
			.id = genUID(),
		};

		auto meshes = processMeshes(data, maxVert, maxTriangles, coneWieght, errorLevel);
		if (!meshes)
			return meshes.err();

		result.meshData = meshes.value();

		auto materials = processMaterials(baseDir, data->materials, int(data->materials_count));
		if (!materials)
			return materials.err();

		// Generate mip levels.
		auto mippedImages = generateMipLevels(materials.value());
		if (!mippedImages)
			return mippedImages.err();

		// Compress textures to BC7.
		for (auto& t : mippedImages.value())
		{
			t.albedo.main = compressTextureBC7(t.albedo.main);

			for (int i = 0; i < t.albedo.mipLevels.size(); i++)
				t.albedo.mipLevels[i] = compressTextureBC7(t.albedo.mipLevels[i]);

			t.normal.main = compressTextureBC7(t.normal.main);

			for (int i = 0; i < t.normal.mipLevels.size(); i++)
				t.normal.mipLevels[i] = compressTextureBC7(t.normal.mipLevels[i]);

			t.metallicRoughness.main = compressTextureBC7(t.metallicRoughness.main);

			for (int i = 0; i < t.metallicRoughness.mipLevels.size(); i++)
				t.metallicRoughness.mipLevels[i] = compressTextureBC7(t.metallicRoughness.mipLevels[i]);
		}

		for (auto& t : mippedImages.value())
		{
			materialTextures tx{};

			auto albedo = loadTexture(t.albedo);
			if (!albedo)
				return albedo.err();

			tx.albedo = albedo.value();

			auto normal = loadTexture(t.normal);
			if (!normal)
				return normal.err();

			tx.normal = normal.value();

			auto metallicRoughness = loadTexture(t.metallicRoughness);
			if (!metallicRoughness)
				return metallicRoughness.err();

			tx.metallicRoughness = metallicRoughness.value();

			result.mat.textures.push_back(tx);
		}

		result.mat.generateHash();

		cgltf_free(data);

		{
			std::lock_guard l{ mModelMu };

			mLoadedModels.put(key(path), result);
		}

		return result;
	}
}
