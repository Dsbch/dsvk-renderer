#include <pch.h>
#include "descriptorSet.h"

namespace vktest
{
	std::once_flag descriptorSet::isPoolCreated;
	descriptorPool descriptorSet::pool;

	engine::error descriptorPool::initPool(VkDevice device)
	{
		const uint32_t maxDescriptorSets = 100;
		const uint32_t maxDescriptors = 100;

		mDevice = device;

		// TODO: rewrite? or just leave here.
		std::vector<VkDescriptorPoolSize> poolSizes = {
					{
						.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
						.descriptorCount = maxDescriptors
					}
		};

		VkDescriptorPoolCreateInfo pool_info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		pool_info.flags = 0;
		pool_info.maxSets = maxDescriptorSets;
		pool_info.poolSizeCount = (uint32_t)poolSizes.size();
		pool_info.pPoolSizes = poolSizes.data();

		auto result = vkCreateDescriptorPool(mDevice, &pool_info, nullptr, &mPool);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		return {};
	}

	descriptorPool::descriptorPool()
		:
		mPool(VK_NULL_HANDLE),
		mDevice(VK_NULL_HANDLE)
	{
	}

	void descriptorPool::destory()
	{
		vkDestroyDescriptorPool(mDevice, mPool, nullptr);
	}

	descriptorSet::descriptorSet()
		:
		mDevice(VK_NULL_HANDLE),
		mDescriptorSetLayout(VK_NULL_HANDLE),
		mDescriptorSet(VK_NULL_HANDLE)
	{
	}

	void descriptorSet::init(VkDevice device)
	{
		mDevice = device;

		if (mDevice)
		{
			std::call_once(
				isPoolCreated,
				[&]()->void
				{
					mErr = pool.initPool(mDevice);
				}
			);
		}
	}

	void descriptorSet::destroy()
	{
		vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
	}

	void descriptorSet::destroyPool()
	{
		pool.destory();
	}

	engine::error descriptorSet::checkError()
	{
		return mErr;
	}

	void descriptorSet::clearBindings()
	{
		mBindings.clear();
		mSource.clear();
	}

	void descriptorSet::addBinding(VkDescriptorSetLayoutBinding binding, VkWriteDescriptorSet source)
	{
		mBindings.push_back(binding);
		mSource.push_back(source);
	}

	engine::error descriptorSet::build(VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
	{
		auto buildLayoutRes = buildLayout(shaderStages, pNext, flags);
		if (!buildLayoutRes)
			return buildLayoutRes.err();

		mDescriptorSetLayout = buildLayoutRes.value();

		auto allocRes = allocate();
		if (!allocRes)
			return allocRes.err();

		mDescriptorSet = allocRes.value();

		// set sources for each binding.
		for (auto& s : mSource)
		{
			s.dstSet = mDescriptorSet;

			vkUpdateDescriptorSets(mDevice, 1, &s, 0, nullptr);
		}

		return {};
	}

	std::pair<VkDescriptorSet, VkDescriptorSetLayout> descriptorSet::getDescriptorSet()
	{
		return { mDescriptorSet, mDescriptorSetLayout };
	}

	engine::withError<VkDescriptorSetLayout> descriptorSet::buildLayout(VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
	{
		for (auto& b : mBindings) {
			b.stageFlags |= shaderStages;
		}

		VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		info.pNext = pNext;
		info.pBindings = mBindings.data();
		info.bindingCount = (uint32_t)mBindings.size();
		info.flags = flags;

		VkDescriptorSetLayout set;
		if (auto result = vkCreateDescriptorSetLayout(mDevice, &info, nullptr, &set); result != VK_SUCCESS)
			return { vkResultToStr(result) };

		return set;
	}

	engine::withError<VkDescriptorSet> descriptorSet::allocate()
	{
		VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = pool.mPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mDescriptorSetLayout;

		VkDescriptorSet ds;
		auto result = vkAllocateDescriptorSets(mDevice, &allocInfo, &ds);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		return ds;
	}
}