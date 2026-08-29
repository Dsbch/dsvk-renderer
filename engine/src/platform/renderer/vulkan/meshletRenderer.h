#pragma once

#include <pch.h>
#include "descriptorSet.h"
#include "registry.h"
#include "submit.h"
#include "deletionQueue.h"
#include "helper.h"
#include "platform/renderer/renderer.h"
#include "computeRenderer.h"
#include "pipeline.h"
#include "commandBuffer.h"
#include "resourceManager.h"

namespace engine
{
	struct meshletRenderer
	{
	public:
		error init(
			std::shared_ptr<context> ctx,
			PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT,
			PFN_vkCmdDrawMeshTasksIndirectEXT vkCmdDrawMeshTasksIndirectEXT,
			VkDevice device,
			VkPhysicalDevice physicalDevice,
			VmaAllocator allocator,
			submit& is,
			deviceLimits limits,
			graphicsPreset preset,
			std::shared_ptr<resourceManager> resourceManager
		);
		error destroy();

		error addToRender(const model& m, VkDevice device, VmaAllocator allocator, submit& is, uint32_t frameIndex);
		error updateInstance(const model& m, VkDevice device, VmaAllocator allocator, submit& is, uint32_t frameIndex);
		error updateAnimations(const model& m, VkDevice device, VmaAllocator allocator, submit& is, uint32_t frameIndex);
		void removeFromRender(const model& m, VkDevice device, VmaAllocator allocator, submit& is, uint32_t frameIndex);
		
		error opaquePass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error accumilationPass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error compositePass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
	
		voxelDrawParams getVoxelSceneParams() const;
		error voxilizeOpaqueGeometry(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
	private:
		error initBlendingPipelines(VkDevice device, VmaAllocator allocator, submit& is);
		error initVoxelPipelines(VkDevice device, VmaAllocator allocator, submit& is);

		aabb getSceneBoundingBox() const;

		std::shared_ptr<context> mCtx;
		std::shared_ptr<resourceManager> mResourceManager;
		deletionQueue mDeletionQueue;
		graphicsPreset mPreset;

		PFN_vkCmdDrawMeshTasksEXT mVkCmdDrawMeshTasksEXT;
		PFN_vkCmdDrawMeshTasksIndirectEXT mVkCmdDrawMeshTasksIndirectEXT;

		graphicsPipeline mVoxelizationPipeline;
		std::unordered_map<uint32_t, aabb> mSceneAABB;
		
		graphicsPipeline mCompositePipeline;
		graphicsPipeline mAccumilationPipeline;
		std::unordered_map<uint32_t, graphicsPipeline> mOpaquePipelines;

		// Compute renderer to make HZB and for culling.
		computeRenderer mComputeRenderer;
	};
}