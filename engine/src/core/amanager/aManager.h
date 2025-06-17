#pragma once

#include <pch.h>

namespace engine
{
	class texture;
	class shaderProgram;
	class cubeMap;

	class aManager
	{
	private:
		std::map<std::array<std::string, 6>, std::shared_ptr<cubeMap>> mLoadedCubeMaps;
		std::map<std::string, std::shared_ptr<texture>> mLoadedTextures;
		std::map<std::array<std::string, 2>, std::shared_ptr<shaderProgram>> mCompiledShaders;
		std::pair<const std::shared_ptr<texture>, error> getTexture(const std::string& path);
		std::pair<const std::shared_ptr<shaderProgram>, error> getShader(const std::string& vertexPath, const std::string& fragmentPath);
		std::pair<const std::shared_ptr<cubeMap>, error> getCubeMap(const std::array<std::string, 6> path);
	public:
		std::pair<const std::shared_ptr<shaderProgram>, error> loadShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
		std::pair<const std::shared_ptr<texture>, error> loadTexture(const std::string& path);
		std::pair<const std::shared_ptr<cubeMap>, error> loadCubeMap(const std::array<std::string, 6> path);
	};
}