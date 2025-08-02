#include <pch.h>

#include "swapChain.h"
#include "vkHelper.h"

namespace vktest
{
	frameData& swapChain::getCurrentFrameData()
	{
		return mFrames[mFrameNumber % FRAME_OVERLAP];
	}

	vulkanImage& swapChain::getDrawImage()
	{
		return mDrawImage;
	}

	void swapChain::inrement()
	{
		mFrameNumber++;
	}

	void swapChain::pickImageExtent()
	{
		// here we pick needed height and width of our image to draw.
		mDrawImage.imageExtent.height = std::min(mSwapchainExtent.height, mDrawImage.imageExtent.height);
		mDrawImage.imageExtent.width = std::min(mSwapchainExtent.width, mDrawImage.imageExtent.width);
	}

	VkSwapchainKHR& swapChain::getSwapChain()
	{
		return mSwapchain;
	}

	std::vector<VkImage> swapChain::getSwapChainImages()
	{
		return mSwapchainImages;
	}

	std::vector<VkImageView> swapChain::getSwapChainImageViews()
	{
		return mSwapchainImageViews;
	}

	VkFormat swapChain::getSwapChainImageFormat()
	{
		return mSwapchainImageFormat;
	}

	void swapChain::present(VkQueue graphicQueue, uint32_t swapChainImageIndex)
	{
		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo = vktest::present_info();

		presentInfo.pSwapchains = &mSwapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &getCurrentFrameData().renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapChainImageIndex;

		vkQueuePresentKHR(graphicQueue, &presentInfo);
	}

	engine::withError<uint32_t> swapChain::acquireImageIndex()
	{
		uint32_t result = 0;
		VkResult e = vkAcquireNextImageKHR(mDevice, mSwapchain, 1000000000, getCurrentFrameData().swapchainSemaphore, nullptr, &result);
		if (e == VK_ERROR_OUT_OF_DATE_KHR || e != VK_SUCCESS)
		{
			return { vkResultToStr(e) };
		}

		return result;
	}


	engine::error swapChain::waitOnCurrentFence()
	{
		auto result = vkWaitForFences(mDevice, 1, &getCurrentFrameData().renderFence, true, 1000000000);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		// reset fence to reuse.
		result = vkResetFences(mDevice, 1, &getCurrentFrameData().renderFence);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		return {};
	}

	engine::error swapChain::resetCommandBuffer()
	{
		auto result = vkResetCommandBuffer(getCurrentFrameData().commandBuffer, 0);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		return {};
	}

	VkExtent2D& swapChain::getSwapChainExtent()
	{
		return mSwapchainExtent;
	}

	engine::error swapChain::createSwapChain(uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ mChosenGPU, mDevice, mSurface };

		auto result = swapchainBuilder
			.set_desired_min_image_count(FRAME_OVERLAP)
			.set_desired_format(VkSurfaceFormatKHR{ .format = mSwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build();
		if (!result.has_value())
			return result.error().message();

		mSwapchainExtent = result.value().extent;
		mSwapchain = result.value().swapchain;
		mSwapchainImages = result.value().get_images().value();
		mSwapchainImageViews = result.value().get_image_views().value();

		//draw image size will match the window
		VkExtent3D drawImageExtent = {
			width,
			height,
			1
		};

		//hardcoding the draw format to 32 bit float
		mDrawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		mDrawImage.imageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo rimg_info = vkinit::image_create_info(mDrawImage.imageFormat, drawImageUsages, drawImageExtent);

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo = {};
		rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		auto createImageRes = vmaCreateImage(mAllocator, &rimg_info, &rimg_allocinfo, &mDrawImage.image, &mDrawImage.allocation, nullptr);
		if (createImageRes != VK_SUCCESS)
			return vkResultToStr(createImageRes);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(mDrawImage.imageFormat, mDrawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		auto createImageViewRes = vkCreateImageView(mDevice, &rview_info, nullptr, &mDrawImage.imageView);
		if (createImageViewRes != VK_SUCCESS)
			return vkResultToStr(createImageViewRes);

		return {};
	}

	VkFormat swapChain::getImageFormat()
	{
		return mDrawImage.imageFormat;
	}

	engine::error swapChain::init(uint32_t width, uint32_t height, uint32_t graphicsQueueFamily)
	{
		VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info(0);

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			auto result = vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mFrames[i].renderFence);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };

			result = vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mFrames[i].swapchainSemaphore);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };

			result = vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mFrames[i].renderSemaphore);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };

			result = vkCreateCommandPool(mDevice, &commandPoolInfo, nullptr, &mFrames[i].commandPool);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };

			VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(mFrames[i].commandPool, 1);

			result = vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &mFrames[i].commandBuffer);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };
		}

		return createSwapChain(width, height);
	}

	void swapChain::destroy()
	{
		vkDeviceWaitIdle(mDevice);

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			vkDestroyCommandPool(mDevice, mFrames[i].commandPool, nullptr);
			vkDestroyFence(mDevice, mFrames[i].renderFence, nullptr);
			vkDestroySemaphore(mDevice, mFrames[i].renderSemaphore, nullptr);
			vkDestroySemaphore(mDevice, mFrames[i].swapchainSemaphore, nullptr);
		}

		vkDestroyImageView(mDevice, mDrawImage.imageView, nullptr);
		vmaDestroyImage(mAllocator, mDrawImage.image, mDrawImage.allocation);

		vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

		for (int i = 0; i < mSwapchainImageViews.size(); i++)
			vkDestroyImageView(mDevice, mSwapchainImageViews[i], nullptr);
	}
}