#pragma once

#include <pch.h>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "submit.h"
#include "swapChain.h"
#include "descriptorSet.h"
#include "pipeline.h"
#include "image.h"
#include "registry.h"
#include "texture.h"
#include "commandBuffer.h"

namespace engine
{
	enum class handleType
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
		vulkImg,
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
			materialRegistry* matReg;
			commandBuffer* cmdBuf;
			vulkanImage* img;
		};
	};

	struct deletionQueue
	{
	public:
		deletionQueue() = default;
		deletionQueue(const deletionQueue&) = delete;

		void init(VkDevice device);
		void addDestroyTask(const destroyTask& task);
		error flushDeletonQueue();
	private:
		VkDevice mDevice;
		std::deque<destroyTask> mQueue;
	};
}