#pragma once

#include <pch.h>

#include "descriptorSet.h"
#include "submit.h"
#include "deletionQueue.h"
#include "pipeline.h"
#include "helper.h"
#include "swapChain.h"
#include "registry.h"
#include "resourceManager.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	struct computeRenderer
	{
	public:
		error init(
			std::shared_ptr<context> ctx,
			VkDevice device,
			VkPhysicalDevice physicalDevice,
			VmaAllocator allocator,
			submit& is,
			deviceLimits limits,
			graphicsPreset preset,
			std::shared_ptr<resourceManager> resourceManager
		);
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
		deletionQueue mDeletionQueue;
		graphicsPreset mPreset;
		std::shared_ptr<resourceManager> mResourceManager;

		computePipeline mBuildHzbPipeline;
		computePipeline mCullingPipeline;
		computePipeline mCompactCommandsPipeline;

		error initComputePipeline(VkDevice device);
	};
}