#pragma once

#include <pch.h>

#include <VkBootstrap.h>
#include "base/context/context.h"

namespace engine
{
	struct submit
	{
	public:
		submit() 
			:
			mDevice(VK_NULL_HANDLE),
			mQueue(VK_NULL_HANDLE),
			mCommandPool(VK_NULL_HANDLE),
			mCommandBufferImmediate(VK_NULL_HANDLE),
			mQueueFamily(0)
		{}

		engine::error init(std::shared_ptr<context> ctx, VkDevice mDevice, VkQueue queue, uint32_t queueFamily);
		void destroy();

		engine::error immediate(std::function<void(VkCommandBuffer cmd)>&& function);
		engine::error queue(std::function<void(VkCommandBuffer cmd)>&& function, std::function<void()>&& cleanUp);

		std::vector<VkSemaphore> getCurrentSemaInUse();
		void markAllSemaAsUsed();
	private:
		std::mutex mMu;

		VkDevice mDevice;
		VkQueue mQueue;
		uint32_t mQueueFamily;
		VkCommandPool mCommandPool;
		VkCommandBuffer mCommandBufferImmediate;

		static std::mutex mu;
		static std::vector<std::pair<VkSemaphore, std::function<void()>>> semaInUse;
		static std::vector<std::pair<VkSemaphore, std::function<void()>>> semaToDelete;
		static std::once_flag onceFlag;
	};
}