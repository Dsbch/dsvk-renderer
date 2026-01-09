#pragma once

#include <pch.h>
#include "platform/renderer/vertex.h"

namespace engine
{
	class texture;
	class shader;

	struct atlasMapping
	{
		int index; // Index to a input array.
		int x, y; // Offsets for X, Y.
	};

	class aManager
	{
	public:
		void setMakeShaderFunc(std::function<withError<std::shared_ptr<shader>>(const std::vector<uint32_t>& src)>&& func);
		void setMakeTextureFunc(std::function<withError<std::shared_ptr<texture>>(uint8_t* data, int width, int heigth, imageChannel channel)>&& func);
		
		withError<std::shared_ptr<shader>> getDefaultTaskShader();
		withError<std::shared_ptr<shader>> getDefaultMeshShader();
		withError<std::shared_ptr<shader>> getDefaultPixelShader();
		withError<std::shared_ptr<shader>> getDefaultComputeShader();
		withError<std::shared_ptr<shader>> loadShader(const std::string& shaderPath);
		withError<std::shared_ptr<texture>> loadTexture(const std::string& path);

		withError<model> loadModelGLTF(
			const std::string& path,
			size_t maxVert = 64,
			size_t maxTriangles = 64,
			float coneWieght = 0.0f
		);

		void testTextureAtlassing();
	private:
		struct image
		{
			uint8_t* data;
			int w, h;
			int padding;
			int channels;
		};

		withError<std::pair<std::shared_ptr<texture>, std::vector<atlasMapping>>> makeTextureAtlas(const std::vector<image>& images);

		withError<std::shared_ptr<texture>> loadRawTexture(const uint8_t* data, size_t size);

		std::function<withError<std::shared_ptr<shader>>(const std::vector<uint32_t>& src)> makeShader;
		std::function<withError<std::shared_ptr<texture>>(uint8_t* data, int width, int heigth, imageChannel channel)> makeTexture;

		withError<std::shared_ptr<texture>> getTexture(const std::string& path);
		withError<std::shared_ptr<shader>> getShader(const std::string& shaderPath);
		
		std::map<uint32_t, std::shared_ptr<texture>> mLoadedTextures;
		std::map<uint32_t, std::pair<std::shared_ptr<texture>, std::vector<atlasMapping>>> mLoadedTextureAtlases;
		std::map<uint32_t, std::shared_ptr<shader>> mLoadedShaders;
	};
}