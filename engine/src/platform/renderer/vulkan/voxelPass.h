#pragma once

#include <pch.h>

#include "resourceManager.h"
#include "base/include.h"

namespace engine
{
	struct voxelPass
	{
	public:
		voxelPass() = default;
		voxelPass(const voxelPass&) = delete;

		error init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanContext, std::shared_ptr<resourceManager> resourceManager);
		error destroy();

		error voxelizeScene(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
		error visualizeVoxelScene(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex);
	private:
		std::shared_ptr<context> mCtx;
		std::shared_ptr<resourceManager> mResourceManager;
		std::shared_ptr<vulkanContext> mVulkanCtx;

		graphicsPipeline mVoxelizationPipeline;
		graphicsPipeline mVoxelRayMarchingPipeline;
	};
}