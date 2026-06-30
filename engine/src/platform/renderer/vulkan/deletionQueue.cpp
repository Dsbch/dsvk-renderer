#include <pch.h>
#include "deletionQueue.h"

namespace engine
{
	void deletionQueue::init(VkDevice device)
	{
		mDevice = device;
	}

	void deletionQueue::addDestroyTask(const destroyTask& task)
	{
		mQueue.push_back(std::move(task));
	}

	error deletionQueue::flushDeletonQueue()
	{
		for (auto it = mQueue.rbegin(); it != mQueue.rend(); it++)
		{
			switch (it->type)
			{
			case allocator:
				vmaDestroyAllocator(it->allocator);
				break;
			case iSub:
				if (it->iSubmit)
					it->iSubmit->destroy();
				break;
			case sChain:
				if (it->sChain)
					it->sChain->destroy();
				break;
			case descSet:
				if (it->descSet)
					it->descSet->destroy();
				break;
			case computePipe:
				if (it->computePipe)
					it->computePipe->destroy();
				break;
			case graphicsPipe:
				if (it->graphicsPipe)
					it->graphicsPipe->destroy();
				break;
			case buffRegistry:
				if (it->buffRegistry)
					it->buffRegistry->destroy();
				break;
			case sampler:
				if (it->sampler)
					vkDestroySampler(mDevice, *it->sampler, nullptr);
				break;
			case vulkanBuf:
				if (it->vulkanBuf)
					it->vulkanBuf->destroy();
				break;
			case gpuProf:
				if (it->profiler)
					it->profiler->destroy();
				break;
			case matReg:
				if (it->matReg)
					it->matReg->destroy();
				break;
			case pipeData:
				if (it->pipeData)
					it->pipeData->destroy();
				break;
			default:
				LOGERROR("deletionQueue.cpp unkown deletionType");
			}
		}

		mQueue.clear();

		return {};
	}
}
