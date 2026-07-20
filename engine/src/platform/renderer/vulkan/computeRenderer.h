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
		uint32_t perMeshBufferBinding;
		uint32_t meshletBufferBinding;
		uint32_t perInstanceBufferBinding;
		uint32_t perDrawDataBufferBinding;
		uint32_t visabilityBuffer;
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
			VkBuffer UBObuffer,
			VkBuffer visabilityBuffer,
			const swapChain& sChain
		);
		error destroy();

		error buildHZB(VkCommandBuffer cmd, renderer::renderParams in, const swapChain& sChain);

		struct cullMeshletsParams
		{
			uint32_t cmdBufferCount;
			uint32_t cullStage;
			uint32_t opaqueCmdBufferIndex;
			uint32_t hzbLength;
		};

		error cullMeshlets(VkCommandBuffer cmd, renderer::renderParams in, cullMeshletsParams params);

		struct compactCommandBufferParams
		{
			uint32_t cmdBufferCount;
			uint32_t opaqueCmdBufferIndex;
			uint32_t stage;
			uint32_t compactRule;
		};

		error compactCommandBuffer(VkCommandBuffer cmd, renderer::renderParams in, compactCommandBufferParams params);

		error updateSwapchainDependentDescriptors(const swapChain& sChain);
		error updateOpaqueCmdBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
		error updateAccumilationCmdBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
		error updatePerMeshBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
		error updateMeshletBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
		error updatePerInstancetBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
		error updateVisabilityBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
	private:
		std::shared_ptr<context> mCtx;
		deletionQueue mDeletionQueue;
		computeBindings mBindings;
		graphicsPreset mPreset;

		computePipeline mBuildHzbPipeline;
		computePipeline mCullingPipeline;
		computePipeline mCompactCommandsPipeline;

		VkSampler mSampler;
		descriptorSet mDescriptorSet;

		error initDescriptors(
			VkDevice device,
			VkPhysicalDevice physicalDevice,
			VkBuffer UBObuffer,
			VkBuffer visabilityBuffer,
			deviceLimits limits
		);
		error initComputePipeline(VkDevice device);
	};
}