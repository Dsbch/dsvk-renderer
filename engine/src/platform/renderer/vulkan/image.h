#pragma once
#include <pch.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "submit.h"
#include "buffer.h"
#include "helper.h"

namespace engine
{
	void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
	VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, uint32_t mipLevels = 1);
	VkImageViewCreateInfo imageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);

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
		allocatedImage image;

		void init(VkDevice device, VmaAllocator allocator);
		engine::error build(submit& is, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
		engine::error build(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
		void destroy();
	private:
		engine::withError<allocatedImage> createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
		uint32_t mipLevels(VkExtent3D size) const;

		VmaAllocator mAllocator;

		VkDevice mDevice;
	};
}