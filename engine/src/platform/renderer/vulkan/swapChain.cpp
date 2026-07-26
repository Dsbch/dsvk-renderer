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

	VkFormat swapChain::getResolveImageFormat() const
	{
		return mResolveImage.img.format;
	}

	VkExtent3D swapChain::getDrawImageExtent() const
	{
		return mDrawImage.img.extent;
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

	VkExtent3D swapChain::getResolveImageExtent() const
	{
		return mResolveImage.img.extent;
	}

	VkImage swapChain::getDrawImage(bool needResolve) const
	{
		if (needResolve)
			return mResolveImage.img.image;

		return mDrawImage.img.image;
	}

	VkImage swapChain::getDepthImage(bool needResolve) const
	{
		if (needResolve)
			return mDepthResolveImage.img.image;

		return mDepthImage.img.image;
	}

	VkImage swapChain::getAccumImage(bool needResolve) const
	{
		if (needResolve)
			return mAccumResolveImage.img.image;

		return mAccumImage.img.image;
	}

	VkImage swapChain::getRevealImage(bool needResolve) const
	{
		if (needResolve)
			return mRevealResolveImage.img.image;

		return mRevealImage.img.image;
	}

	VkImageView swapChain::getDrawImageView(bool needResolve) const
	{
		if (needResolve)
			return mResolveImage.img.view;

		return mDrawImage.img.view;
	}

	VkImageView swapChain::getDepthImageView(bool needResolve) const
	{
		if (needResolve)
			return mDepthResolveImage.img.view;

		return mDepthImage.img.view;
	}

	VkImageView swapChain::getAccumImageView(bool needResolve) const
	{
		if (needResolve)
			return mAccumResolveImage.img.view;

		return mAccumImage.img.view;
	}

	VkImageView swapChain::getRevealImageView(bool needResolve) const
	{
		if (needResolve)
			return mRevealResolveImage.img.view;

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
		drawImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageUsageFlags depthImageUsages{};
		depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;

		error err = mDrawImage.build(is, drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, false, sampleCounts(mPreset.msaa), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mResolveImage.build(is, drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mDepthImage.build(is, drawImageExtent, VK_FORMAT_D32_SFLOAT, depthImageUsages, false, sampleCounts(mPreset.msaa), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mDepthResolveImage.build(is, drawImageExtent, VK_FORMAT_D32_SFLOAT, depthImageUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		// Build images for OIT.
		const VkImageUsageFlags weightedUsages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		err = mAccumImage.build(is, drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, sampleCounts(mPreset.msaa), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mAccumResolveImage.build(is, drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mRevealImage.build(is, drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, sampleCounts(mPreset.msaa), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mRevealResolveImage.build(is, drawImageExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		// Build HZB.
		uint32_t mip0Width = (std::max)(1u, width >> 1);
		uint32_t mip0Height = (std::max)(1u, height >> 1);

		uint32_t hzbMipLevels = static_cast<uint32_t>(std::floor(std::log2((std::max)(mip0Width, mip0Height))));

		mHZB.clear();

		VkExtent3D mipExtent = { mip0Width, mip0Height, 1 };

		for (uint32_t l = 0; l < hzbMipLevels; l++)
		{
			const uint32_t mipWidth = (std::max)(1u, width >> (l + 1));
			const uint32_t mipHeight = (std::max)(1u, height >> (l + 1));

			vulkanImage currentDepth{};

			currentDepth.init(mDevice, mAllocator);

			mipExtent.width = mipWidth;
			mipExtent.height = mipHeight;

			err = currentDepth.build(is, mipExtent, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL);
			if (err)
				return err;

			mHZB.push_back(std::move(currentDepth));
		}

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

		mDrawImage.init(mDevice, mAllocator);
		mDepthImage.init(mDevice, mAllocator);
		mDepthResolveImage.init(mDevice, mAllocator);
		mResolveImage.init(mDevice, mAllocator);
		mAccumImage.init(mDevice, mAllocator);
		mRevealImage.init(mDevice, mAllocator);
		mAccumResolveImage.init(mDevice, mAllocator);
		mRevealResolveImage.init(mDevice, mAllocator);
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

		mDepthImage.destroy();
		mDepthResolveImage.destroy();
		mDrawImage.destroy();
		mResolveImage.destroy();
		mAccumResolveImage.destroy();
		mRevealResolveImage.destroy();
		mAccumImage.destroy();
		mRevealImage.destroy();

		for (auto& d : mHZB)
			d.destroy();

		vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

		mSwapchain = VK_NULL_HANDLE;

		for (int i = 0; i < mSwapchainImageViews.size(); i++)
			vkDestroyImageView(mDevice, mSwapchainImageViews[i], nullptr);
	}
	uint32_t swapChain::getHzbSize() const
	{
		return uint32_t(mHZB.size());
	}

	const std::vector<vulkanImage>& swapChain::getHZB() const
	{
		return mHZB;
	}

	void swapChain::transitionCurrentSwapChainImage(VkCommandBuffer cmd, VkImageLayout current, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask) const
	{
		transitionImage(
			cmd,
			getCurrentSwapChainImage(),
			getDrawImageFormat(),
			current,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask
		);
	}

	void swapChain::transitionDrawImage(
		VkCommandBuffer cmd,
		VkImageLayout current,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask
	) const
	{
		transitionImage(
			cmd,
			getDrawImage(false),
			getDrawImageFormat(),
			current,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask
		);

		if (mPreset.msaa > 1)
		{
			transitionImage(
				cmd,
				getDrawImage(true),
				getDrawImageFormat(),
				current,
				newLayout,
				srcStageMask,
				srcAccessMask,
				dstStageMask,
				dstAccessMask
			);
		}
	}

	void swapChain::transitionDepthImage(
		VkCommandBuffer cmd,
		VkImageLayout current,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask
	) const
	{
		transitionImage(
			cmd,
			getDepthImage(false),
			getDepthImageFormat(),
			current,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask
		);

		if (mPreset.msaa > 1)
		{
			transitionImage(
				cmd,
				getDepthImage(true),
				getDepthImageFormat(),
				current,
				newLayout,
				srcStageMask,
				srcAccessMask,
				dstStageMask,
				dstAccessMask
			);
		}
	}
	void swapChain::transitionAccumImage(
		VkCommandBuffer cmd,
		VkImageLayout current,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask
	) const
	{
		transitionImage(
			cmd,
			getAccumImage(false),
			getAccumImageFormat(),
			current,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask
		);

		if (mPreset.msaa > 1)
		{
			transitionImage(
				cmd,
				getAccumImage(true),
				getAccumImageFormat(),
				current,
				newLayout,
				srcStageMask,
				srcAccessMask,
				dstStageMask,
				dstAccessMask
			);
		}
	}

	void swapChain::transitionRevealImage(
		VkCommandBuffer cmd,
		VkImageLayout current,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask
	) const
	{
		transitionImage(
			cmd,
			getRevealImage(false),
			getRevealImageFormat(),
			current,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask
		);

		if (mPreset.msaa > 1)
		{
			transitionImage(
				cmd,
				getRevealImage(true),
				getRevealImageFormat(),
				current,
				newLayout,
				srcStageMask,
				srcAccessMask,
				dstStageMask,
				dstAccessMask
			);
		}
	}

	void swapChain::transitionHzbChainImages(VkCommandBuffer cmd, VkImageLayout current, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask) const
	{
		for (auto& hzb : getHZB())
		{
			transitionImage(
				cmd,
				hzb.img.image,
				hzb.img.format,
				current,
				newLayout,
				srcStageMask,
				srcAccessMask,
				dstStageMask,
				dstAccessMask
			);
		}
	}
}
