#pragma once

#include <pch.h>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "submit.h"
#include "swapChain.h"
#include "descriptorSet.h"
#include "pipeline.h"
#include "registry.h"
#include "texture.h"
#include "gpuProfiler.h"
#include "commandBuffer.h"

namespace engine
{
	enum handleType
	{
		allocator,
		iSub,
		sChain,
		descSet,
		computePipe,
		graphicsPipe,
		buffRegistry,
		sampler,
		vulkanBuf,
		gpuProf,
		matReg,
		cmdBuf,
	};

	struct destroyTask
	{
		handleType type;
		union
		{
			VmaAllocator allocator;
			submit* iSubmit;
			swapChain* sChain;
			descriptorSet* descSet;
			computePipeline* computePipe;
			graphicsPipeline* graphicsPipe;
			bufferRegistry* buffRegistry;
			VkSampler* sampler;
			vulkanBuffer* vulkanBuf;
			gpuProfiler* profiler;
			materialRegistry* matReg;
			commandBuffer* cmdBuf;
		};
	};

	struct deletionQueue
	{
	public:
		void init(VkDevice device);
		void addDestroyTask(const destroyTask& task);
		error flushDeletonQueue();
	private:
		VkDevice mDevice;
		std::deque<destroyTask> mQueue;
	};
}