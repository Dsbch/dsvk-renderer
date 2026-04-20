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
			mQueueFamily(0),
			mRunning(true)
		{}

		engine::error init(std::shared_ptr<context> ctx, VkDevice mDevice, VkQueue queue, uint32_t queueFamily);
		void destroy();

		engine::error immediate(const std::function<void(VkCommandBuffer cmd)>&& function);
		engine::error queue(const std::function<void(VkCommandBuffer cmd)>&& function, std::function<void()>&& cleanUp);

		std::vector<VkSubmitInfo2> getSumbitedCommands();
		void deleteSubmitedCommands(size_t idx);
		std::vector<VkSemaphore> getCurrentSemaInUse();
		void deleteSemaInUse(size_t idx);
	private:
		co::wait_group mWg;
		std::atomic<bool> mRunning;

		VkDevice mDevice;
		VkQueue mQueue;
		uint32_t mQueueFamily;
		VkCommandPool mCommandPool;
		std::shared_ptr<co::mutex> mCommandPoolMutex;
		VkCommandBuffer mCommandBufferImmediate;
		co::mutex mSubmitedCommandsMu;
		std::vector<std::pair<VkCommandBufferSubmitInfo, std::vector<VkSemaphoreSubmitInfo>>> mSubmitedCommands;

		co::mutex mu;
		std::vector<std::pair<VkSemaphore, std::function<void()>>> semaInUse;
		std::vector<std::pair<VkSemaphore, std::function<void()>>> semaToDelete;
	};
}