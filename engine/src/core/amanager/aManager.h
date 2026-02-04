#pragma once

#include <pch.h>
#include "platform/renderer/vertex.h"
#include "base/cache/cache.h"
#include <platform/renderer/vertex.h>

struct cgltf_node;
struct cgltf_primitive;
struct cgltf_texture;
struct stbrp_rect;
struct cgltf_material;

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
		withError<std::shared_ptr<shader>> getDefaultLineVertexShader();
		withError<std::shared_ptr<shader>> getDefaultLinePixelShader();
		withError<std::shared_ptr<shader>> loadShader(const std::string& shaderPath);

		void clearCache();

		withError<model> loadModelGLTF(
			const std::string& path,
			size_t maxVert = 32,
			size_t maxTriangles = 32,
			float coneWieght = 0.0f
		);
	private:
		static void calculateTangents(
			std::vector<vertex>& v,
			const std::vector<uint32_t>& index
		);
		static std::vector<uint32_t> repackPrimitives(
			const std::vector<uint8_t>& primitives,
			std::vector<meshlet>& meshlets
		);
		static error remapMesh(
			const std::vector<vertex>& vertecies,
			const std::vector<uint32_t> indicies,
			std::vector<vertex>& vOut,
			std::vector<uint32_t>& iOut
		);
		static error generateMeshlets(
			const std::vector<vertex>& vertecies,
			const std::vector<uint32_t>& indicies,
			std::vector<meshlet>& mOut,
			std::vector<uint8_t>& pOut,
			std::vector<uint32_t>& iOut,
			size_t maxVert, size_t maxTriangles, float coneWieght,
			float errorLevel,
			size_t targetIndexCount
		);
		static glm::mat4 getNodeWorldTransform(const cgltf_node* node);
		
		struct primitive
		{
			std::vector<vertex> vertecies;
			std::vector<uint32_t> indicies;
		};
		static primitive processPrimitive(const cgltf_primitive& prim, const glm::mat4& transform, cgltf_material* materials);

		struct imageInfo
		{
			int w, h, channels;
		};
		static withError<imageInfo> getImageInfo(const cgltf_texture* texture, const std::filesystem::path& baseDir);

		struct image
		{
			std::vector<uint8_t> data;
			int w, h;
			int padding;
			int channels;
		};
		static withError<image> processTexture(const cgltf_texture* texture, const std::filesystem::path& baseDir);
		static error applyBaseFactor(image& img, float factor[4]);
		static void applyMetallicRoughnessFactor(image& img, float metallic, float roughness);
		
		withError<materials> processMaterials(const std::filesystem::path& baseDir, const cgltf_material* materialsPtr, int materialCount);

		static void writeImageToAtlas(std::vector<uint8_t>& atlas, int size, const stbrp_rect* r, image& img);
		withError<std::pair<std::shared_ptr<texture>, std::map<uint32_t, atlasEntry>>> makeTextureAtlas(const std::vector<image>& images);
		
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