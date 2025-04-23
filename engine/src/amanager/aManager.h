#pragma once

#include <pch.h>
#include "renderer/opengl/texture.h"
#include "renderer/opengl/shader.h"

namespace engine
{
	class aManager
	{
	private:
		std::map<std::string, std::shared_ptr<texture>> mLoadedTextures;
		std::map<std::string, std::shared_ptr<shaderProgram>> mCompiledShaders;
	public:
		std::pair<const std::shared_ptr<texture>, engine::error> getTexture(const std::string& id);
		std::pair<const std::shared_ptr<texture>, engine::error> loadTexture(const std::string& path);
		std::pair<const std::shared_ptr<shaderProgram>, engine::error> getCompiledShader(const std::string& id);
		std::pair<const std::shared_ptr<shaderProgram>, engine::error> loadAndCompileShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
	};
}