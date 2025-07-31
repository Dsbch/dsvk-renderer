#pragma once

#include <pch.h>

#include <vma/vk_mem_alloc.h>
#include <VkBootstrap.h>

namespace vktest
{
	const static uint32_t FRAME_OVERLAP = 3;

	struct vulkanImage
	{
		VkImage image;
		VkImageView imageView;
		VmaAllocation allocation;
		VkExtent3D imageExtent;
		VkFormat imageFormat;
	};

	struct frameData
	{
		VkCommandPool commandPool;
		VkCommandBuffer commandBuffer;

		VkSemaphore swapchainSemaphore, renderSemaphore;
		VkFence renderFence;
	};

	class swapChain
	{
	public:
		swapChain(VmaAllocator vma, VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice chosenGPU) :
			mAllocator(vma),
			mDevice(device),
			mSurface(surface),
			mChosenGPU(chosenGPU),
			mSwapchain(VK_NULL_HANDLE),
			mSwapchainImageFormat(VK_FORMAT_B8G8R8A8_UNORM),
			mSwapchainExtent(),
			mFrameNumber(0),
			mFrames(
				{
					frameData{.commandPool = VK_NULL_HANDLE, .commandBuffer = VK_NULL_HANDLE, .swapchainSemaphore = VK_NULL_HANDLE, .renderSemaphore = VK_NULL_HANDLE, .renderFence = VK_NULL_HANDLE },
					frameData{.commandPool = VK_NULL_HANDLE, .commandBuffer = VK_NULL_HANDLE, .swapchainSemaphore = VK_NULL_HANDLE, .renderSemaphore = VK_NULL_HANDLE, .renderFence = VK_NULL_HANDLE },
					frameData{.commandPool = VK_NULL_HANDLE, .commandBuffer = VK_NULL_HANDLE, .swapchainSemaphore = VK_NULL_HANDLE, .renderSemaphore = VK_NULL_HANDLE, .renderFence = VK_NULL_HANDLE }
				}
			)
		{
		}

		engine::error init(uint32_t width, uint32_t height, uint32_t graphicsQueueFamily);
		void destroy();

		VkFormat getImageFormat();
		engine::error resize(uint32_t width, uint32_t height);
		frameData& getCurrentFrameData();
		vulkanImage& getDrawImage();
		void inrement();

		void pickImageExtent();

		VkExtent2D& getSwapChainExtent();
		VkSwapchainKHR& getSwapChain();
		std::vector<VkImage> getSwapChainImages();
		std::vector<VkImageView> getSwapChainImageViews();
		VkFormat getSwapChainImageFormat();
	private:
		engine::error createSwapChain(uint32_t width, uint32_t height);

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
		
		vulkanImage mDrawImage;

		uint32_t mFrameNumber;
		// frame data, relates to swap chain.
		// We have triple buffered swap chain.
		std::array<frameData, FRAME_OVERLAP> mFrames;
	};
}