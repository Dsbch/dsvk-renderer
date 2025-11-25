#include <pch.h>

#include "vulkanSwapChain.h"

namespace engine
{
	VkPresentInfoKHR presentInfo()
	{
		VkPresentInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.pNext = 0;

		info.swapchainCount = 0;
		info.pSwapchains = nullptr;
		info.pWaitSemaphores = nullptr;
		info.waitSemaphoreCount = 0;
		info.pImageIndices = nullptr;

		return info;
	}

	frameData& swapChain::getCurrentFrameData()
	{
		return mFrames[mFrameNumber % FRAME_OVERLAP];
	}

	VkFormat swapChain::getDrawImageFormat()
	{
		return mDrawImage.image.format;
	}

	VkFormat swapChain::getDepthImageFormat()
	{
		return mDepthImage.image.format;
	}

	VkExtent3D swapChain::getDrawImageExtent()
	{
		return mDrawImage.image.extent;
	}

	VkExtent3D swapChain::getDepthImageExtent()
	{
		return mDepthImage.image.extent;
	}

	VkImage swapChain::getDrawImage()
	{
		return mDrawImage.image.image;
	}

	VkImage swapChain::getDepthImage()
	{
		return mDepthImage.image.image;
	}

	VkImageView swapChain::getDrawImageView()
	{
		return mDrawImage.image.view;
	}

	VkImageView swapChain::getDepthImageView()
	{
		return mDepthImage.image.view;
	}

	void swapChain::increment()
	{
		mFrameNumber++;
	}

	void swapChain::pickImageExtent()
	{
		// here we pick needed height and width of our images.
		mDrawImage.image.extent.height = std::min(mSwapchainExtent.height, mDrawImage.image.extent.height);
		mDrawImage.image.extent.width = std::min(mSwapchainExtent.width, mDrawImage.image.extent.width);
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

	engine::error swapChain::present(VkQueue graphicQueue, uint32_t swapChainImageIndex)
	{
		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presInfo = presentInfo();

		presInfo.pSwapchains = &mSwapchain;
		presInfo.swapchainCount = 1;

		presInfo.pWaitSemaphores = &getCurrentFrameData().renderSemaphore;
		presInfo.waitSemaphoreCount = 1;

		presInfo.pImageIndices = &swapChainImageIndex;

		auto result = vkQueuePresentKHR(graphicQueue, &presInfo);
		if (result != VK_SUCCESS)
			return vkResultToStr(result);

		return {};
	}

	engine::withError<uint32_t> swapChain::acquireImageIndex()
	{
		uint32_t result = 0;
		VkResult e = vkAcquireNextImageKHR(mDevice, mSwapchain, 1000000000, getCurrentFrameData().swapchainSemaphore, nullptr, &result);
		if (e != VK_SUCCESS)
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

		return {};
	}

	error swapChain::resetCurrentFence()
	{
		auto result = vkResetFences(mDevice, 1, &getCurrentFrameData().renderFence);
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

		// build drawImage.
		VkExtent3D drawImageExtent = {
			width,
			height,
			1
		};

		mDrawImage.image.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		mDrawImage.image.extent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		drawImageUsages |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		
		auto err = mDrawImage.build(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, false);
		if (err)
			return err;

		// build depth image.
		VkImageUsageFlags depthImageUsages{};
		depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthImageUsages |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		err = mDepthImage.build(drawImageExtent, VK_FORMAT_D32_SFLOAT, depthImageUsages, false);
		if (err)
			return err;

		return {};
	}

	engine::error swapChain::build(uint32_t width, uint32_t height, uint32_t graphicsQueueFamily)
	{
		VkCommandPoolCreateInfo commandPoolInfo = commandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		VkFenceCreateInfo fenceInfo = fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreInfo = semaphoreCreateInfo(0);

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			auto result = vkCreateFence(mDevice, &fenceInfo, nullptr, &mFrames[i].renderFence);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };

			result = vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mFrames[i].swapchainSemaphore);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };

			result = vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mFrames[i].renderSemaphore);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };

			result = vkCreateCommandPool(mDevice, &commandPoolInfo, nullptr, &mFrames[i].commandPool);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };

			VkCommandBufferAllocateInfo cmdAllocInfo = commandBufferAllocateInfo(mFrames[i].commandPool, 1);

			result = vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &mFrames[i].commandBuffer);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };
		}

		return createSwapChain(width, height);
	}

	void swapChain::init(VmaAllocator vma, VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice chosenGPU)
	{
		mAllocator = vma;
		mDevice = device;
		mSurface = surface;
		mChosenGPU = chosenGPU;

		mDrawImage.init(mDevice, mAllocator);
		mDepthImage.init(mDevice, mAllocator);
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

		mDepthImage.destroy();
		mDrawImage.destroy();

		vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

		mSwapchain = VK_NULL_HANDLE;

		for (int i = 0; i < mSwapchainImageViews.size(); i++)
			vkDestroyImageView(mDevice, mSwapchainImageViews[i], nullptr);
	}
}
