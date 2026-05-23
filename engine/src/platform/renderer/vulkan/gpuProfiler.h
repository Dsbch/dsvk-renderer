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
		void init(VkDevice device, uint32_t slotCount, deviceLimits limits);
		void destroy();
		error createProfiling();

		error beginTimeStamp(VkCommandBuffer cmd);
		void endTimestamp(VkCommandBuffer cmd);
		void reset(VkCommandBuffer cmd);

		std::vector<float> getAllSlots();
	private:
		VkDevice mDevice;
		VkQueryPool mQueryPool;
		uint32_t mPoolCount;
		deviceLimits mDeviceLimits;

		uint32_t mCurrentSlot;
	};
}