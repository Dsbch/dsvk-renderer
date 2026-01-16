#pragma once

#include <pch.h>
#include "platform/renderer/vertex.h"
#include "base/cache/cache.h"

namespace engine
{
	class texture;
	class shader;

	struct atlasEntry
	{
		int x, y; // Offsets for X, Y.
		int size;
		float downSampleScale;
	};

	class aManager
	{
	public:
		aManager();

		void setMakeShaderFunc(std::function<withError<std::shared_ptr<shader>>(const std::vector<uint32_t>& src)>&& func);
		void setMakeTextureFunc(std::function<withError<std::shared_ptr<texture>>(uint8_t* data, int width, int heigth, imageChannel channel)>&& func);
		
		withError<std::shared_ptr<shader>> getDefaultTaskShader();
		withError<std::shared_ptr<shader>> getDefaultMeshShader();
		withError<std::shared_ptr<shader>> getDefaultPixelShader();
		withError<std::shared_ptr<shader>> getDefaultComputeShader();
		withError<std::shared_ptr<shader>> loadShader(const std::string& shaderPath);

		void clearCache();

		withError<model> loadModelGLTF(
			const std::string& path,
			size_t maxVert = 32,
			size_t maxTriangles = 32,
			float coneWieght = 0.0f
		);
	private:
		struct image
		{
			uint8_t* data;
			int w, h;
			int padding;
			int channels;
		};

		withError<std::pair<std::shared_ptr<texture>, std::map<uint32_t, atlasEntry>>> makeTextureAtlas(std::vector<image> images);
		
		std::function<withError<std::shared_ptr<shader>>(const std::vector<uint32_t>& src)> makeShader;
		std::function<withError<std::shared_ptr<texture>>(uint8_t* data, int width, int heigth, imageChannel channel)> makeTexture;
		
		std::mutex mModelMu;
		std::mutex mShaderMu;
		std::mutex mTexturesMu;
		lruCache<uint32_t, model> mLoadedModels;
		lruCache<uint32_t, std::shared_ptr<shader>> mLoadedShaders;
		lruCache<uint32_t, std::pair<std::shared_ptr<texture>, std::map<uint32_t, atlasEntry>>> mLoadedTextureAtlases;
	};
}