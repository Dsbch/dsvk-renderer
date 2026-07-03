#include <pch.h>
#include "submit.h"
#include "helper.h"

namespace engine
{
	error submit::init(std::shared_ptr<context> ctx, VkDevice device, VkQueue queue, uint32_t queueFamily)
	{
		mCommandPoolMutex = std::make_shared<co::mutex>();

		mDevice = device;

		mQueue = queue;
		mQueueFamily = queueFamily;

		VkCommandPoolCreateInfo cmdPoolInfo = commandPoolCreateInfo(mQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		VkResult vkres = (vkCreateCommandPool(mDevice, &cmdPoolInfo, nullptr, &mCommandPool));
		if (vkres != VK_SUCCESS)
		{
			return error{ vkResultToStr(vkres) };
		}

		VkCommandBufferAllocateInfo cmdAllocInfo = commandBufferAllocateInfo(mCommandPool, 1);

		vkres = (vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &mCommandBufferImmediate));
		if (vkres != VK_SUCCESS)
		{
			return error{ vkResultToStr(vkres) };
		}

		mWg.add(1);

		goCatch(
			[this, ctx, device]()
			{
				defer(mWg.done());

				while (mRunning || !semaToDelete.empty())
				{
					{
						co::mutex_guard m{ mu };

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
								LOGERROR("error from cleanUp thread on vkWaitSemaphores: {}", vkResultToStr(result));

							if (sema.second != nullptr)
								sema.second();

							semaToDelete.pop_back();

							LOGDEBUG("cleanup submit thread");
						}
					}

					co::sleep(10);
				}
			}
		);

		return {};
	}

	void submit::destroy()
	{
		deleteSemaInUse(semaInUse.size());

		mRunning = false;

		mWg.wait();

		if (mCommandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
	}

	error submit::immediate(const std::function<void(VkCommandBuffer cmd)>& function)
	{
		VkFence fence;
		VkFenceCreateInfo fenceInfo = fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

		VkResult vkres = vkCreateFence(mDevice, &fenceInfo, nullptr, &fence);
		if (vkres != VK_SUCCESS)
			return error{ vkResultToStr(vkres) };

		vkres = vkResetFences(mDevice, 1, &fence);
		if (vkres != VK_SUCCESS)
			return error{ vkResultToStr(vkres) };

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

	error submit::queue(const std::function<void(VkCommandBuffer cmd)>& function, const std::function<void()>& cleanUp)
	{
		auto queuedResult = queueCommand(function);
		if (!queuedResult)
			return queuedResult.err();

		{
			co::mutex_guard l{ mSubmitedCommandsMu };

			std::vector<VkSemaphoreSubmitInfo> semaSubmitInfo = { semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, queuedResult.value().sema) };
			VkCommandBufferSubmitInfo cmdinfo = commandBufferSubmitInfo(queuedResult.value().cmd);
			mSubmitedCommands.push_back({ std::move(cmdinfo), std::move(semaSubmitInfo) });
		}

		{
			co::mutex_guard m{ mu };
			semaInUse.push_back(
				std::pair<VkSemaphore, std::function<void()>>
			{
				queuedResult.value().sema,
					[cleanUp = cleanUp, device = mDevice, commandPool = mCommandPool, cmd = queuedResult.value().cmd, commandPoolMu = mCommandPoolMutex]()
					{
						{
							co::mutex_guard l{ *commandPoolMu.get() };
							vkFreeCommandBuffers(device, commandPool, 1, &cmd);
						}

						cleanUp();
					}
			});
		}

		return {};
	}

	error submit::queue(const std::function<void(VkCommandBuffer cmd)>& function)
	{
		auto queuedResult = queueCommand(function);
		if (!queuedResult)
			return queuedResult.err();

		{
			co::mutex_guard l{ mSubmitedCommandsMu };

			std::vector<VkSemaphoreSubmitInfo> semaSubmitInfo = { semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, queuedResult.value().sema) };
			VkCommandBufferSubmitInfo cmdinfo = commandBufferSubmitInfo(queuedResult.value().cmd);
			mSubmitedCommands.push_back({ std::move(cmdinfo), std::move(semaSubmitInfo) });
		}

		{
			co::mutex_guard m{ mu };
			semaInUse.push_back(
				std::pair<VkSemaphore, std::function<void()>>
			{
				queuedResult.value().sema,
					[device = mDevice, commandPool = mCommandPool, cmd = queuedResult.value().cmd, commandPoolMu = mCommandPoolMutex]()
					{
						{
							co::mutex_guard l{ *commandPoolMu.get() };
							vkFreeCommandBuffers(device, commandPool, 1, &cmd);
						}
					}
			});
		}

		return {};
	}

	std::vector<VkSubmitInfo2> submit::getSumbitedCommands()
	{
		co::mutex_guard l{ mSubmitedCommandsMu };

		std::vector<VkSubmitInfo2> result{};
		result.reserve(mSubmitedCommands.size());

		for (auto& c : mSubmitedCommands)
			result.push_back(submitInfo(&c.first, c.second));

		return result;
	}

	void submit::deleteSubmitedCommands(size_t idx)
	{
		co::mutex_guard l{ mSubmitedCommandsMu };

		mSubmitedCommands.erase(mSubmitedCommands.begin(), mSubmitedCommands.begin() + idx);
	}

	std::vector<VkSemaphore> submit::getCurrentSemaInUse()
	{
		co::mutex_guard m{ mu };

		std::vector<VkSemaphore> result;

		for (auto& s : semaInUse)
			result.push_back(s.first);

		return result;
	}

	void submit::deleteSemaInUse(size_t idx)
	{
		co::mutex_guard m{ mu };

		semaToDelete.insert(semaToDelete.end(), std::move_iterator(semaInUse.begin()), std::move_iterator(semaInUse.begin() + idx));

		semaInUse.erase(semaInUse.begin(), semaInUse.begin() + idx);
	}

	withError<submit::queuedCommand> submit::queueCommand(const std::function<void(VkCommandBuffer cmd)>& function)
	{
		submit::queuedCommand queueResult{};

		co::mutex_guard l{ *mCommandPoolMutex.get() };

		VkCommandBufferAllocateInfo cmdAllocInfo = commandBufferAllocateInfo(mCommandPool, 1);

		VkResult result = (vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &queueResult.cmd));
		if (result != VK_SUCCESS)
			return error{ vkResultToStr(result) };

		VkCommandBufferBeginInfo cmdBeginInfo = commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		result = vkBeginCommandBuffer(queueResult.cmd, &cmdBeginInfo);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		VkSemaphoreTypeCreateInfo timelineInfo = timelineSemaphoreCreateInfo(0);
		VkSemaphoreCreateInfo semaphoreInfo = semaphoreCreateInfo(0, &timelineInfo);

		result = vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &queueResult.sema);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		function(queueResult.cmd);

		result = vkEndCommandBuffer(queueResult.cmd);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		return queueResult;
	}
}
