#pragma once

#include <pch.h>
#include "descriptorSet.h"
#include "registry.h"
#include "submit.h"
#include "deletionQueue.h"
#include "helper.h"
#include "swapChain.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	struct meshletBindings
	{
		uint32_t descriptorSet;
		uint32_t totalDescriptorsCount;

		// Vertex attributes.
		uint32_t positionsBinding;
		uint32_t normalBinding;
		uint32_t tangentBinding;
		uint32_t jointIndexBinding;
		uint32_t wightBinding;

		// Buffers binding.
		uint32_t perInstanceBinding;
		uint32_t cmdOpaqueBufferBinding;
		uint32_t cmdAccumilationBufferBinding;
		uint32_t indexBinding;
		uint32_t primitiveBinding;
		uint32_t meshletBinding;
		uint32_t jointsBinding;
		uint32_t perMeshBinding;
		uint32_t perDrawBufferUboBinding;

		// Materil binding.
		uint32_t materialArrayBinding;
		uint32_t accumBinding;
		uint32_t revealBinding;
		
		// Other bindings.
		uint32_t hzbChainBinding;
	};

	struct meshletRenderer
	{
	public:
		struct opaquePassParams
		{
			uint32_t hzbBufLength;
		};

		error init(
			std::shared_ptr<context> ctx, 
			PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT, 
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

		error addToRender(VkDevice device, submit& is, const swapChain& sChain, const model& m);
		error updateInstance(const model& m, submit& is);
		error updateAnimations(const model& m, submit& is);
		void removeFromRender(const model& m);
		
		error updateDescriptors(renderer::renderCallIn in, submit& is);
		error opaquePass(VkCommandBuffer cmd, renderer::renderCallIn in, opaquePassParams params);
		error accumilationPass(VkCommandBuffer cmd, renderer::renderCallIn in);
		error compositePass(VkCommandBuffer cmd, renderer::renderCallIn in);
		error updateSwapchainDependentDescriptors(const swapChain& sChain);
	private:
		std::shared_ptr<context> mCtx;
		deletionQueue mDeletionQueue;
		meshletBindings mBindings;
		graphicsPreset mPreset;
		
		PFN_vkCmdDrawMeshTasksEXT mVkCmdDrawMeshTasksEXT;

		VkSampler mSampler;
		descriptorSet mDescriptorSet;
		// Vertex attribtes.
		bufferRegistry mPositionRegistry;
		bufferRegistry mNormalRegistry;
		bufferRegistry mTangentRegistry;
		bufferRegistry mJointIndexRegistry;
		bufferRegistry mWeightRegistry;

		// Buffers.
		bufferRegistry mIndexRegistry;
		bufferRegistry mPrimitiveRegistry;
		bufferRegistry mMeshletRegistry;
		bufferRegistry mPerMeshRegistry;
		
		pipelineRegistry mPipelineRegistry;
		materialRegistry mMaterialRegistry;

		// Can be updated each frame, they live as MAPPED buffers.
		bufferRegistry mPerInstanceRegistry;
		bufferRegistry mJointRegistry;

		error initRegistry(VkDevice device, VmaAllocator allocator, submit& is);
		error initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, deviceLimits limits, VkBuffer UBObuffer);
		error initBlendingPipelines(VkDevice device, const swapChain& sChain);
	};
}