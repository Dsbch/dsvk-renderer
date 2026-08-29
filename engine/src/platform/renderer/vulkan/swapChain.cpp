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
		return mFrames[getCurrentFrameIndex()];
	}

	void swapChain::increment()
	{
		mFrameNumber++;
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
			if (e == VK_ERROR_OUT_OF_DATE_KHR)
				return { errCodeOutOfDateKHR, vkResultToStr(e) };

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

	uint32_t swapChain::getCurrentFrameIndex() const
	{
		return mFrameNumber % mFramesInFlight;
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

	error swapChain::createSwapChain(submit& is, uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ mChosenGPU, mDevice, mSurface };

		auto result = swapchainBuilder
			.set_desired_min_image_count(mFramesInFlight)
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
		
		return {};
	}

	error swapChain::build(submit& is, uint32_t width, uint32_t height, uint32_t graphicsQueueFamily)
	{
		VkCommandPoolCreateInfo commandPoolInfo = commandPoolCreateInfo(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		VkFenceCreateInfo fenceInfo = fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreInfo = semaphoreCreateInfo(0);

		for (uint32_t i = 0; i < mFramesInFlight; i++)
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

		return createSwapChain(is, width, height);
	}

	void swapChain::init(
		VmaAllocator vma,
		VkDevice device,
		VkSurfaceKHR surface,
		VkPhysicalDevice chosenGPU,
		submit& is,
		graphicsPreset preset
	)
	{
		mAllocator = vma;
		mDevice = device;
		mSurface = surface;
		mChosenGPU = chosenGPU;
		mPreset = preset;
	}

	void swapChain::destroy()
	{
		vkDeviceWaitIdle(mDevice);

		for (uint32_t i = 0; i < mFramesInFlight; i++)
		{
			vkDestroyCommandPool(mDevice, mFrames[i].commandPool, nullptr);
			vkDestroyFence(mDevice, mFrames[i].renderFence, nullptr);
			vkDestroySemaphore(mDevice, mRenderSema[i], nullptr);
			vkDestroySemaphore(mDevice, mFrames[i].swapchainSemaphore, nullptr);
		}

		vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

		mSwapchain = VK_NULL_HANDLE;

		for (int i = 0; i < mSwapchainImageViews.size(); i++)
			vkDestroyImageView(mDevice, mSwapchainImageViews[i], nullptr);
	}
	
	void swapChain::transitionCurrentSwapChainImage(VkCommandBuffer cmd, VkImageLayout current, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask) const
	{
		transitionImage(
			cmd,
			getCurrentSwapChainImage(),
			mSwapchainImageFormat,
			current,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask
		);
	}
}
