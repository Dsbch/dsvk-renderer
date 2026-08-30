#pragma once

#include <pch.h>

#include "computeRenderer.h"
#include "vulkanContext.h"
#include "resourceManager.h"
#include "base/include.h"

namespace engine
{
	struct meshletRenderer
	{
	public:
		meshletRenderer() = default;
		meshletRenderer(const meshletRenderer&) = delete;

		error init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanContext, std::shared_ptr<resourceManager> resourceManager);
		error destroy();

		error createPipeline(const model& m);
		
		error opaquePass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error accumilationPass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error compositePass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
	
		error voxilizeOpaqueGeometry(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
	private:
		error initBlendingPipelines();
		error initVoxelPipelines();

		std::shared_ptr<context> mCtx;
		std::shared_ptr<resourceManager> mResourceManager;
		std::shared_ptr<vulkanContext> mVulkanCtx;

		graphicsPipeline mVoxelizationPipeline;
		graphicsPipeline mCompositePipeline;
		graphicsPipeline mAccumilationPipeline;
		std::unordered_map<uint32_t, graphicsPipeline> mOpaquePipelines;

		// Compute renderer to make HZB and for culling.
		computeRenderer mComputeRenderer;
	};
}