#pragma once
#include <pch.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "platform/renderer/texture.h"
#include "submit.h"
#include "buffer.h"
#include "helper.h"

namespace engine
{
	void transitionImage(
		VkCommandBuffer cmd,
		VkImage image,
		VkFormat format,
		VkImageLayout currentLayout,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask
	);
	VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, uint32_t mipLevels = 1, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, VkImageType imageType = VK_IMAGE_TYPE_2D);
	VkImageViewCreateInfo imageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

	void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);

	void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent3D srcSize, VkExtent2D dstSize);

	struct allocatedImage
	{
		VkImage image;
		VkImageView view;
		VmaAllocation allocation;
		VkExtent3D extent;
		VkFormat format;
	};

	struct vulkanImage
	{
	public:
		allocatedImage img;

		void init(VkDevice device, VmaAllocator allocator);
		engine::error build(submit& is, const image& img, VkImageLayout neededLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		engine::error build(submit& is, const imageWithMipLevels& img, VkImageLayout neededLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		engine::error build(
			submit& is, 
			VkExtent3D size, 
			VkFormat format, 
			VkImageUsageFlags usage, 
			bool mipmapped, 
			VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, 
			VkImageLayout neededLayout = VK_IMAGE_LAYOUT_GENERAL, 
			bool queue = false,
			VkImageType imageType = VK_IMAGE_TYPE_2D	
		);
		void destroy();
	private:
		engine::withError<allocatedImage> createImage(
			VkExtent3D size, 
			VkFormat format, 
			VkImageUsageFlags usage, 
			bool mipmapped, 
			VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
			VkImageType imageType = VK_IMAGE_TYPE_2D
		);
		
		VmaAllocator mAllocator;

		VkDevice mDevice;
	};
}