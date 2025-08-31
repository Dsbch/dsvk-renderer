#pragma once
#include <pch.h>

#include <VkBootstrap.h>
#include <vma/vk_mem_alloc.h>

#include "immediateSubmit.h"
#include "vkHelper.h"
#include "vkBuffer.h"

namespace vktest
{
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
		engine::error build(immediateSubmit is, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
		engine::error build(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
		void destroy();
	private:
		engine::withError<allocatedImage> createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped);
		uint32_t mipLevels(VkExtent3D size) const;

		VmaAllocator mAllocator;

		VkDevice mDevice;
	};
}