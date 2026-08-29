#pragma once

#include <pch.h>

#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#include "helper.h"
#include "image.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	VkPresentInfoKHR presentInfo();

	struct frameData
	{
		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;
		VkSemaphore swapchainSemaphore;
		VkFence renderFence;
	};

	struct swapChain
	{
	public:
		swapChain(uint32_t framesInFlight) :
			mAllocator(VK_NULL_HANDLE),
			mDevice(VK_NULL_HANDLE),
			mSurface(VK_NULL_HANDLE),
			mChosenGPU(VK_NULL_HANDLE),
			mSwapchain(VK_NULL_HANDLE),
			mSwapchainImageFormat(VK_FORMAT_B8G8R8A8_UNORM),
			mFrames(framesInFlight),
			mRenderSema(framesInFlight),
			mSwapchainExtent(),
			mSwapchainIndex(0),
			mFrameNumber(0),
			mFramesInFlight(framesInFlight)
		{}

		void init(VmaAllocator vma, VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice chosenGPU, submit& is, graphicsPreset preset);
		error build(submit& is, uint32_t width, uint32_t height, uint32_t graphicsQueueFamily);
		void destroy();

		void transitionCurrentSwapChainImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;

		VkFormat getSwapChainImageFormat() const;
		VkExtent2D& getSwapChainExtent();
		VkSwapchainKHR& getSwapChain();

		VkImage getCurrentSwapChainImage() const;
		VkImageView getCurrentSwapChainImageView() const;

		error acquireImageIndex();
		error waitOnRenderFence();
		error resetRenderFence();
		error resetCommandBuffer();
		error present(VkQueue graphicQueue);
		VkCommandBuffer getCommandBuffer();
		VkSemaphore getSwapchainSemaphore();
		VkSemaphore getRenderSemaphore();
		VkFence getRenderFence();

		uint32_t getCurrentFrameIndex() const;
	private:
		void increment();
		error createSwapChain(submit& is, uint32_t width, uint32_t height);

		frameData& getCurrentFrameData();

		// VMA.
		VmaAllocator mAllocator;
		// Vulkan device for commands.
		VkDevice mDevice;
		// Vulkan window surface.
		VkSurfaceKHR mSurface;
		// physical device.
		VkPhysicalDevice mChosenGPU;

		// swap chain stuff.
		VkSwapchainKHR mSwapchain;
		VkFormat mSwapchainImageFormat;
		VkExtent2D mSwapchainExtent;
		
		// That index is owned by GPU.
		// Have to index swapchain images and render sema's with that index.
		uint32_t mSwapchainIndex;
		std::vector<VkSemaphore> mRenderSema;
		std::vector<VkImage> mSwapchainImages;
		std::vector<VkImageView> mSwapchainImageViews;

		// CPU side index used to index CPU side per frame data.
		uint32_t mFrameNumber;
		uint32_t mFramesInFlight;
		graphicsPreset mPreset;
		std::vector<frameData> mFrames;
	};
}