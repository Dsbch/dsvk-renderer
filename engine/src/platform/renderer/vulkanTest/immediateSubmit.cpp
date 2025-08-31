#include <pch.h>
#include "immediateSubmit.h"
#include "vkHelper.h"

namespace vktest
{
	engine::error immediateSubmit::init(VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily)
	{
		mDevice = device;

		mGraphicsQueue = graphicsQueue;
		mGraphicsQueueFamily = graphicsQueueFamily;

		VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
		VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(mGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		auto vkres = (vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mFence));
		if (vkres != VK_SUCCESS)
		{
			return engine::error{ vkResultToStr(vkres) };
		}

		vkres = (vkCreateCommandPool(mDevice, &commandPoolInfo, nullptr, &mCommandPool));
		if (vkres != VK_SUCCESS)
		{
			return engine::error{ vkResultToStr(vkres) };
		}

		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(mCommandPool, 1);

		vkres = (vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &mCommandBuffer));
		if (vkres != VK_SUCCESS)
		{
			return engine::error{ vkResultToStr(vkres) };
		}

		return {};
	}

	void immediateSubmit::destroy()
	{
		if (mFence != VK_NULL_HANDLE)
			vkDestroyFence(mDevice, mFence, nullptr);

		if (mCommandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
	}

	engine::error immediateSubmit::submit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		auto result = vkResetFences(mDevice, 1, &mFence);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		result = vkResetCommandBuffer(mCommandBuffer, 0);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		result = vkBeginCommandBuffer(mCommandBuffer, &cmdBeginInfo);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		function(mCommandBuffer);

		result = vkEndCommandBuffer(mCommandBuffer);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(mCommandBuffer);
		VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, nullptr, nullptr);

		// submit command buffer to the queue and execute it.
		//  _renderFence will now block until the graphic commands finish execution
		result = vkQueueSubmit2(mGraphicsQueue, 1, &submit, mFence);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		result = vkWaitForFences(mDevice, 1, &mFence, true, 10000000000);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		return {};
	}
}
