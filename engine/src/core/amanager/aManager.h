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

		void setMakeShaderFunc(std::function<withError<std::shared_ptr<const shader>>(const std::vector<uint32_t>& src)>&& func);
		void setMakeTextureFunc(std::function<withError<std::shared_ptr<const texture>>(const image& img)>&& func);
		void setMakeTextureWithMipsFunc(std::function<withError<std::shared_ptr<const texture>>(const imageWithMipLevels& img)>&& func);

		withError<std::shared_ptr<const shader>> getDefaultCompositeTaskShader();
		withError<std::shared_ptr<const shader>> getDefaultCompositeMeshShader();
		withError<std::shared_ptr<const shader>> getDefaultCompositePixelShader();
		withError<std::shared_ptr<const shader>> getDefaultAccumilateTaskShader();
		withError<std::shared_ptr<const shader>> getDefaultAccumilateMeshShader();
		withError<std::shared_ptr<const shader>> getDefaultAccumilatePixelShader();
		withError<std::shared_ptr<const shader>> getDefaultComputeCompactShader();
		withError<std::shared_ptr<const shader>> getDefaultTaskShader();
		withError<std::shared_ptr<const shader>> getDefaultMeshShader();
		withError<std::shared_ptr<const shader>> getDefaultPixelShader();
		withError<std::shared_ptr<const shader>> getDefaultLineVertexShader();
		withError<std::shared_ptr<const shader>> getHzbGenShader();
		withError<std::shared_ptr<const shader>> getCullingShader();
		withError<std::shared_ptr<const shader>> getDefaultLinePixelShader();
		withError<std::shared_ptr<const shader>> loadShader(const std::string& path);
		withError<materialTextures> loadDetaultMaterial();
		withError<std::shared_ptr<const texture>> loadTexture(const image& img);
		withError<std::shared_ptr<const texture>> loadTexture(const imageWithMipLevels& img);

		void clearCache();

		withError<std::shared_ptr<const model>> loadModelGLTF(
			const std::string& path,
			size_t maxVert = 32,
			size_t maxTriangles = 32,
			float coneWieght = 0.0f,
			float errorLevel = 0.01f
		);
	private:
		std::function<withError<std::shared_ptr<const shader>>(const std::vector<uint32_t>& src)> makeShader;
		std::function<withError<std::shared_ptr<const texture>>(const image& img)> makeTexture;
		std::function<withError<std::shared_ptr<const texture>>(const imageWithMipLevels& img)> makeTextureWithMips;

		co::mutex mModelMu;
		co::mutex mShaderMu;
		co::mutex mTexturesMu;
		lruCache<uint32_t, std::shared_ptr<const model>> mLoadedModels;
		lruCache<uint32_t, std::shared_ptr<const shader>> mLoadedShaders;
		lruCache<uint32_t, std::shared_ptr<const texture>> mLoadedTextures;
	};
}