#include <pch.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "aManager.h"
#include "platform/renderer/texture.h"
#include "platform/renderer/shader.h"
#include "platform/renderer/rendererFactory.h"

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

	std::pair<const std::shared_ptr<texture>, engine::error> aManager::getTexture(const std::string& path)
	{
		auto it = mLoadedTextures.find(std::filesystem::canonical(path).string());
		if (it != mLoadedTextures.end())
		{
			return { it->second , {} };
		}
		else {
			return { {}, {"tried to access not loaded texture."} };
		}
	}

	std::pair<const std::shared_ptr<texture>, engine::error> aManager::loadTexture(const std::string& path)
	{
		auto cachedTexure = getTexture(path);
		if (!cachedTexure.second)
			return cachedTexure;

		int width, height, nrChannels;
		uint8_t* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
		if (!data)
		{
			return { {}, {"can't load texture"} };
		}

		auto t = rendererFactory::createTexure(data, width, height, (imageChannel)nrChannels);

		stbi_image_free(data);

		auto key = std::filesystem::canonical(path).string();
		mLoadedTextures[key] = t;

		return { mLoadedTextures[key], {} };
	}

	std::pair<const std::shared_ptr<shaderProgram>, error> aManager::getShader(const std::string& vertexPath, const std::string& fragmentPath)
	{
		auto it = mCompiledShaders.find(std::array<std::string, 2>{std::filesystem::canonical(vertexPath).string(), std::filesystem::canonical(fragmentPath).string()});
		if (it != mCompiledShaders.end())
		{
			return { it->second, {} };
		}
		else {
			return { {}, {"tried to access not loaded shader."} };
		}
	}

	std::pair<const std::shared_ptr<shaderProgram>, engine::error> aManager::loadShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
	{
		auto cachedShader = getShader(vertexShaderPath, fragmentShaderPath);
		if (!cachedShader.second)
			return cachedShader;

		std::string vs = openAndRead(vertexShaderPath);
		std::string fs = openAndRead(fragmentShaderPath);

		auto program = rendererFactory::createShader(fs, vs);

		auto err = program->compile();
		if (err)
		{
			return { {}, err };
		}

		auto keyVertex = std::filesystem::canonical(vertexShaderPath).string();
		auto keyFragment = std::filesystem::canonical(fragmentShaderPath).string();

		mCompiledShaders[std::array<std::string, 2>{keyVertex, keyFragment}] = program;

		return { mCompiledShaders[std::array<std::string, 2>{keyVertex, keyFragment}], {} };
	}

	std::pair<const std::shared_ptr<cubeMap>, error> aManager::getCubeMap(const std::array<std::string, 6> path)
	{
		std::array<std::string, 6> genericPath;
		for (int i = 0; i < path.size(); i++)
			genericPath[i] = std::filesystem::canonical(path[i]).string();

		auto it = mLoadedCubeMaps.find(genericPath);
		if (it != mLoadedCubeMaps.end())
		{
			return { it->second, {} };
		}
		else {
			return { {}, {"tried to access not loaded cubemap."} };
		}
	}


	std::pair<const std::shared_ptr<cubeMap>, error> aManager::loadCubeMap(const std::array<std::string, 6> path)
	{
		auto cachedCubeMap = getCubeMap(path);
		if (!cachedCubeMap.second)
			return cachedCubeMap;

		std::array<uint8_t*, 6> cubeMaps;

		int width, height, nrChannels;
		for (int i = 0; i < path.size(); i++)
		{
			uint8_t* data = stbi_load(path[i].c_str(), &width, &height, &nrChannels, 0);
			if (!data)
			{
				return { {}, {"can't load texture"} };
			}

			cubeMaps[i] = data;
		}

		auto t = rendererFactory::createCubeMap(cubeMaps, width, height, (imageChannel)nrChannels);

		for (auto& cm : cubeMaps)
		{
			stbi_image_free(cm);
		}

		std::array<std::string, 6> genericPath;
		for (int i = 0; i < path.size(); i++)
			genericPath[i] = std::filesystem::canonical(path[i]).string();

		mLoadedCubeMaps[genericPath] = t;

		return { mLoadedCubeMaps[genericPath], {} };
	}
}