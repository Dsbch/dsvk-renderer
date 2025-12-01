#pragma once

#include <pch.h>

#include "VkBootstrap.h"
#include "helper.h"

namespace engine
{
	struct poolConstraints
	{
		// Max number of descriptors of type VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE per pool.
		uint32_t maxTextureDescriptors;
		// Max number of descriptors of type VK_DESCRIPTOR_TYPE_STORAGE_BUFFER per pool.
		uint32_t maxStorageDescriptors;

		// Combines all sets that used in pool.
		uint32_t getMaxSetsPerPool();
	};

	struct descriptorPool
	{
	public:

		void init(VkDevice device, poolConstraints constraints);
		void destroy();
		withError<VkDescriptorSet> allocate(VkDescriptorSetLayout layout);
	private:
		poolConstraints mConstraints;

		VkDevice mDevice;
		std::vector<VkDescriptorPoolSize> mPoolSizes;

		error createPool();
		VkDescriptorPool mCurrentPool;
		std::vector<VkDescriptorPool> mPoolsInUse;
	};

	struct descriptorSet
	{
	public:
		error init(VkDevice device, VkPhysicalDevice physicalDevice, poolConstraints constraints = { .maxTextureDescriptors = 1000, .maxStorageDescriptors = 1000 });
		void destroy();

		void clearBindings();
		void clearWrites();
		void addBinding(VkDescriptorSetLayoutBinding binding);
		void addWrite(const std::vector<VkWriteDescriptorSet>& source);
		error build(VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
		void updateWrite();

		std::pair<VkDescriptorSet, VkDescriptorSetLayout> getDescriptorSet();


		static VkDescriptorSetLayoutBinding getLayoutBindingInfo(uint32_t binding, uint32_t descriptorCount, VkDescriptorType type);
		static std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorImageInfo>& imgInfo);
		static std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t dstBinding, const std::vector <VkDescriptorBufferInfo>& bufferInfo);

		static withError<VkSampler> createSampler(
			VkDevice device,
			float maxFiltering,
			VkFilter magFilter = VK_FILTER_LINEAR,
			VkFilter minFilter = VK_FILTER_LINEAR,
			VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VkBool32 anisotropyEnable = VK_TRUE,
			VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			VkBool32 unnormalizedCoordinates = VK_FALSE
		);

		static void destroyPool();
	private:
		static descriptorPool pool;
		static std::once_flag isPoolCreated;;

		VkDevice mDevice;
		VkDescriptorSet mDescriptorSet;
		VkDescriptorSetLayout mDescriptorSetLayout;

		std::vector<VkDescriptorSetLayoutBinding> mBindings;
		std::vector<std::vector<VkWriteDescriptorSet>> mWrite;

		withError<VkDescriptorSetLayout> buildLayout(VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
	};
}