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
			case handleType::allocator:
				vmaDestroyAllocator(it->allocator);
				break;
			case handleType::iSub:
				if (it->iSubmit)
					it->iSubmit->destroy();
				break;
			case handleType::sChain:
				if (it->sChain)
					it->sChain->destroy();
				break;
			case handleType::descSet:
				if (it->descSet)
					it->descSet->destroy();
				break;
			case handleType::computePipe:
				if (it->computePipe)
					it->computePipe->destroy();
				break;
			case handleType::graphicsPipe:
				if (it->graphicsPipe)
					it->graphicsPipe->destroy();
				break;
			case handleType::buffRegistry:
				if (it->buffRegistry)
					it->buffRegistry->destroy();
				break;
			case handleType::sampler:
				if (it->sampler)
					vkDestroySampler(mDevice, *it->sampler, nullptr);
				break;
			case handleType::vulkanBuf:
				if (it->vulkanBuf)
					it->vulkanBuf->destroy();
				break;
			case handleType::vulkImg:
				if (it->img)
					it->img->destroy();
				break;
			case handleType::matReg:
				if (it->matReg)
					it->matReg->destroy();
				break;
			case handleType::cmdBuf:
				if (it->cmdBuf)
					it->cmdBuf->destroy();
				break;
			default:
				LOGERROR("deletionQueue.cpp unkown deletionType");
			}
		}

		mQueue.clear();

		return {};
	}
}
