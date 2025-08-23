#include <pch.h>
#include "vulkanImage.h"

namespace vktest
{
	static inline uint32_t bytesPerTexel(VkFormat f)
	{
		switch (f)
		{
		case VK_FORMAT_R8_UNORM:                  return 1;
		case VK_FORMAT_R8G8_UNORM:                return 2;
		case VK_FORMAT_R8G8B8A8_UNORM:			  return 4;
		case VK_FORMAT_B8G8R8A8_UNORM:            return 4;
		case VK_FORMAT_R16_SFLOAT:                return 2;
		case VK_FORMAT_R16G16B16A16_SFLOAT:       return 8;   // 4 * 16-bit
		case VK_FORMAT_R32_SFLOAT:                return 4;
		case VK_FORMAT_R32G32_SFLOAT:             return 8;
		case VK_FORMAT_R32G32B32_SFLOAT:          return 12;
		case VK_FORMAT_R32G32B32A32_SFLOAT:       return 16;
		default:                                  return 0;
		}
	}

	void vulkanImage::init(VkDevice device, VmaAllocator allocator)
	{
		mDevice = device;
		mAllocator = allocator;
	}

	engine::error vulkanImage::build(immediateSubmit is, void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
	{
		size_t dataSize = size.depth * size.width * size.height * bytesPerTexel(format);
		auto uploadbuffer = vulkanBuffer::createBuffer(mAllocator, mDevice, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_ONLY, true);
		if (!uploadbuffer)
			return uploadbuffer.err();

		std::memcpy(uploadbuffer.value().info.pMappedData, data, dataSize);

		auto newImage = createImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);
		if (!newImage)
			return newImage.err();

		auto err = is.submit([&](VkCommandBuffer cmd)
			{
				vkinit::transition_image(cmd, newImage.value().image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

				VkBufferImageCopy copyRegion = {};
				copyRegion.bufferOffset = 0;
				copyRegion.bufferRowLength = 0;
				copyRegion.bufferImageHeight = 0;

				copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.mipLevel = 0;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount = 1;
				copyRegion.imageExtent = size;

				vkCmdCopyBufferToImage(cmd, uploadbuffer.value().buffer, newImage.value().image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

				//TODO: add mip-map generating
				// see: https://vulkan-tutorial.com/Generating_Mipmaps

				vkinit::transition_image(cmd, newImage.value().image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		);
		if (err)
			return err;

		vulkanBuffer::destroyBuffer(mAllocator, uploadbuffer.value());

		image = newImage.value();

		return { };
	}

	engine::error vulkanImage::build(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
	{
		auto newImage = createImage(size, format, usage, mipmapped);
		if (!newImage)
			return newImage.err();

		image = newImage.value();

		return {};
	}

	void vulkanImage::destroy()
	{
		vkDestroyImageView(mDevice, image.view, nullptr);
		vmaDestroyImage(mAllocator, image.image, image.allocation);
	}

	engine::withError<allocatedImage> vulkanImage::createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
	{
		allocatedImage newImage = {};

		newImage.format = format;
		newImage.extent = size;

		VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);

		if (mipmapped)
		{
			img_info.mipLevels = mipLevels(size);
		}

		// always allocate images on dedicated GPU memory.
		VmaAllocationCreateInfo allocinfo = {};
		allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// allocate and create the image
		auto vkRes = vmaCreateImage(mAllocator, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr);
		if (vkRes != VK_SUCCESS)
			return { vkResultToStr(vkRes) };

		// if the format is a depth format, we will need to have it use the correct aspect flag.
		VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		if (format == VK_FORMAT_D32_SFLOAT)
		{
			aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		// build a image-view for the image
		VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, newImage.image, aspectFlag);
		view_info.subresourceRange.levelCount = img_info.mipLevels;

		vkRes = vkCreateImageView(mDevice, &view_info, nullptr, &newImage.view);
		if (vkRes != VK_SUCCESS)
			return { vkResultToStr(vkRes) };

		return newImage;
	}

	uint32_t vulkanImage::mipLevels(VkExtent3D size) const
	{
		return static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}
}

