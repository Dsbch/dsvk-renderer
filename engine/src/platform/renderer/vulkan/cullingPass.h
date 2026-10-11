#pragma once

#include <pch.h>

#include "resourceManager.h"
#include "base/include.h"

namespace engine
{
	struct cullingPass
	{
	public:
		error init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanContext, std::shared_ptr<resourceManager> resourceManager);
		error destroy();

		error buildHZB(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);

		struct cullMeshletsParams
		{
			uint32_t cullStage;
			uint32_t opaqueCmdBufferIndex;
			uint32_t hzbLength;
			uint32_t meshCount;
			uint32_t compactCapacity;
			vulkanBuffer dispatchBuffer;
		};

		error cullMeshes(VkCommandBuffer cmd, renderer::renderParams in, cullMeshletsParams params, uint32_t frameIndex);
		error cullMeshlets(VkCommandBuffer cmd, renderer::renderParams in, cullMeshletsParams params, uint32_t frameIndex);
	private:
		std::shared_ptr<context> mCtx;
		std::shared_ptr<resourceManager> mResourceManager;
		std::shared_ptr<vulkanContext> mVulkanCtx;

		computePipeline mBuildHzbPipeline;
		computePipeline mCullingMeshPipeline;
		computePipeline mCullingMeshletPipeline;

		error initComputePipeline();
	};
}