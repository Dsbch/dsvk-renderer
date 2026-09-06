#pragma once

#include <pch.h>

#include "resourceManager.h"
#include "meshletPass.h"
#include "linePass.h"
#include "uiPass.h"
#include "voxelPass.h"
#include "base/include.h"

namespace engine
{
	class vulkanRenderer : public renderer
	{
	public:
		vulkanRenderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window);
		~vulkanRenderer();
		error checkError() const;
		std::string getVersion() const;
		std::string getGpuName() const;
		error changeViewPort(uint32_t width, uint32_t height);

		error render();

		withError<std::shared_ptr<const texture>> makeTexture(const image& img);
		withError<std::shared_ptr<const shader>> makeShader(const std::vector<uint32_t>& src);
		withError<std::shared_ptr<const texture>> makeTextureWithMips(const imageWithMipLevels& img);
	private:
		error drawAsRaster(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error drawAsVoxels(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);

		error initPasses(std::shared_ptr<window> window);

		error drawOpaque(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error drawTransperent(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error compositeOpaqueAndTransperent(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error drawUI(VkCommandBuffer cmd);

		void updateProfInfo(float deltaTime, uint32_t frameIndex);
		void registerSceneMetrics(const model& m, bool isDeleted = false);
		void visualizeNormals(const model& m);
		void visualizeAABB(const model& m);
		void visualizeAABB(const aabb& box);

		error handleEvents();
		error addToRender(const std::set<model>& addedEntities, uint32_t frameIndex);
		error updateInstance(const std::set<model>& updatedEntities, uint32_t frameIndex);
		error updateAnimations(const std::set<model>& animationUpdatedEntities, uint32_t frameIndex);
		void removeFromRender(const std::set<model>& deletedEntities, uint32_t frameIndex);

		aabb getSceneBoundingBox() const;
		voxelDrawParams getVoxelSceneParams() const;

		std::unordered_map<uint32_t, aabb> mSceneAABB;

		bool mWindowMinimized;
		profilingInfo mProfInfo;

		// Geometry pass.
		std::unique_ptr <meshletPass> mMeshletPass;
		// Line renderer.
		std::unique_ptr <linePass> mLinePass;
		// UI renderer.
		std::unique_ptr<uiPass> mUiPass;
		// Voxel pass.
		std::unique_ptr<voxelPass> mVoxelPass;

		std::shared_ptr<vulkanContext> mVulkanCtx;
		std::shared_ptr<resourceManager> mResourceManager;
	};
}
