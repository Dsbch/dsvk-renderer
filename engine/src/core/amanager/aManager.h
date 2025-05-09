#pragma once

#include <pch.h>

namespace engine
{
	class texture;
	class shaderProgram;

	class aManager
	{
	private:
		std::map<std::string, std::shared_ptr<texture>> mLoadedTextures;
		std::map<std::string, std::shared_ptr<shaderProgram>> mCompiledShaders;
		std::pair<const std::shared_ptr<texture>, error> getTexture(const std::string& path);
		std::pair<const std::shared_ptr<shaderProgram>, error> getShader(const std::string& vertexPath, const std::string& fragmentPath);
	public:
		std::pair<const std::shared_ptr<shaderProgram>, error> loadShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
		std::pair<const std::shared_ptr<texture>, error> loadTexture(const std::string& path);
	};
}