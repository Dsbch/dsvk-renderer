#pragma once

#include <pch.h>

#include "VkBootstrap.h"
#include "helper.h"

namespace engine
{
	struct descriptorPool
	{
		VkDescriptorPool mPool;
		VkDevice mDevice;

		engine::error initPool(VkDevice device);

		descriptorPool();
		void destroy();
	};

	class descriptorSet
	{
	public:
		descriptorSet();

		engine::error init(VkDevice device, VkPhysicalDevice physicalDevice);
		void destroy();
		
		void clearBindings();
		void clearWrites();
		void addBinding(VkDescriptorSetLayoutBinding binding);
		void addWrite(const std::vector<VkWriteDescriptorSet>& source);
		engine::error build(VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
		void updateWrite();
		
		std::pair<VkDescriptorSet, VkDescriptorSetLayout> getDescriptorSet();


		static VkDescriptorSetLayoutBinding getLayoutBindingInfo(uint32_t binding, uint32_t descriptorCount, VkDescriptorType type);
		static std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t dstBinding, VkDescriptorType descriptorType, const std::vector<VkDescriptorImageInfo>& imgInfo);
		static std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t dstBinding, const std::vector <VkDescriptorBufferInfo>& bufferInfo);
		
		static engine::withError<VkSampler> createSampler(
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
		
		engine::withError<VkDescriptorSetLayout> buildLayout(VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
		engine::withError<VkDescriptorSet> allocate();
	};
}