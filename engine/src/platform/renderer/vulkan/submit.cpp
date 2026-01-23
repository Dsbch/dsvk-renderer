#include <pch.h>
#include "submit.h"
#include "helper.h"

namespace engine
{
	std::mutex submit::mu;
	std::once_flag submit::onceFlag;
	std::vector<std::pair<VkSemaphore, std::function<void()>>> submit::semaInUse;
	std::vector<std::pair<VkSemaphore, std::function<void()>>> submit::semaToDelete;

	engine::error submit::init(std::shared_ptr<context> ctx, VkDevice device, VkQueue queue, uint32_t queueFamily)
	{
		mDevice = device;

		mQueue = queue;
		mQueueFamily = queueFamily;

		VkCommandPoolCreateInfo cmdPoolInfo = commandPoolCreateInfo(mQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		VkResult vkres = (vkCreateCommandPool(mDevice, &cmdPoolInfo, nullptr, &mCommandPool));
		if (vkres != VK_SUCCESS)
		{
			return engine::error{ vkResultToStr(vkres) };
		}

		VkCommandBufferAllocateInfo cmdAllocInfo = commandBufferAllocateInfo(mCommandPool, 1);

		vkres = (vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &mCommandBufferImmediate));
		if (vkres != VK_SUCCESS)
		{
			return engine::error{ vkResultToStr(vkres) };
		}

		std::call_once(
			onceFlag,
			[ctxPtr = ctx.get(), device = mDevice]()
			{
				ctxPtr->mThreadPool->start(
					[ctxPtr = ctxPtr, device = device]()
					{
						while (ctxPtr->mThreadPool->isThreadPoolRunning() || !semaToDelete.empty())
						{
							{
								std::lock_guard m{ mu };

								while (!semaToDelete.empty())
								{
									auto sema = semaToDelete.back();

									VkSemaphoreWaitInfo waitInfo;
									uint64_t waitVal = 1;

									waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
									waitInfo.pNext = NULL;
									waitInfo.flags = 0;
									waitInfo.semaphoreCount = 1;
									waitInfo.pSemaphores = &sema.first;
									waitInfo.pValues = &waitVal;

									VkResult result = vkWaitSemaphores(device, &waitInfo, UINT64_MAX);
									if (result != VK_SUCCESS)
									{
										LOGERROR("error from cleanUp thread on vkWaitSemaphores: {}", vkResultToStr(result));
									}
								
									if (sema.second != nullptr)
										sema.second();

									semaToDelete.pop_back();
								}
							}

							std::this_thread::sleep_for(std::chrono::milliseconds(10));
						}
					}
				);
			}
		);

		return {};
	}

	void submit::destroy()
	{
		if (mCommandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
	}

	engine::error submit::immediate(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		std::lock_guard l{ mMu };

		VkFence fence;
		VkFenceCreateInfo fenceInfo = fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

		VkResult vkres = vkCreateFence(mDevice, &fenceInfo, nullptr, &fence);
		if (vkres != VK_SUCCESS)
		{
			return engine::error{ vkResultToStr(vkres) };
		}

		vkres = vkResetFences(mDevice, 1, &fence);
		if (vkres != VK_SUCCESS)
		{
			return engine::error{ vkResultToStr(vkres) };
		}

		vkres = vkResetCommandBuffer(mCommandBufferImmediate, 0);
		if (vkres != VK_SUCCESS)
			return { vkResultToStr(vkres) };

		VkCommandBufferBeginInfo cmdBeginInfo = commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		vkres = vkBeginCommandBuffer(mCommandBufferImmediate, &cmdBeginInfo);
		if (vkres != VK_SUCCESS)
			return { vkResultToStr(vkres) };

		function(mCommandBufferImmediate);

		vkres = vkEndCommandBuffer(mCommandBufferImmediate);
		if (vkres != VK_SUCCESS)
			return { vkResultToStr(vkres) };

		VkCommandBufferSubmitInfo cmdinfo = commandBufferSubmitInfo(mCommandBufferImmediate);
		VkSubmitInfo2 submit = submitInfo(&cmdinfo);

		vkres = vkQueueSubmit2(mQueue, 1, &submit, fence);
		if (vkres != VK_SUCCESS)
			return { vkResultToStr(vkres) };

		vkres = vkWaitForFences(mDevice, 1, &fence, true, MAXUINT);
		if (vkres != VK_SUCCESS)
			return { vkResultToStr(vkres) };

		vkDestroyFence(mDevice, fence, nullptr);

		return {};
	}

	engine::error submit::queue(std::function<void(VkCommandBuffer cmd)>&& function, std::function<void()>&& cleanUp)
	{
		std::lock_guard l{ mMu };

		VkCommandBuffer cmd;
		VkCommandBufferAllocateInfo cmdAllocInfo = commandBufferAllocateInfo(mCommandPool, 1);

		VkResult result = (vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &cmd));
		if (result != VK_SUCCESS)
		{
			return engine::error{ vkResultToStr(result) };
		}

		VkCommandBufferBeginInfo cmdBeginInfo = commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		result = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		VkSemaphore sema;
		VkSemaphoreTypeCreateInfo timelineInfo = timelineSemaphoreCreateInfo(0);
		VkSemaphoreCreateInfo semaphoreInfo = semaphoreCreateInfo(0, &timelineInfo);

		result = vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &sema);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		std::vector<VkSemaphoreSubmitInfo> semaSubmitInfo{};
		semaSubmitInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, sema));

		VkCommandBufferSubmitInfo cmdinfo = commandBufferSubmitInfo(cmd);
		VkSubmitInfo2 submit = submitInfo(&cmdinfo, semaSubmitInfo);

		function(cmd);

		result = vkEndCommandBuffer(cmd);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		result = vkQueueSubmit2(mQueue, 1, &submit, VK_NULL_HANDLE);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		{
			std::lock_guard m{ mu };
			semaInUse.push_back(std::pair<VkSemaphore, std::function<void()>>{ sema, cleanUp });
		}

		return {};
	}

	std::vector<VkSemaphore> submit::getCurrentSemaInUse()
	{
		std::lock_guard m{ mu };

		std::vector<VkSemaphore> result;

		for (auto& s : semaInUse)
			result.push_back(s.first);

		return result;
	}

	void submit::markAllSemaAsUsed()
	{
		std::lock_guard m{ mu };

		semaToDelete.insert(semaToDelete.end(), std::move_iterator(semaInUse.begin()), std::move_iterator(semaInUse.end()));

		semaInUse.clear();
	}
}
