#pragma once

#include <pch.h>

#include "gpuProfiler.h"
#include "include.h"

namespace engine
{
	// Container for all generic vulkan stuff: device, allocator, instance, swapChain etc.
	class vulkanContext
	{
	public:
		vulkanContext() = default;
		vulkanContext(const vulkanContext&) = delete;
		
		vulkanContext(std::shared_ptr<context> ctx, std::shared_ptr<window> window);
		
		error checkError();

		error changeViewPort(uint32_t width, uint32_t height);

		PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT;
		PFN_vkCmdDrawMeshTasksIndirectEXT vkCmdDrawMeshTasksIndirectEXT;
		VkDebugUtilsMessengerEXT debugMessenger;

		VkDevice device;
		VmaAllocator allocator;
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		deviceLimits deviceLimits;
		VkPhysicalDeviceProperties deviceProps;
		VkQueue graphicsQueue;
		uint32_t graphicsQueueFamily;
		VkSurfaceKHR surface;

		graphicsPreset preset;

		swapChain sChain;
		submit iSubmit;
		deletionQueue delQueue;

		gpuProfiler profiler;
	private:
		std::shared_ptr<context> mCtx;
		error mErr;

		error initVulkan(std::shared_ptr<window> window);
		error setLimits();
		error initImmediateSubmit();
		error initSwapchain(uint32_t width, uint32_t height);
		error loadExtensions();
		void chooseGraphicsPreset();
	};
}