#include <pch.h>
#include "gpuProfiler.h"

namespace engine
{
	void gpuProfiler::init(VkDevice device, deviceLimits limits, uint32_t framesInFlight)
	{
		mDevice = device;
		mQueryPool = {};

		mPoolCount = 4096;
		mCurrentSlot = 0;

		mDeviceLimits = limits;

		mUsedSlots.resize(framesInFlight);
		mQueryPool.resize(framesInFlight);
	}

	error gpuProfiler::createProfiling(submit& is)
	{
		for (uint32_t i = 0; i < mQueryPool.size(); i++)
		{
			VkQueryPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
			poolInfo.queryCount = mPoolCount;

			VkResult res = vkCreateQueryPool(mDevice, &poolInfo, nullptr, &mQueryPool[i]);
			if (res != VK_SUCCESS)
				return { vkResultToStr(res) };

			error err = is.immediate([pool = mQueryPool[i], poolCount = mPoolCount](VkCommandBuffer cmd)
				{
					vkCmdResetQueryPool(cmd, pool, 0, poolCount);
				}
			);
			if (err)
				return err;
		}

		return {};
	}

	void gpuProfiler::destroy()
	{
		for (auto& pool : mQueryPool)
			vkDestroyQueryPool(mDevice, pool, nullptr);
	}

	error gpuProfiler::beginTimeStamp(VkCommandBuffer cmd, const std::string& slotName, uint32_t frameIndex)
	{
		if (mUsedSlots[frameIndex].find(slotName) != mUsedSlots[frameIndex].end())
			return { "slot with such name was already recorded that frame" };

		if (mCurrentSlot >= mPoolCount)
			return { "slot count is reached" };

		vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, mQueryPool[frameIndex], mCurrentSlot);

		mUsedSlots[frameIndex][slotName] = { mCurrentSlot , mCurrentSlot + 1 };

		mCurrentSlot += 2;

		return {};
	}

	void gpuProfiler::endTimestamp(VkCommandBuffer cmd, const std::string& slotName, uint32_t frameIndex)
	{
		if (mUsedSlots[frameIndex].find(slotName) != mUsedSlots[frameIndex].end())
		{
			auto slot = mUsedSlots[frameIndex][slotName];

			vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQueryPool[frameIndex], slot.second);
		}
	}

	void gpuProfiler::reset(VkCommandBuffer cmd, uint32_t frameIndex)
	{
		mCurrentSlot = 0;

		mUsedSlots[frameIndex].clear();

		vkCmdResetQueryPool(cmd, mQueryPool[frameIndex], 0, mPoolCount);
	}

	std::map<std::string, float> gpuProfiler::getAllSlots(uint32_t frameIndex)
	{
		std::map<std::string, float> result{};

		struct timeStampResult {
			uint64_t value;
			uint64_t isAvailable;
		};

		std::vector<timeStampResult> results{};
		results.resize(mPoolCount);

		VkResult queryRes = vkGetQueryPoolResults(
			mDevice, mQueryPool[frameIndex], 0, mPoolCount,
			results.size() * sizeof(timeStampResult),
			results.data(),
			sizeof(timeStampResult),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
		);
		if (queryRes != VK_SUCCESS && queryRes != VK_NOT_READY)
			return result;

		for (auto& [k, v] : mUsedSlots[frameIndex])
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