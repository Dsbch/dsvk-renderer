#pragma once

#include <pch.h>

#include "platform/renderer/vulkan/base/include.h"

namespace engine
{
	struct textureStore
	{
	public:
		void init(std::shared_ptr<vulkanContext> mVulkanCtx);
		error build();
		void destroy();

		void transitionColorAttachmentImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionDepthImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionAccumImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionRevealImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionHzbChainImages(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
	private:
		materialRegistry mMaterialRegistry;
		vulkanImage mClipMap;
		vulkanImage mAccumImage;
		vulkanImage mRevealImage;
		vulkanImage mAccumResolveImage;
		vulkanImage mRevealResolveImage;
		vulkanImage mColorAttachmentImage;
		vulkanImage mColorAttachmentResolveImage;
		vulkanImage mDepthImage;
		vulkanImage mDepthResolveImage;
		std::vector<vulkanImage> mHZBImages;

		std::shared_ptr<vulkanContext> mVulkanCtx;
	};
}