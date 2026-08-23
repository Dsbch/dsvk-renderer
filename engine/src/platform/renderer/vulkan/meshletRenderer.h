#pragma once

#include <pch.h>
#include "descriptorSet.h"
#include "registry.h"
#include "submit.h"
#include "deletionQueue.h"
#include "helper.h"
#include "swapChain.h"
#include "platform/renderer/renderer.h"
#include "computeRenderer.h"
#include "pipeline.h"
#include "commandBuffer.h"

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
		uint32_t visabilityBuffer;
		uint32_t perDrawBufferUboBinding;

		// Materil binding.
		uint32_t materialArrayBinding;
		uint32_t accumBinding;
		uint32_t revealBinding;

		// Voxel bindings.
		uint32_t clipMapBinding;
	};

	struct meshletRenderer
	{
	public:
		error init(
			std::shared_ptr<context> ctx,
			PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT,
			PFN_vkCmdDrawMeshTasksIndirectEXT vkCmdDrawMeshTasksIndirectEXT,
			VkDevice device,
			VkPhysicalDevice physicalDevice,
			VmaAllocator allocator,
			submit& is,
			deviceLimits limits,
			graphicsPreset preset,
			const std::vector<vulkanBuffer>& UBObuffer,
			const swapChain& sChain
		);
		error destroy();

		error addToRender(VkDevice device, VmaAllocator allocator, submit& is, const swapChain& sChain, const model& m, uint32_t frameIndex);
		error updateInstance(const model& m, submit& is, uint32_t frameIndex);
		error updateAnimations(const model& m, submit& is, uint32_t frameIndex);
		void removeFromRender(const model& m, uint32_t frameIndex);

		error updateDescriptors(renderer::renderParams in, submit& is, VkDevice device, VmaAllocator allocator, uint32_t frameIndex);
		error updateSwapchainDependentDescriptors(const swapChain& sChain);
		
		error opaquePass(VkCommandBuffer cmd, renderer::renderParams in, const swapChain& sChain, uint32_t frameIndex);
		error accumilationPass(VkCommandBuffer cmd, renderer::renderParams in, const swapChain& sChain, uint32_t frameIndex);
		error compositePass(VkCommandBuffer cmd, renderer::renderParams in, const swapChain& sChain, uint32_t frameIndex);
	
		voxelDrawParams getVoxelSceneParams() const;
		error voxilizeOpaqueGeometry(VkCommandBuffer cmd, renderer::renderParams in, const swapChain& sChain, uint32_t frameIndex);
	private:
		error initRegistry(VkDevice device, VmaAllocator allocator, submit& is);
		error initDescriptors(
			VkDevice device, 
			VkPhysicalDevice physicalDevice,
			deviceLimits limits, 
			const std::vector<vulkanBuffer>& UBObuffer
		);
		error initBlendingPipelines(VkDevice device, const swapChain& sChain, VmaAllocator allocator, submit& is);
		error initVoxelPipelines(VkDevice device, const swapChain& sChain, VmaAllocator allocator, submit& is);

		uint32_t getMaxCmdBufferSize(uint32_t frameIndex) const;
		aabb getSceneBoundingBox() const;

		std::shared_ptr<context> mCtx;
		deletionQueue mDeletionQueue;
		meshletBindings mBindings;
		graphicsPreset mPreset;

		PFN_vkCmdDrawMeshTasksEXT mVkCmdDrawMeshTasksEXT;
		PFN_vkCmdDrawMeshTasksIndirectEXT mVkCmdDrawMeshTasksIndirectEXT;

		VkSampler mSampler;
		descriptorSet mDescriptorSet;

		// Visability buffers for indirect calls.
		std::vector<vulkanBuffer> mVisabilityBuffer;
		
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

		// Can be updated each frame, they live as MAPPED buffers.
		bufferRegistry mPerInstanceRegistry;
		bufferRegistry mJointRegistry;
		
		// Material registry.
		materialRegistry mMaterialRegistry;

		// Voxel stuff.
		// N-frames buffered???????
		vulkanImage mClipMap;
		graphicsPipeline mVoxelizationPipeline;
		std::unordered_map<uint32_t, aabb> mSceneAABB;
		
		graphicsPipeline mCompositePipeline;
		graphicsPipeline mAccumilationPipeline;
		std::map<uint32_t, std::pair<graphicsPipeline, commandBuffer>> mOpaquePipelines;

		commandBuffer mAccumilationCommandBuffer;

		// Compute renderer to make HZB and for culling.
		computeRenderer mComputeRenderer;
	};
}