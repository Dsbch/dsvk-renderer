#include <pch.h>
#include "gpuProfiler.h"

namespace engine
{
	void gpuProfiler::init(VkDevice device, uint32_t slotCount, deviceLimits limits)
	{
		mDevice = device;
		mQueryPool = {};

		mPoolCount = 2 * slotCount;
		mCurrentSlot = 0;

		mDeviceLimits = limits;
	}

	error gpuProfiler::createProfiling()
	{
		VkQueryPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		poolInfo.queryCount = mPoolCount;

		VkResult res = vkCreateQueryPool(mDevice, &poolInfo, nullptr, &mQueryPool);
		if (res != VK_SUCCESS)
			return { vkResultToStr(res) };

		return {};
	}

	void gpuProfiler::destroy()
	{
		if (mDevice && mQueryPool)
			vkDestroyQueryPool(mDevice, mQueryPool, nullptr);
	}

	error gpuProfiler::beginTimeStamp(VkCommandBuffer cmd)
	{
		if (mCurrentSlot >= mPoolCount)
			return { "slot count is reached" };

		vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQueryPool, mCurrentSlot);

		mCurrentSlot++;

		return {};
	}

	void gpuProfiler::endTimestamp(VkCommandBuffer cmd)
	{
		vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQueryPool, mCurrentSlot);
		mCurrentSlot++;
	}

	void gpuProfiler::reset(VkCommandBuffer cmd)
	{
		mCurrentSlot = 0;
		vkCmdResetQueryPool(cmd, mQueryPool, 0, mPoolCount);
	}

	std::vector<float> gpuProfiler::getAllSlots()
	{
		std::vector<float> result{};

		struct timeStampResult {
			uint64_t value;
			uint64_t isAvailable;
		};

		std::vector<timeStampResult> results{};
		results.resize(mPoolCount);

		VkResult queryRes = vkGetQueryPoolResults(
			mDevice, mQueryPool, 0, mPoolCount,
			results.size() * sizeof(timeStampResult),
			results.data(),
			sizeof(timeStampResult),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
		);
		if (queryRes != VK_SUCCESS && queryRes != VK_NOT_READY)
			return result;

		for (uint32_t i = 0; i < mPoolCount; i+=2)
		{
			if (results[i].isAvailable != 0 && results[i + 1].isAvailable != 0)
			{
				uint64_t start = results[i].value;
				uint64_t end = results[i+1].value;

				double elapsedNanoseconds = (end - start) * mDeviceLimits.timestampPeriod;

				double elapsedMiliSeconds = elapsedNanoseconds / 1000000.0;

				result.push_back(float(elapsedMiliSeconds));
			}
			else
				result.push_back(0);
		}

		return result;
	}
}