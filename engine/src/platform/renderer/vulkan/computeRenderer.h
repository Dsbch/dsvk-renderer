#pragma once

#include <pch.h>

#include "descriptorSet.h"
#include "submit.h"
#include "deletionQueue.h"
#include "pipeline.h"
#include "helper.h"
#include "swapChain.h"
#include "registry.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	struct computeBindings
	{
		uint32_t descriptorSet;
		uint32_t totalDescriptorsCount;

		// HZB bindings.
		uint32_t orignalZBufferBinding;
		uint32_t hzbBinding;
		// Buffers binding.
		uint32_t cmdOpaqueBufferBinding;
		uint32_t cmdAccumilationBufferBinding;
	};

	struct computeRenderer
	{
	public:
		error init(
			std::shared_ptr<context> ctx,
			VkDevice device,
			VkPhysicalDevice physicalDevice,
			VmaAllocator allocator,
			submit& is,
			deviceLimits limits,
			graphicsPreset preset,
			const swapChain& sChain
		);
		error destroy();

		error buildHZB(VkCommandBuffer cmd, renderer::renderCallIn in, const swapChain& sChain);

		error updateSwapchainDependentDescriptors(const swapChain& sChain);
		error updateOpaqueCmdBufferDescriptors(std::vector<VkDescriptorBufferInfo>& cmdOpaqueInfo);
		error updateAccumilationCmdBufferDescriptors(std::vector<VkDescriptorBufferInfo>& cmdAccumilationInfo);
	private:
		std::shared_ptr<context> mCtx;
		deletionQueue mDeletionQueue;
		computeBindings mBindings;
		graphicsPreset mPreset;

		computePipeline mComputePipeline;

		VkSampler mSampler;
		descriptorSet mDescriptorSet;

		error initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, deviceLimits limits);
		error initComputePipeline(VkDevice device);
	};
}