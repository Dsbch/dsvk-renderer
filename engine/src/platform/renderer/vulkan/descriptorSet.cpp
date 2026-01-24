#include <pch.h>

#include "helper.h"
#include "descriptorSet.h"

namespace engine
{
	void descriptorPool::init(VkDevice device, poolConstraints constraints)
	{
		mCurrentPool = VK_NULL_HANDLE;

		mDevice = device;

		mConstraints = constraints;

		if (constraints.maxBuffersDescriptors != 0)
			mPoolSizes.push_back(
				{
					.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.descriptorCount = mConstraints.maxBuffersDescriptors
				}
			);

		if (constraints.maxImageDescriptors != 0)
			mPoolSizes.push_back(
				{
					.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					.descriptorCount = mConstraints.maxImageDescriptors
				}
			);


		if (constraints.maxCombinedImageDescriptors != 0)
			mPoolSizes.push_back(
				{
					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = mConstraints.maxCombinedImageDescriptors
				}
			);


		if (constraints.maxUniformBuffersDescriptors != 0)
			mPoolSizes.push_back(
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = mConstraints.maxUniformBuffersDescriptors
				}
			);
	}

	void descriptorPool::destroy()
	{
		for (auto& p : mPoolsInUse)
			vkDestroyDescriptorPool(mDevice, p, nullptr);

		vkDestroyDescriptorPool(mDevice, mCurrentPool, nullptr);
	}

	error descriptorPool::createPool()
	{
		if (mCurrentPool != VK_NULL_HANDLE)
		{
			mPoolsInUse.push_back(mCurrentPool);

			mCurrentPool = VK_NULL_HANDLE;
		}

		VkDescriptorPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
		poolInfo.maxSets = mConstraints.getMaxSetsPerPool();
		poolInfo.poolSizeCount = (uint32_t)mPoolSizes.size();
		poolInfo.pPoolSizes = mPoolSizes.data();

		auto result = vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mCurrentPool);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		return {};
	}

	withError<VkDescriptorSet> descriptorPool::allocate(VkDescriptorSetLayout layout)
	{
		if (mCurrentPool == VK_NULL_HANDLE)
		{
			error err = createPool();
			if (err)
				return err;
		}

		VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = mCurrentPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet ds;
		auto result = vkAllocateDescriptorSets(mDevice, &allocInfo, &ds);
		if (result != VK_SUCCESS && result != VK_ERROR_OUT_OF_POOL_MEMORY && result != VK_ERROR_FRAGMENTED_POOL)
			return { vkResultToStr(result) };

		if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
		{
			error err = createPool();
			if (err)
				return err;

			allocInfo.descriptorPool = mCurrentPool;

			auto result = vkAllocateDescriptorSets(mDevice, &allocInfo, &ds);
			if (result != VK_SUCCESS)
				return { vkResultToStr(result) };
		}

		return ds;
	}

	VkDescriptorPool descriptorPool::getCurrentPool()
	{
		return mCurrentPool;
	}

	error descriptorSet::init(VkDevice device, VkPhysicalDevice physicalDevice, poolConstraints constraints)
	{
		mDevice = device;
		mPool.init(mDevice, constraints);

		return {};
	}

	void descriptorSet::destroy()
	{
		clearBindings();
		vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
		mPool.destroy();
	}

	void descriptorSet::clearBindings()
	{
		mBindings.clear();
	}

	void descriptorSet::addBinding(VkDescriptorSetLayoutBinding binding)
	{
		mBindings.push_back(binding);
	}

	error descriptorSet::build(VkShaderStageFlags shaderStages, uint32_t totalDescriptorsCount)
	{
		if (mBindings.size() != totalDescriptorsCount)
			return error{ "calls to addBinding < than totalDescriptorsCount" };

		const VkDescriptorBindingFlagsEXT pFlags =
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

		std::vector<VkDescriptorBindingFlagsEXT> pFlagsV{};

		for (uint32_t i = 0; i < totalDescriptorsCount; i++)
		{
			pFlagsV.push_back(pFlags);
		}

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags{};
		binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		binding_flags.bindingCount = totalDescriptorsCount;
		binding_flags.pBindingFlags = pFlagsV.data();

		auto buildLayoutRes = buildLayout(shaderStages, &binding_flags, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT);
		if (!buildLayoutRes)
			return buildLayoutRes.err();

		mDescriptorSetLayout = buildLayoutRes.value();

		auto allocRes = mPool.allocate(mDescriptorSetLayout);
		if (!allocRes)
			return allocRes.err();

		mDescriptorSet = allocRes.value();

		return {};
	}

	void descriptorSet::updateWrite(std::vector<VkWriteDescriptorSet>& writeInfo)
	{
		for (auto& s : writeInfo)
		{
			s.dstSet = mDescriptorSet;
		}

		vkUpdateDescriptorSets(mDevice, uint32_t(writeInfo.size()), writeInfo.data(), 0, nullptr);
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

	std::vector<VkWriteDescriptorSet> descriptorSet::getWriteInfo(uint32_t dstBinding, VkDescriptorType descriptorType, std::vector<VkDescriptorImageInfo>& imgInfo)
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

	std::vector<VkWriteDescriptorSet> descriptorSet::getWriteInfo(uint32_t dstBinding, std::vector<VkDescriptorBufferInfo>& bufferInfo, bool isUBO)
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
			write.descriptorType = isUBO ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.pImageInfo = nullptr;
			write.pBufferInfo = &bufferInfo[i];
			write.pTexelBufferView = nullptr;

			result.push_back(write);
		}

		return result;
	}

	withError<VkSampler> descriptorSet::createSampler(VkDevice device, float maxFiltering, VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV, VkSamplerAddressMode addressModeW, VkBool32 anisotropyEnable, VkBorderColor borderColor, VkBool32 unnormalizedCoordinates)
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

	withError<VkDescriptorSetLayout> descriptorSet::buildLayout(VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
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

	uint32_t poolConstraints::getMaxSetsPerPool()
	{
		return maxBuffersDescriptors + maxUniformBuffersDescriptors + maxImageDescriptors + maxCombinedImageDescriptors;
	}
}