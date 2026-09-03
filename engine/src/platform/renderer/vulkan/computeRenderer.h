#pragma once

#include <pch.h>

#include "resourceManager.h"
#include "base/include.h"

namespace engine
{
	struct computeRenderer
	{
	public:
		error init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanContext, std::shared_ptr<resourceManager> resourceManager);
		error destroy();

		error buildHZB(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);

		struct cullMeshletsParams
		{
			uint32_t cmdBufferCount;
			uint32_t cullStage;
			uint32_t opaqueCmdBufferIndex;
			uint32_t hzbLength;
		};

		error cullMeshlets(VkCommandBuffer cmd, renderer::renderParams in, cullMeshletsParams params, uint32_t frameIndex);

		struct compactCommandBufferParams
		{
			uint32_t cmdBufferCount;
			uint32_t opaqueCmdBufferIndex;
			uint32_t stage;
			uint32_t compactRule;
		};

		error compactCommandBuffer(VkCommandBuffer cmd, renderer::renderParams in, compactCommandBufferParams params, uint32_t frameIndex);
	private:
		std::shared_ptr<context> mCtx;
		std::shared_ptr<resourceManager> mResourceManager;
		std::shared_ptr<vulkanContext> mVulkanCtx;

		computePipeline mBuildHzbPipeline;
		computePipeline mCullingPipeline;
		computePipeline mCompactCommandsPipeline;

		error initComputePipeline();
	};
}