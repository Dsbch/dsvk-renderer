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

		uint32_t accumBinding;
		uint32_t revealBinding;
		uint32_t vertexBinding;
		uint32_t animVertexBinding;
		uint32_t perInstanceBinding;
		uint32_t meshletCmdBinding;
		uint32_t indexBinding;
		uint32_t primitiveBinding;
		uint32_t meshletBinding;
		uint32_t jointsBinding;
		uint32_t perMeshBinding;

		uint32_t perDrawBufferUboBinding;

		uint32_t materialArrayBinding;
	};

	struct meshletRenderer
	{
	public:
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
		error drawOpaqueGeometry(VkCommandBuffer cmd, renderer::renderCallIn in);
		error drawTransperentGeometry(VkCommandBuffer cmd, renderer::renderCallIn in);
		error compositeOpaqueAndTransperent(VkCommandBuffer cmd, renderer::renderCallIn in);
		error updateSwapchainDependentDescriptors(const swapChain& sChain);
	private:
		std::shared_ptr<context> mCtx;
		deletionQueue mDeletionQueue;
		meshletBindings mBindings;
		graphicsPreset mPreset;
		
		PFN_vkCmdDrawMeshTasksEXT mVkCmdDrawMeshTasksEXT;

		VkSampler mSampler;
		descriptorSet mDescriptorSet;
		bufferRegistry mVertexRegistry;
		bufferRegistry mAnimVertexRegistry;
		bufferRegistry mIndexRegistry;
		bufferRegistry mPrimitiveRegistry;
		bufferRegistry mMeshletRegistry;
		bufferRegistry mPerInstanceRegistry;
		bufferRegistry mJointRegistry;
		bufferRegistry mPerMeshRegistry;
		pipelineRegistry mPipelineRegistry;
		materialRegistry mMaterialRegistry;

		error initRegistry(VkDevice device, VmaAllocator allocator, submit& is);
		error initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, deviceLimits limits, VkBuffer UBObuffer);
		error initBlendingPipelines(VkDevice device, const swapChain& sChain);
	};
}