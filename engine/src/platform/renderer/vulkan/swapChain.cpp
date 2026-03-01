#include <pch.h>

#include "swapChain.h"

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

	VkFormat swapChain::getDrawImageFormat() const
	{
		return mDrawImage.img.format;
	}

	VkFormat swapChain::getDepthImageFormat() const
	{
		return mDepthImage.img.format;
	}

	VkFormat swapChain::getAccumImageFormat() const
	{
		return mAccumImage.img.format;
	}

	VkFormat swapChain::getRevealImageFormat() const
	{
		return mRevealImage.img.format;
	}

	VkExtent3D swapChain::getDrawImageExtent() const
	{
		return mDrawImage.img.extent;
	}

	VkExtent3D swapChain::getResolveImageExtent() const
	{
		return mResolveImage.img.extent;
	}

	VkExtent3D swapChain::getDepthImageExtent() const
	{
		return mDepthImage.img.extent;
	}

	VkExtent3D swapChain::getAccumImageExtent() const
	{
		return mAccumImage.img.extent;
	}

	VkExtent3D swapChain::getRevealImageExtent() const
	{
		return mRevealImage.img.extent;
	}

	VkImage swapChain::getDrawImage() const
	{
		return mDrawImage.img.image;
	}

	VkImage swapChain::getDepthImage() const
	{
		return mDepthImage.img.image;
	}

	VkImage swapChain::getResolveImage() const
	{
		return mResolveImage.img.image;
	}

	VkImage swapChain::getAccumResolveImage() const
	{
		return mAccumResolveImage.img.image;
	}

	VkImage swapChain::getRevealResolveImage() const
	{
		return mRevealResolveImage.img.image;
	}

	VkImage swapChain::getAccumImage() const
	{
		return mAccumImage.img.image;
	}

	VkImage swapChain::getRevealImage() const
	{
		return mRevealImage.img.image;
	}

	VkImageView swapChain::getDrawImageView() const
	{
		return mDrawImage.img.view;
	}

	VkImageView swapChain::getDepthImageView() const
	{
		return mDepthImage.img.view;
	}

	VkImageView swapChain::getResolveImageView() const
	{
		return mResolveImage.img.view;
	}

	VkImageView swapChain::getAccumResolveImageView() const
	{
		return mAccumResolveImage.img.view;
	}

	VkImageView swapChain::getRevealResolveImageView() const
	{
		return mRevealResolveImage.img.view;
	}

	VkImageView swapChain::getAccumImageView() const
	{
		return mAccumImage.img.view;
	}

	VkImageView swapChain::getRevealImageView() const
	{
		return mRevealImage.img.view;
	}

	void swapChain::increment()
	{
		mFrameNumber++;
	}

	void swapChain::pickImageExtent()
	{
		// here we pick needed height and width of our images.
		mDrawImage.img.extent.height = std::min(mSwapchainExtent.height, mDrawImage.img.extent.height);
		mDrawImage.img.extent.width = std::min(mSwapchainExtent.width, mDrawImage.img.extent.width);
	}

	VkSwapchainKHR& swapChain::getSwapChain()
	{
		return mSwapchain;
	}

	VkImage swapChain::getCurrentSwapChainImage() const
	{
		return mSwapchainImages[mSwapchainIndex];
	}

	VkImageView swapChain::getCurrentSwapChainImageView() const
	{
		return mSwapchainImageViews[mSwapchainIndex];
	}

	VkFormat swapChain::getSwapChainImageFormat() const
	{
		return mSwapchainImageFormat;
	}

	error swapChain::present(VkQueue graphicQueue)
	{
		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presInfo = presentInfo();

		presInfo.pSwapchains = &mSwapchain;
		presInfo.swapchainCount = 1;

		presInfo.pWaitSemaphores = &mRenderSema[mSwapchainIndex];
		presInfo.waitSemaphoreCount = 1;

		presInfo.pImageIndices = &mSwapchainIndex;

		auto result = vkQueuePresentKHR(graphicQueue, &presInfo);
		if (result != VK_SUCCESS)
			return vkResultToStr(result);

		increment();

		return {};
	}

	error swapChain::acquireImageIndex()
	{
		VkResult e = vkAcquireNextImageKHR(mDevice, mSwapchain, 1000000000, getCurrentFrameData().swapchainSemaphore, nullptr, &mSwapchainIndex);
		if (e != VK_SUCCESS)
		{
			return { vkResultToStr(e) };
		}

		return {};
	}

	error swapChain::waitOnRenderFence()
	{
		auto result = vkWaitForFences(mDevice, 1, &getCurrentFrameData().renderFence, true, 1000000000);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		return {};
	}

	error swapChain::resetRenderFence()
	{
		auto result = vkResetFences(mDevice, 1, &getCurrentFrameData().renderFence);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		return {};
	}

	VkCommandBuffer swapChain::getCommandBuffer()
	{
		return getCurrentFrameData().commandBuffer;
	}

	VkSemaphore swapChain::getSwapchainSemaphore()
	{
		return getCurrentFrameData().swapchainSemaphore;
	}

	VkSemaphore swapChain::getRenderSemaphore()
	{
		return mRenderSema[mSwapchainIndex];
	}

	VkFence swapChain::getRenderFence()
	{
		return getCurrentFrameData().renderFence;
	}

	error swapChain::resetCommandBuffer()
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

	error swapChain::createSwapChain(uint32_t width, uint32_t height, graphicsPreset preset)
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

		mDrawImage.img.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		mDrawImage.img.extent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		drawImageUsages |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		error err = mDrawImage.build(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, false, sampleCounts(preset.msaa));
		if (err)
			return err;

		err = mResolveImage.build(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, false, VK_SAMPLE_COUNT_1_BIT);
		if (err)
			return err;

		err = mAccumResolveImage.build(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages | VK_IMAGE_USAGE_SAMPLED_BIT, false, VK_SAMPLE_COUNT_1_BIT);
		if (err)
			return err;

		err = mRevealResolveImage.build(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages | VK_IMAGE_USAGE_SAMPLED_BIT, false, VK_SAMPLE_COUNT_1_BIT);
		if (err)
			return err;

		// build depth image.
		VkImageUsageFlags depthImageUsages{};
		depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthImageUsages |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		err = mDepthImage.build(drawImageExtent, VK_FORMAT_D32_SFLOAT, depthImageUsages, false, sampleCounts(preset.msaa));
		if (err)
			return err;

		// Build images for OIT.
		const VkImageUsageFlags weightedUsages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		err = mAccumImage.build(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, sampleCounts(preset.msaa));
		if (err)
			return err;

		err = mRevealImage.build(drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, sampleCounts(preset.msaa));
		if (err)
			return err;

		return {};
	}

	error swapChain::build(uint32_t width, uint32_t height, uint32_t graphicsQueueFamily, graphicsPreset preset)
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

			result = vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderSema[i]);
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

		return createSwapChain(width, height, preset);
	}

	void swapChain::init(
		VmaAllocator vma, 
		VkDevice device, 
		VkSurfaceKHR surface, 
		VkPhysicalDevice chosenGPU 
	)
	{
		mAllocator = vma;
		mDevice = device;
		mSurface = surface;
		mChosenGPU = chosenGPU;

		mDrawImage.init(mDevice, mAllocator);
		mDepthImage.init(mDevice, mAllocator);
		mResolveImage.init(mDevice, mAllocator);
		mAccumImage.init(mDevice, mAllocator);
		mRevealImage.init(mDevice, mAllocator);
		mAccumResolveImage.init(mDevice, mAllocator);
		mRevealResolveImage.init(mDevice, mAllocator);
	}

	void swapChain::destroy()
	{
		vkDeviceWaitIdle(mDevice);

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			vkDestroyCommandPool(mDevice, mFrames[i].commandPool, nullptr);
			vkDestroyFence(mDevice, mFrames[i].renderFence, nullptr);
			vkDestroySemaphore(mDevice, mRenderSema[i], nullptr);
			vkDestroySemaphore(mDevice, mFrames[i].swapchainSemaphore, nullptr);
		}

		mDepthImage.destroy();
		mDrawImage.destroy();
		mResolveImage.destroy();
		mAccumResolveImage.destroy();
		mRevealResolveImage.destroy();
		mAccumImage.destroy();
		mRevealImage.destroy();

		vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

		mSwapchain = VK_NULL_HANDLE;

		for (int i = 0; i < mSwapchainImageViews.size(); i++)
			vkDestroyImageView(mDevice, mSwapchainImageViews[i], nullptr);
	}
}
