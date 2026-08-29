#pragma once

#include <pch.h>
#include "pipeline.h"
#include "descriptorSet.h"
#include "buffer.h"
#include "platform/renderer/vertex.h"
#include "platform/renderer/renderer.h"
#include "deletionQueue.h"
#include "resourceManager.h"

namespace engine
{
	struct lineRenderer
	{
	public:
		error init(
			std::shared_ptr<context> ctx, 
			VkDevice device, 
			VkPhysicalDevice physicalDevice, 
			VmaAllocator allocator, 
			submit& is, 
			graphicsPreset preset,
			deviceLimits limits,
			std::shared_ptr<resourceManager> resourceManager
		);
		error destroy();
		error addLine(VkDevice device, VmaAllocator allocator, submit& is, line l);
		error drawLines(VkCommandBuffer cmd, uint32_t frameIndex);
	private:
		std::shared_ptr<context> mCtx;
		graphicsPreset mPreset;

		deletionQueue mDeletionQueue;

		graphicsPipeline mPipeline;
		std::shared_ptr<resourceManager> mResourceManager;

		error initPipeline(VkDevice device, VkFormat depthFormat, VkFormat drawFormat);
	};
}