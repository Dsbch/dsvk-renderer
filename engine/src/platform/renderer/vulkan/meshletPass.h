#pragma once

#include <pch.h>

#include "cullingPass.h"
#include "resourceManager.h"
#include "base/include.h"

namespace engine
{
	struct meshletPass
	{
	public:
		meshletPass() = default;
		meshletPass(const meshletPass&) = delete;

		error init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanContext, std::shared_ptr<resourceManager> resourceManager);
		error destroy();

		error createPipeline(const model& m);
		
		error opaquePass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error accumilationPass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error compositePass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
	private:
		error initBlendingPipelines();

		std::shared_ptr<context> mCtx;
		std::shared_ptr<resourceManager> mResourceManager;
		std::shared_ptr<vulkanContext> mVulkanCtx;

		graphicsPipeline mCompositePipeline;
		graphicsPipeline mAccumilationPipeline;
		std::unordered_map<uint32_t, graphicsPipeline> mOpaquePipelines;

		cullingPass mCullingPass;
	};
}