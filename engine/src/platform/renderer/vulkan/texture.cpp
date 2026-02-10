#include <pch.h>
#include "texture.h"

namespace engine
{
	vulkanTexture::vulkanTexture(VkDevice device, VmaAllocator allocator, submit& is, const image& img)
		:
		texture(img), mHash(0), mImage{}
	{
		mImage.init(device, allocator);

		mErr = mImage.build(is, img);
		if (mErr)
			return;

		mHash = img.hash();
	}

	vulkanTexture::vulkanTexture(VkDevice device, VmaAllocator allocator, submit& is, const imageWithMipLevels& img)
		:
		texture(img), mHash(0), mImage{}
	{
		mImage.init(device, allocator);

		mErr = mImage.build(is, img);
		if (mErr)
			return;

		mHash = img.main.hash();
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