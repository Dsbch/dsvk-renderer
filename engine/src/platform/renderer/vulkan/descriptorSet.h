#pragma once

#include <pch.h>

#include "VkBootstrap.h"
#include "vkHelper.h"

namespace vktest
{
	struct descriptorPool
	{
		VkDescriptorPool mPool;
		VkDevice mDevice;

		engine::error initPool(VkDevice device);

		descriptorPool();
		void destory();
	};

	class descriptorSet
	{
	public:
		descriptorSet();

		void init(VkDevice device);
		void destroy();
		engine::error checkError();
		
		void clearBindings();
		void addBinding(VkDescriptorSetLayoutBinding binding, VkWriteDescriptorSet source);
		engine::error build(VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
		
		std::pair<VkDescriptorSet, VkDescriptorSetLayout> getDescriptorSet();
		
		static void destroyPool();
	private:
		static descriptorPool pool;
		static std::once_flag isPoolCreated;

		engine::error mErr;
		
		VkDevice mDevice;
		VkDescriptorSet mDescriptorSet;
		VkDescriptorSetLayout mDescriptorSetLayout;

		std::vector<VkDescriptorSetLayoutBinding> mBindings;
		std::vector<VkWriteDescriptorSet> mSource;
		
		engine::withError<VkDescriptorSetLayout> buildLayout(VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
		engine::withError<VkDescriptorSet> allocate();
	};
}