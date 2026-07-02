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
			const swapChain& sChain
		);
		error destroy();

		error buildHZB(VkCommandBuffer cmd, renderer::renderCallIn in, const swapChain& sChain);

		struct cullMeshletsParams
		{
			uint32_t meshletCount;
			uint32_t cullStage;
			uint32_t opaqueCmdBufferIndex;
			uint32_t hzbLength;
		};

		error cullMeshlets(VkCommandBuffer cmd, renderer::renderCallIn in, cullMeshletsParams params);

		error updateSwapchainDependentDescriptors(const swapChain& sChain);
		error updateOpaqueCmdBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
		error updateAccumilationCmdBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
		error updatePerMeshBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
		error updateMeshletBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
		error updatePerInstancetBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info);
	private:
		std::shared_ptr<context> mCtx;
		deletionQueue mDeletionQueue;
		computeBindings mBindings;
		graphicsPreset mPreset;

		computePipeline mBuildHzbPipeline;
		computePipeline mCullingPipeline;

		VkSampler mSampler;
		descriptorSet mDescriptorSet;

		error initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer UBObuffer, deviceLimits limits);
		error initComputePipeline(VkDevice device);
	};
}