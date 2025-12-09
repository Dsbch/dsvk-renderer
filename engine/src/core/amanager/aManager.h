#pragma once

#include <pch.h>

namespace engine
{
	class texture;
	class shader;
	class renderer;

	class aManager
	{
	public:
		withError<std::shared_ptr<shader>> getDefaultTaskShader(renderer* r);
		withError<std::shared_ptr<shader>> getDefaultMeshShader(renderer* r);
		withError<std::shared_ptr<shader>> getDefaultPixelShader(renderer* r);
		withError<std::shared_ptr<shader>> getDefaultComputeShader(renderer* r);
		withError<std::shared_ptr<shader>> loadShader(const std::string& shaderPath, renderer* r);
		withError<std::shared_ptr<texture>> loadTexture(const std::string& path, renderer* r);
	private:
		withError<std::shared_ptr<texture>> getTexture(const std::string& path);
		withError<std::shared_ptr<shader>> getShader(const std::string& shaderPath);
		
		std::map<uint32_t, std::shared_ptr<texture>> mLoadedTextures;
		std::map<uint32_t, std::shared_ptr<shader>> mLoadedShaders;
	};
}