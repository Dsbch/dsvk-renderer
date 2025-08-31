#pragma once

#include <pch.h>

#include <VkBootstrap.h>

namespace engine
{
	struct immediateSubmit
	{
	public:
		immediateSubmit() 
			:
			mDevice(VK_NULL_HANDLE),
			mGraphicsQueue(VK_NULL_HANDLE),
			mFence(VK_NULL_HANDLE),
			mCommandPool(VK_NULL_HANDLE),
			mCommandBuffer(VK_NULL_HANDLE),
			mGraphicsQueueFamily(0)
		{}

		engine::error init(VkDevice mDevice, VkQueue graphicsQueue, uint32_t graphicsQueueFamily);
		void destroy();

		engine::error submit(std::function<void(VkCommandBuffer cmd)>&& function);
	private:
		VkDevice mDevice;
		VkQueue mGraphicsQueue;
		uint32_t mGraphicsQueueFamily;
		VkFence mFence;
		VkCommandPool mCommandPool;
		VkCommandBuffer mCommandBuffer;
	};
}