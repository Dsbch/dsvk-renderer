#pragma once

#include <pch.h>

#include "base/include.h"

namespace engine
{
	struct gpuProfiler
	{
	public:
		gpuProfiler() = default;
		gpuProfiler(const gpuProfiler&) = delete;

		void init(VkDevice device, deviceLimits limits, uint32_t framesInFlight);
		void destroy();
		error createProfiling(submit& is);

		error beginTimeStamp(VkCommandBuffer cmd, const std::string& slotName, uint32_t frameIndex);
		void endTimestamp(VkCommandBuffer cmd, const std::string& slotName, uint32_t frameIndex);
		void reset(VkCommandBuffer cmd, uint32_t frameIndex);

		std::map<std::string, float> getAllSlots(uint32_t frameIndex);
	private:
		VkDevice mDevice;
		std::vector<VkQueryPool> mQueryPool;
		uint32_t mPoolCount;
		uint32_t mCurrentSlot;
		deviceLimits mDeviceLimits;

		std::vector<std::map<std::string, std::pair<uint32_t, uint32_t>>> mUsedSlots;
	};
}