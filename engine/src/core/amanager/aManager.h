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
		withError<std::shared_ptr<texture>> getTexture(const std::string& path);
		withError<std::shared_ptr<shaderProgram>> getShader(const std::string& vertexPath, const std::string& fragmentPath);
		withError<std::shared_ptr<cubeMap>> getCubeMap(const std::array<std::string, 6> path);
	public:
		withError<std::shared_ptr<shaderProgram>> loadShader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
		withError<std::shared_ptr<texture>> loadTexture(const std::string& path);
		withError<std::shared_ptr<cubeMap>> loadCubeMap(const std::array<std::string, 6> path);
	};
}