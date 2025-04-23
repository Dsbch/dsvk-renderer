#include <pch.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "assetManager.h"

namespace engineCore
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

	assetManager::assetManager(context ctx) : mCtx(ctx)
	{
	}

	std::pair<const std::shared_ptr<texture>, engineCore::error> assetManager::getTexture(const std::string& id)
	{
		auto it = mLoadedTextures.find(id);
		if (it != mLoadedTextures.end())
		{
			return { it->second , {} };
		}
		else {
			return { {}, {"tried to access not loaded texture."} };
		}
	}

	std::pair<const std::shared_ptr<shaderProgram>, engineCore::error> assetManager::getCompiledShader(const std::string& id)
	{
		auto it = mCompiledShaders.find(id);
		if (it != mCompiledShaders.end())
		{
			return { it->second, {} };
		}
		else {
			return { {}, {"tried to access not loaded shader."} };
		}
	}

	std::pair<const std::shared_ptr<texture>, engineCore::error> assetManager::loadTexture(const std::string& path)
	{
		int width, height, nrChannels;
		uint8_t* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
		if (!data)
		{
			return { {}, {"can't load texture"} };
		}

		auto t = std::make_shared<texture>(data, width, height, (imageChannel)nrChannels);

		stbi_image_free(data);

		mLoadedTextures[path] = t;

		return { mLoadedTextures[path], {} };
	}

	std::pair<const std::shared_ptr<shaderProgram>, engineCore::error> assetManager::loadAndCompileShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
	{
		std::string vs = openAndRead(vertexShaderPath);
		std::string fs = openAndRead(fragmentShaderPath);

		auto program = std::make_shared<shaderProgram>(fs, vs);

		auto err = program->compile();
		if (err)
		{
			return { {}, err };
		}

		mCompiledShaders[vertexShaderPath] = program;

		return { mCompiledShaders[vertexShaderPath], {} };
	}

}