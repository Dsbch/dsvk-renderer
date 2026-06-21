#include <pch.h>
#include "gpuProfiler.h"

namespace engine
{
	void gpuProfiler::init(VkDevice device, deviceLimits limits)
	{
		mDevice = device;
		mQueryPool = {};

		mPoolCount = 2 << 10;
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

	error gpuProfiler::beginTimeStamp(VkCommandBuffer cmd, const std::string& slotName)
	{
		if (mUsedSlots.find(slotName) != mUsedSlots.end())
			return { "slot with such name was already recorded that frame" };

		if (mCurrentSlot >= mPoolCount)
			return { "slot count is reached" };

		vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQueryPool, mCurrentSlot);

		mUsedSlots[slotName] = { mCurrentSlot , mCurrentSlot + 1 };

		mCurrentSlot += 2;

		return {};
	}

	void gpuProfiler::endTimestamp(VkCommandBuffer cmd, const std::string& slotName)
	{
		if (mUsedSlots.find(slotName) != mUsedSlots.end())
		{
			auto slot = mUsedSlots[slotName];

			vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQueryPool, slot.second);
		}
	}

	void gpuProfiler::reset(VkCommandBuffer cmd)
	{
		mCurrentSlot = 0;

		mUsedSlots.clear();

		vkCmdResetQueryPool(cmd, mQueryPool, 0, mPoolCount);
	}

	std::map<std::string, float> gpuProfiler::getAllSlots()
	{
		std::map<std::string, float> result{};

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

		for (auto& [k, v] : mUsedSlots)
		{
			if (results[v.first].isAvailable != 0 && results[v.second].isAvailable != 0)
			{
				uint64_t start = results[v.first].value;
				uint64_t end = results[v.second].value;

				double elapsedNanoseconds = (end - start) * mDeviceLimits.timestampPeriod;

				double elapsedMiliSeconds = elapsedNanoseconds / 1000000.0;

				result[k] = (float(elapsedMiliSeconds));
			}
			else
				result[k] = std::nanf("");
		}

		return result;
	}
}