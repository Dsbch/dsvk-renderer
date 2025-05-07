#include <pch.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "aManager.h"

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
		auto it = mLoadedTextures.find(path);
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

	std::pair<const std::shared_ptr<shaderProgram>, error> aManager::getCompiledShader(const std::string& vertexPath, const std::string& fragmentPath)
	{
		auto it = mCompiledShaders.find(vertexPath + fragmentPath);
		if (it != mCompiledShaders.end())
		{
			return { it->second, {} };
		}
		else {
			return { {}, {"tried to access not loaded shader."} };
		}
	}

	std::pair<const std::shared_ptr<shaderProgram>, engine::error> aManager::loadAndCompileShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
	{
		std::string vs = openAndRead(vertexShaderPath);
		std::string fs = openAndRead(fragmentShaderPath);

		auto program = std::make_shared<shaderProgram>(fs, vs);

		auto err = program->compile();
		if (err)
		{
			return { {}, err };
		}

		mCompiledShaders[vertexShaderPath + fragmentShaderPath] = program;

		return { mCompiledShaders[vertexShaderPath + fragmentShaderPath], {} };
	}

}