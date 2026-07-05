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

	const static uint32_t FRAME_OVERLAP = 3;

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
		swapChain() :
			mAllocator(VK_NULL_HANDLE),
			mDevice(VK_NULL_HANDLE),
			mSurface(VK_NULL_HANDLE),
			mChosenGPU(VK_NULL_HANDLE),
			mSwapchain(VK_NULL_HANDLE),
			mSwapchainImageFormat(VK_FORMAT_B8G8R8A8_UNORM),
			mFrames(
				{
					frameData{.commandPool = VK_NULL_HANDLE, .commandBuffer = VK_NULL_HANDLE, .swapchainSemaphore = VK_NULL_HANDLE, .renderFence = VK_NULL_HANDLE },
					frameData{.commandPool = VK_NULL_HANDLE, .commandBuffer = VK_NULL_HANDLE, .swapchainSemaphore = VK_NULL_HANDLE, .renderFence = VK_NULL_HANDLE },
					frameData{.commandPool = VK_NULL_HANDLE, .commandBuffer = VK_NULL_HANDLE, .swapchainSemaphore = VK_NULL_HANDLE, .renderFence = VK_NULL_HANDLE }
				}
			),
			mRenderSema(
				{
					VK_NULL_HANDLE,
					VK_NULL_HANDLE,
					VK_NULL_HANDLE,
				}
				),
			mDepthImage(),
			mDrawImage(),
			mSwapchainExtent(),
			mSwapchainIndex(0),
			mFrameNumber(0)
		{}

		void init(VmaAllocator vma, VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice chosenGPU, submit& is, graphicsPreset preset);
		error build(submit& is, uint32_t width, uint32_t height, uint32_t graphicsQueueFamily);
		void destroy();

		uint32_t getHzbSize() const;
		const std::vector<vulkanImage>& getHZB() const;

		void transitionCurrentSwapChainImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionDrawImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionDepthImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionAccumImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionRevealImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionHzbChainImages(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;

		VkFormat getDrawImageFormat() const;
		VkFormat getDepthImageFormat() const;
		VkFormat getAccumImageFormat() const;
		VkFormat getRevealImageFormat() const;
		VkFormat getResolveImageFormat() const;

		VkExtent3D getDrawImageExtent() const;
		VkExtent3D getDepthImageExtent() const;
		VkExtent3D getAccumImageExtent() const;
		VkExtent3D getRevealImageExtent() const;
		VkExtent3D getResolveImageExtent() const;

		VkImage getDrawImage(bool needResolve) const;
		VkImage getDepthImage(bool needResolve) const;
		VkImage getAccumImage(bool needResolve) const;
		VkImage getRevealImage(bool needResolve) const;

		VkImageView getDrawImageView(bool needResolve) const;
		VkImageView getDepthImageView(bool needResolve) const;
		VkImageView getAccumImageView(bool needResolve) const;
		VkImageView getRevealImageView(bool needResolve) const;

		void pickImageExtent();

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
		std::vector<VkImage> mSwapchainImages;
		std::vector<VkImageView> mSwapchainImageViews;
		VkExtent2D mSwapchainExtent;

		// For OIT blending.
		vulkanImage mAccumImage;
		vulkanImage mRevealImage;
		vulkanImage mAccumResolveImage;
		vulkanImage mRevealResolveImage;

		// For opaque geometry.
		vulkanImage mDrawImage;
		vulkanImage mResolveImage;
		vulkanImage mDepthImage;
		vulkanImage mDepthResolveImage;

		std::vector<vulkanImage> mHZB;

		uint32_t mFrameNumber;
		uint32_t mSwapchainIndex;

		graphicsPreset mPreset;

		std::array<frameData, FRAME_OVERLAP> mFrames;
		std::array<VkSemaphore, FRAME_OVERLAP> mRenderSema;
	};
}