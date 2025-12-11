#include <pch.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "aManager.h"
#include "gltf.h"
#include "platform/renderer/renderer.h"


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

	withError<std::shared_ptr<texture>> aManager::getTexture(const std::string& path)
	{
		auto it = mLoadedTextures.find(key(path));
		if (it != mLoadedTextures.end())
			return it->second;
		else 
			return error{"tried to access not loaded texture."};
	}

	withError<std::shared_ptr<shader>> aManager::getShader(const std::string& path)
	{
		auto it = mLoadedShaders.find(key(path));
		if (it != mLoadedShaders.end())
			return it->second;
		else
			return error{ "tried to access not loaded texture." };
	}

	withError<std::shared_ptr<shader>> aManager::loadShader(const std::string& path)
	{
		if (!makeShader)
			return error{ "makeShader wasn't set" };

		auto loadRes = getShader(path);
		if (loadRes)
			return loadRes.value();

		std::ifstream file(path, std::ios::ate | std::ios::binary);
		if (!file.is_open()) 
			return error{"can't open file {}", path};

		size_t fileSize = (size_t)file.tellg();
		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);
		file.read((char*)buffer.data(), fileSize);
		file.close();

		auto shader = makeShader(buffer);
		if (!shader)
			shader.err();

		mLoadedShaders[key(path)] = shader.value();

		return shader.value();
	}

	withError<std::shared_ptr<texture>> aManager::loadTexture(const std::string& path)
	{
		if (!makeTexture)
			return error{ "makeTexture wasn't set" };

		auto loadRes = getTexture(path);
		if (loadRes)
			return loadRes.value();

		int width, height, nrChannels;
		uint8_t* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
		if (!data)
		{
			return error{ "can't load texture {}", path };;
		}

		auto texture = makeTexture(data, width, height, intToChannel(nrChannels));
		if (!texture)
			texture.err();

		stbi_image_free(data);

		mLoadedTextures[key(path)] = texture.value();

		return texture.value();
	}

	void aManager::setMakeShaderFunc(std::function<withError<std::shared_ptr<shader>>(const std::vector<uint32_t>& src)>&& func)
	{
		makeShader = std::move(func);
	}

	void aManager::setMakeTextureFunc(std::function<withError<std::shared_ptr<texture>>(uint8_t* data, int width, int heigth, imageChannel channel)>&& func)
	{
		makeTexture = std::move(func);
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

	withError<std::shared_ptr<shader>> aManager::getDefaultComputeShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkCompute.spv";

		return loadShader(path);
#endif // VULKAN

		return error{"not implemented"};
	}
}