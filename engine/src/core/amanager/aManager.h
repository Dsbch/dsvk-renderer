#pragma once

#include <pch.h>
#include "base/cache/cache.h"
#include "platform/renderer/vertex.h"

namespace engine
{
	class texture;
	class shader;

	class aManager
	{
	public:
		aManager();

		void setMakeShaderFunc(std::function<withError<std::shared_ptr<shader>>(const std::vector<uint32_t>& src)>&& func);
		void setMakeTextureFunc(std::function<withError<std::shared_ptr<texture>>(const image& img)>&& func);
		void setMakeTextureWithMipsFunc(std::function<withError<std::shared_ptr<texture>>(const imageWithMipLevels& img)>&& func);

		withError<std::shared_ptr<shader>> getDefaultTaskShader();
		withError<std::shared_ptr<shader>> getDefaultMeshShader();
		withError<std::shared_ptr<shader>> getDefaultPixelShader();
		withError<std::shared_ptr<shader>> getDefaultComputeShader();
		withError<std::shared_ptr<shader>> getDefaultLineVertexShader();
		withError<std::shared_ptr<shader>> getDefaultLinePixelShader();
		withError<std::shared_ptr<shader>> loadShader(const std::string& path);
		withError<std::shared_ptr<texture>> loadTexture(const image& img);
		withError<std::shared_ptr<texture>> loadTexture(const imageWithMipLevels& img);

		void clearCache();

		withError<model> loadModelGLTF(
			const std::string& path,
			size_t maxVert = 32,
			size_t maxTriangles = 32,
			float coneWieght = 0.0f,
			float errorLevel = 0.01f
		);
	private:
		std::function<withError<std::shared_ptr<shader>>(const std::vector<uint32_t>& src)> makeShader;
		std::function<withError<std::shared_ptr<texture>>(const image& img)> makeTexture;
		std::function<withError<std::shared_ptr<texture>>(const imageWithMipLevels& img)> makeTextureWithMips;

		std::mutex mModelMu;
		std::mutex mShaderMu;
		std::mutex mTexturesMu;
		lruCache<uint32_t, model> mLoadedModels;
		lruCache<uint32_t, std::shared_ptr<shader>> mLoadedShaders;
		lruCache<uint32_t, std::shared_ptr<texture>> mLoadedTextures;
	};
}