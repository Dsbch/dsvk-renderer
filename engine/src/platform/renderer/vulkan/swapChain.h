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
		{
		}

		void init(VmaAllocator vma, VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice chosenGPU);
		error build(uint32_t width, uint32_t height, uint32_t graphicsQueueFamily, graphicsPreset preset);
		void destroy();

		VkFormat getDrawImageFormat() const;
		VkFormat getDepthImageFormat() const;
		VkFormat getAccumImageFormat() const;
		VkFormat getRevealImageFormat() const;

		VkExtent3D getDrawImageExtent() const;
		VkExtent3D getResolveImageExtent() const;
		VkExtent3D getDepthImageExtent() const;
		VkExtent3D getAccumImageExtent() const;
		VkExtent3D getRevealImageExtent() const;

		VkImage getDrawImage() const;
		VkImage getDepthImage() const;
		VkImage getResolveImage() const;
		VkImage getAccumResolveImage() const;
		VkImage getRevealResolveImage() const;
		VkImage getAccumImage() const;
		VkImage getRevealImage() const;

		VkImageView getDrawImageView() const;
		VkImageView getDepthImageView() const;
		VkImageView getResolveImageView() const;
		VkImageView getAccumResolveImageView() const;
		VkImageView getRevealResolveImageView() const;
		VkImageView getAccumImageView() const;
		VkImageView getRevealImageView() const;

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
		error createSwapChain(uint32_t width, uint32_t height, graphicsPreset preset);

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
		vulkanImage mDepthImage;
		vulkanImage mResolveImage;

		uint32_t mFrameNumber;
		uint32_t mSwapchainIndex;

		std::array<frameData, FRAME_OVERLAP> mFrames;
		std::array<VkSemaphore, FRAME_OVERLAP> mRenderSema;
	};
}