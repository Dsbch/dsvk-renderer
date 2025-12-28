#include <pch.h>
#include "vulkanTexture.h"

namespace engine
{
	vulkanTexture::vulkanTexture(VkDevice device, VmaAllocator allocator, submit is, uint8_t* data, int width, int heigth, imageChannel channel)
		:
		texture(data, width, heigth, channel)
	{
		mImage.init(device, allocator);

		VkImageUsageFlags usage = 0;
		usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;       // Needed to copy/upload from a staging buffer
		usage |= VK_IMAGE_USAGE_SAMPLED_BIT;            // Needed to read in a shader
		usage |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;	// GPU only memmory.
		usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;       // To generate mipmaps

		mErr = mImage.build(is, data, VkExtent3D{ .width = uint32_t(width), .height = uint32_t(heigth), .depth = 1 }, VK_FORMAT_R8G8B8A8_UNORM, usage, true);
		if (mErr)
			return;

		mHash = crc32(data, width*heigth);
	}

	vulkanTexture::~vulkanTexture()
	{
		mImage.destroy();
	}

	uint32_t vulkanTexture::hash() const
	{
		return mHash;
	}
}