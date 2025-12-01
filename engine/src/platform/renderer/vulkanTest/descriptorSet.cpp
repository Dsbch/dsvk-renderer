#include <pch.h>
#include "descriptorSet.h"

namespace vktest
{
	std::once_flag descriptorSet::isPoolCreated;
	descriptorPool descriptorSet::pool;
	float descriptorSet::maxFiltering;

	engine::error descriptorPool::initPool(VkDevice device)
	{
		const uint32_t maxDescriptorSets = 100;
		const uint32_t maxDescriptors = 100;
		const uint32_t maxTextureDescriptors = 1000;

		mDevice = device;

		// TODO: create new pool when we reach VK_ERROR_OUT_OF_POOL_MEMORY or VK_ERROR_FRAGMENTED_POOL.
		// see https://vkguide.dev/docs/new_chapter_4/descriptor_abstractions/
		std::vector<VkDescriptorPoolSize> poolSizes = {
					{
						.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
						.descriptorCount = maxDescriptors
					},
					{
						.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						.descriptorCount = maxDescriptors
					},
					{
						.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						.descriptorCount = maxTextureDescriptors
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

	engine::error descriptorSet::init(VkDevice device, VkPhysicalDevice physicalDevice)
	{
		mDevice = device;
		mDescriptorSetLayout = VK_NULL_HANDLE;
		mDescriptorSet = VK_NULL_HANDLE;

		engine::error err;
		if (mDevice)
		{
			std::call_once(
				isPoolCreated,
				[&]()->void
				{
					err = pool.initPool(mDevice);

					VkPhysicalDeviceProperties deviceProps;
					vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);

					maxFiltering = deviceProps.limits.maxSamplerAnisotropy;
				}
			);
		}

		return err;
	}

	void descriptorSet::destroy()
	{
		clearBindings();
		vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
	}

	void descriptorSet::destroyPool()
	{
		pool.destory();
	}

	void descriptorSet::clearBindings()
	{
		mBindings.clear();
		mWrite.clear();
	}

	void descriptorSet::addBinding(VkDescriptorSetLayoutBinding binding)
	{
		mBindings.push_back(binding);
	}

	void descriptorSet::addWrite(const std::vector<VkWriteDescriptorSet>& source)
	{
		mWrite.push_back({ source });
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
		for (auto& s : mWrite)
		{
			for (auto& e : s)
				e.dstSet = mDescriptorSet;

			vkUpdateDescriptorSets(mDevice, uint32_t(s.size()), s.data(), 0, nullptr);
		}

		return {};
	}

	std::pair<VkDescriptorSet, VkDescriptorSetLayout> descriptorSet::getDescriptorSet()
	{
		return { mDescriptorSet, mDescriptorSetLayout };
	}

	VkDescriptorSetLayoutBinding descriptorSet::getLayoutBindingInfo(uint32_t binding, uint32_t descriptorCount, VkDescriptorType type)
	{
		VkDescriptorSetLayoutBinding layout{};
		layout.binding = binding;
		layout.descriptorCount = descriptorCount;
		layout.descriptorType = type;

		return layout;
	}

	std::vector<VkWriteDescriptorSet> descriptorSet::getWriteInfo(uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorImageInfo>& imgInfo)
	{
		std::vector<VkWriteDescriptorSet> result{};
		result.reserve(imgInfo.size());

		for (int i = 0; i < imgInfo.size(); i++)
		{
			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.pNext = nullptr;
			write.dstBinding = dstBinding;

			// will be set by descriptorSet class.
			write.dstSet = nullptr;

			write.descriptorCount = 1;
			write.dstArrayElement = i;
			write.descriptorType = descriptorType;
			write.pImageInfo = &imgInfo[i];

			result.push_back(write);
		}

		return result;
	}

	std::vector<VkWriteDescriptorSet> descriptorSet::getWriteInfo(uint32_t dstBinding, const std::vector <VkDescriptorBufferInfo>& bufferInfo)
	{
		std::vector<VkWriteDescriptorSet> result;
		result.reserve(bufferInfo.size());

		for (int i = 0; i < bufferInfo.size(); i++)
		{
			VkWriteDescriptorSet write = {};

			// will be set later.
			write.dstSet = VK_NULL_HANDLE;

			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.pNext = nullptr;
			write.dstBinding = dstBinding;
			write.dstArrayElement = i;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.pImageInfo = nullptr;
			write.pBufferInfo = &bufferInfo[i];
			write.pTexelBufferView = nullptr;

			result.push_back(write);
		}

		return result;
	}

	engine::withError<VkSampler> descriptorSet::createSampler(VkDevice device, VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW, VkBool32 anisotropyEnable, VkBorderColor borderColor, VkBool32 unnormalizedCoordinates)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = magFilter;
		samplerInfo.minFilter = minFilter;
		samplerInfo.mipmapMode = mipmapMode;
		samplerInfo.addressModeU = addressModeU;
		samplerInfo.addressModeV = addressModeV;
		samplerInfo.addressModeW = addressModeW;

		samplerInfo.minLod = 0.0f; // Optional
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.mipLodBias = 0.0f; // Optional

		samplerInfo.anisotropyEnable = anisotropyEnable;
		samplerInfo.maxAnisotropy = maxFiltering;
		samplerInfo.borderColor = borderColor;
		samplerInfo.unnormalizedCoordinates = unnormalizedCoordinates;

		VkSampler sampler;
		VkResult res = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
		if (res != VK_SUCCESS)
			return { vkResultToStr(res) };

		return sampler;
	}

	engine::withError<VkDescriptorSetLayout> descriptorSet::buildLayout(VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
	{
		for (auto& b : mBindings)
			b.stageFlags |= shaderStages;

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