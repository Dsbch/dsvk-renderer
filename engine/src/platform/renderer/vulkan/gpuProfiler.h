#pragma once
#include <pch.h>

#include <vma/vk_mem_alloc.h>
#include "platform/renderer/renderer.h"
#include "helper.h"

namespace engine
{
	struct gpuProfiler
	{
	public:
		void init(VkDevice device, deviceLimits limits);
		void destroy();
		error createProfiling();

		error beginTimeStamp(VkCommandBuffer cmd, const std::string& slotName);
		void endTimestamp(VkCommandBuffer cmd, const std::string& slotName);
		void reset(VkCommandBuffer cmd);

		std::map<std::string, float> getAllSlots();
	private:
		VkDevice mDevice;
		VkQueryPool mQueryPool;
		uint32_t mPoolCount;
		uint32_t mCurrentSlot;
		deviceLimits mDeviceLimits;

		std::map<std::string, std::pair<uint32_t, uint32_t>> mUsedSlots;
	};
}