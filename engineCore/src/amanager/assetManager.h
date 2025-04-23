#pragma once

#include <pch.h>

#include "renderer/opengl/texture.h"
#include "renderer/opengl/shader.h"
#include "context/context.h"

namespace engineCore
{
	class assetManager
	{
	private:
		context mCtx;

		std::map<std::string, std::shared_ptr<texture>> mLoadedTextures;
		std::map<std::string, std::shared_ptr<shaderProgram>> mCompiledShaders;
	public:
		assetManager(context ctx);

		std::pair<const std::shared_ptr<texture>, engineCore::error> getTexture(const std::string& id);
		std::pair<const std::shared_ptr<texture>, engineCore::error> loadTexture(const std::string& path);
		std::pair<const std::shared_ptr<shaderProgram>, engineCore::error> getCompiledShader(const std::string& id);
		std::pair<const std::shared_ptr<shaderProgram>, engineCore::error> loadAndCompileShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
	};

}