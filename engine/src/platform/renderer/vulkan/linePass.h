#pragma once

#include <pch.h>

#include "resourceManager.h"
#include "base/include.h"

namespace engine
{
	struct linePass
	{
	public:
		error init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanCtx, std::shared_ptr<resourceManager> resourceManager);
		error destroy();
		error drawLines(VkCommandBuffer cmd, uint32_t frameIndex);
	private:
		std::shared_ptr<context> mCtx;
		std::shared_ptr<vulkanContext> mVulkanCtx;
		std::shared_ptr<resourceManager> mResourceManager;

		graphicsPipeline mPipeline;

		error initPipeline();
	};
}