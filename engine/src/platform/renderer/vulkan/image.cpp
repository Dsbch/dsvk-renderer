#include <pch.h>
#include "image.h"

namespace engine
{
	static inline uint32_t bytesPerTexel(VkFormat f)
	{
		switch (f)
		{
		case VK_FORMAT_R8_UNORM:                  return 1;
		case VK_FORMAT_R8G8_UNORM:                return 2;
		case VK_FORMAT_R8G8B8_UNORM:			  return 3;
		case VK_FORMAT_R8G8B8A8_UNORM:			  return 4;
		case VK_FORMAT_B8G8R8A8_UNORM:            return 4;
		case VK_FORMAT_R16_SFLOAT:                return 2;
		case VK_FORMAT_R16G16B16A16_SFLOAT:       return 8;   // 4 * 16-bit
		case VK_FORMAT_R32_SFLOAT:                return 4;
		case VK_FORMAT_R32G32_SFLOAT:             return 8;
		case VK_FORMAT_R32G32B32_SFLOAT:          return 12;
		case VK_FORMAT_R32G32B32A32_SFLOAT:       return 16;
		case VK_FORMAT_BC3_UNORM_BLOCK:			  return 1;
		case VK_FORMAT_BC7_UNORM_BLOCK:			  return 1;
		default:                                  return 0;
		}
	}

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
	)
	{
		VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		imageBarrier.pNext = nullptr;

		imageBarrier.srcStageMask = srcStageMask;
		imageBarrier.srcAccessMask = srcAccessMask;
		imageBarrier.dstStageMask = dstStageMask;
		imageBarrier.dstAccessMask = dstAccessMask;

		imageBarrier.oldLayout = currentLayout;
		imageBarrier.newLayout = newLayout;

		VkImageSubresourceRange subImage{};
		subImage.aspectMask = (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D16_UNORM) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;;
		subImage.baseMipLevel = 0;
		subImage.levelCount = VK_REMAINING_MIP_LEVELS;
		subImage.baseArrayLayer = 0;
		subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

		imageBarrier.subresourceRange = subImage;
		imageBarrier.image = image;

		VkDependencyInfo depInfo{};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.pNext = nullptr;

		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(cmd, &depInfo);
	}

	VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, uint32_t mipLevels, VkSampleCountFlagBits samples)
	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext = nullptr;

		info.imageType = VK_IMAGE_TYPE_2D;

		info.format = format;
		info.extent = extent;

		info.mipLevels = mipLevels;
		info.arrayLayers = 1;

		// msaa.
		info.samples = samples;

		//optimal tiling, which means the image is stored on the best gpu format
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = usageFlags;

		return info;
	}

	VkImageViewCreateInfo imageviewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.pNext = nullptr;

		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.image = image;
		info.format = format;
		info.subresourceRange.levelCount = mipLevels;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;
		info.subresourceRange.aspectMask = aspectFlags;

		return info;
	}

	void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize)
	{
		VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

		blitRegion.srcOffsets[1].x = srcSize.width;
		blitRegion.srcOffsets[1].y = srcSize.height;
		blitRegion.srcOffsets[1].z = 1;

		blitRegion.dstOffsets[1].x = dstSize.width;
		blitRegion.dstOffsets[1].y = dstSize.height;
		blitRegion.dstOffsets[1].z = 1;

		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcSubresource.mipLevel = 0;

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstSubresource.mipLevel = 0;

		VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
		blitInfo.dstImage = destination;
		blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		blitInfo.srcImage = source;
		blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		blitInfo.filter = VK_FILTER_LINEAR;
		blitInfo.regionCount = 1;
		blitInfo.pRegions = &blitRegion;

		vkCmdBlitImage2(cmd, &blitInfo);
	}

	void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent3D srcSize, VkExtent2D dstSize)
	{
		VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

		blitRegion.srcOffsets[1].x = srcSize.width;
		blitRegion.srcOffsets[1].y = srcSize.height;
		blitRegion.srcOffsets[1].z = 1;

		blitRegion.dstOffsets[1].x = dstSize.width;
		blitRegion.dstOffsets[1].y = dstSize.height;
		blitRegion.dstOffsets[1].z = 1;

		blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcSubresource.mipLevel = 0;

		blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.baseArrayLayer = 0;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstSubresource.mipLevel = 0;

		VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
		blitInfo.dstImage = destination;
		blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		blitInfo.srcImage = source;
		blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		blitInfo.filter = VK_FILTER_LINEAR;
		blitInfo.regionCount = 1;
		blitInfo.pRegions = &blitRegion;

		vkCmdBlitImage2(cmd, &blitInfo);
	}

	void vulkanImage::init(VkDevice device, VmaAllocator allocator)
	{
		mDevice = device;
		mAllocator = allocator;
	}

	engine::error vulkanImage::build(submit& is, const image& rawImage, VkImageLayout neededLayout)
	{
		size_t dataSize = rawImage.getSize();

		// Main buffer.
		auto uploadbuffer = vulkanBuffer::createBuffer(mAllocator, mDevice, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_ONLY, true);
		if (!uploadbuffer)
			return uploadbuffer.err();

		std::memcpy(uploadbuffer.value().info.pMappedData, rawImage.data.data(), dataSize);

		VkImageUsageFlags usage = 0;
		usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;       // Needed to copy/upload from a staging buffer
		usage |= VK_IMAGE_USAGE_SAMPLED_BIT;            // Needed to read in a shader
		usage |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;	// GPU only memmory.
		usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;       // To generate mipmaps

		VkExtent3D size = VkExtent3D{ .width = uint32_t(rawImage.w), .height = uint32_t(rawImage.h), .depth = 1 };

		VkFormat format = VK_FORMAT_R8G8B8_UNORM;

		if (intToChannel(rawImage.channels) == rgba)
			format = VK_FORMAT_R8G8B8A8_UNORM;

		if (intToChannel(rawImage.channels) == grayscale)
			format = VK_FORMAT_R8_UNORM;

		if (rawImage.compressed)
			format = VK_FORMAT_BC7_UNORM_BLOCK;

		auto newImage = createImage(size, format, usage, false);
		if (!newImage)
			return newImage.err();

		error err = is.queue(
			[&](VkCommandBuffer cmd)
			{
				transitionImage(
					cmd,
					newImage.value().image,
					format,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_PIPELINE_STAGE_2_NONE,
					VK_ACCESS_2_NONE,
					VK_PIPELINE_STAGE_2_COPY_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT
				);

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

				transitionImage(
					cmd,
					newImage.value().image,
					format,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					neededLayout,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
					VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT
				);
			},
			[=]()
			{
				vulkanBuffer::destroyBuffer(mAllocator, uploadbuffer.value());
			}
		);
		if (err)
			return err;

		img = newImage.value();

		return {};
	}

	engine::error vulkanImage::build(submit& is, const imageWithMipLevels& rawImage, VkImageLayout neededLayout)
	{
		size_t dataSize = rawImage.main.getSize();

		// Main buffer.
		auto uploadbuffer = vulkanBuffer::createBuffer(mAllocator, mDevice, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_ONLY, true);
		if (!uploadbuffer)
			return uploadbuffer.err();

		std::memcpy(uploadbuffer.value().info.pMappedData, rawImage.main.data.data(), dataSize);

		VkImageUsageFlags usage = 0;
		usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;       // Needed to copy/upload from a staging buffer
		usage |= VK_IMAGE_USAGE_SAMPLED_BIT;            // Needed to read in a shader
		usage |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;	// GPU only memmory.
		usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;       // To generate mipmaps

		VkExtent3D size = VkExtent3D{ .width = uint32_t(rawImage.main.w), .height = uint32_t(rawImage.main.h), .depth = 1 };

		VkFormat format = VK_FORMAT_R8G8B8_UNORM;

		if (intToChannel(rawImage.main.channels) == rgba)
			format = VK_FORMAT_R8G8B8A8_UNORM;

		if (intToChannel(rawImage.main.channels) == grayscale)
			format = VK_FORMAT_R8_UNORM;

		if (rawImage.main.compressed)
			format = VK_FORMAT_BC7_UNORM_BLOCK;

		auto newImage = createImage(size, format, usage, true);
		if (!newImage)
			return newImage.err();

		size_t mipUploadBufSize = 0;
		for (auto& m : rawImage.mipLevels)
			mipUploadBufSize += m.getSize();

		withError<allocatedBuffer> mipUploadbuffer{ allocatedBuffer{} };

		if (mipUploadBufSize != 0)
		{
			mipUploadbuffer = vulkanBuffer::createBuffer(mAllocator, mDevice, mipUploadBufSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_ONLY, true);
			if (!mipUploadbuffer)
				return mipUploadbuffer.err();
		}

		size_t offset = 0;
		std::vector<VkBufferImageCopy> mipsCopyRegions = {};
		for (int i = 0; i < rawImage.mipLevels.size(); i++)
		{
			VkBufferImageCopy copyRegion{};
			copyRegion.bufferOffset = offset;
			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageSubresource.mipLevel = i + 1;
			copyRegion.imageExtent = VkExtent3D{ .width = uint32_t(rawImage.mipLevels[i].w), .height = uint32_t(rawImage.mipLevels[i].h), .depth = 1, };

			std::memcpy(
				reinterpret_cast<uint8_t*>(mipUploadbuffer.value().info.pMappedData) + offset,
				rawImage.mipLevels[i].data.data(),
				rawImage.mipLevels[i].getSize()
			);

			mipsCopyRegions.push_back(copyRegion);

			offset += rawImage.mipLevels[i].getSize();
		}

		error err = is.queue(
			[=](VkCommandBuffer cmd)
			{
				transitionImage(
					cmd,
					newImage.value().image,
					format,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_PIPELINE_STAGE_2_NONE,
					VK_ACCESS_2_NONE,
					VK_PIPELINE_STAGE_2_COPY_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT
				);

				VkBufferImageCopy copyRegion{};
				copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.layerCount = 1;
				copyRegion.imageExtent = size;

				vkCmdCopyBufferToImage(cmd, uploadbuffer.value().buffer, newImage.value().image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

				// Upload mip levels to GPU.
				if (mipUploadBufSize != 0)
					vkCmdCopyBufferToImage(cmd, mipUploadbuffer.value().buffer, newImage.value().image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint32_t(mipsCopyRegions.size()), mipsCopyRegions.data());

				transitionImage(
					cmd,
					newImage.value().image,
					format,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					neededLayout,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT,
					VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
					VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT
				);
			},
			[=]()
			{
				vulkanBuffer::destroyBuffer(mAllocator, uploadbuffer.value());
				vulkanBuffer::destroyBuffer(mAllocator, mipUploadbuffer.value());
			}
		);
		if (err)
			return err;

		img = newImage.value();

		return {};
	}

	engine::error vulkanImage::build(submit& is, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped, VkSampleCountFlagBits samples, VkImageLayout neededLayout, bool queue)
	{
		auto newImage = createImage(size, format, usage, mipmapped, samples);
		if (!newImage)
			return newImage.err();

		img = newImage.value();

		if (queue)
		{
			error err = is.queue(
				[=](VkCommandBuffer cmd)
				{
					transitionImage(
						cmd,
						newImage.value().image,
						format,
						VK_IMAGE_LAYOUT_UNDEFINED,
						neededLayout,
						VK_PIPELINE_STAGE_2_NONE,
						VK_ACCESS_2_NONE,
						VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
						VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT
					);
				}
			);
			if (err)
				return err;
		}
		else
		{
			error err = is.immediate(
				[=](VkCommandBuffer cmd)
				{
					transitionImage(
						cmd,
						newImage.value().image,
						format,
						VK_IMAGE_LAYOUT_UNDEFINED,
						neededLayout,
						VK_PIPELINE_STAGE_2_NONE,
						VK_ACCESS_2_NONE,
						VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
						VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT
					);
				}
			);
			if (err)
				return err;
		}

		return {};
	}

	void vulkanImage::destroy()
	{
		vkDestroyImageView(mDevice, img.view, nullptr);
		vmaDestroyImage(mAllocator, img.image, img.allocation);

		img.view = VK_NULL_HANDLE;
		img.image = VK_NULL_HANDLE;
		img.allocation = VK_NULL_HANDLE;
	}

	engine::withError<allocatedImage> vulkanImage::createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped, VkSampleCountFlagBits samples)
	{
		allocatedImage newImage = {};

		newImage.format = format;
		newImage.extent = size;

		uint32_t mips = 1;
		if (mipmapped)
			mips = image::mipLevels(int(size.width), int(size.height));

		VkImageCreateInfo img_info = imageCreateInfo(format, usage, size, mips, samples);

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
		VkImageViewCreateInfo view_info = imageviewCreateInfo(format, newImage.image, aspectFlag, mips);

		vkRes = vkCreateImageView(mDevice, &view_info, nullptr, &newImage.view);
		if (vkRes != VK_SUCCESS)
			return { vkResultToStr(vkRes) };

		return newImage;
	}
}

