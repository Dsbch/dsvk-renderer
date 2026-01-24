#pragma once

#include <pch.h>
#include "descriptorSet.h"
#include "registry.h"
#include "submit.h"
#include "deletionQueue.h"
#include "helper.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	struct meshletBindings
	{
		uint32_t descriptorSet;
		uint32_t totalDescriptorsCount;

		uint32_t vertexBinding;
		uint32_t perInstanceBinding;
		uint32_t meshletCmdBinding;
		uint32_t indexBinding;
		uint32_t primitiveBinding;
		uint32_t meshletBinding;

		uint32_t perDrawBufferUboBinding;

		uint32_t materialArrayBinding;
	};

	struct meshletRenderer
	{
	public:
		error init(std::shared_ptr<context> ctx, PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT, VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator allocator, submit& is, deviceLimits limits, VkBuffer UBObuffer);
		error destroy();

		error addToRender(VkDevice device, submit& is, VkFormat depthFormat, VkFormat drawFormat, const model& m);
		void removeFromRender(const model& m);
		
		error updateDescriptors(renderer::renderCallIn in, submit& is);
		error geometryPass(VkCommandBuffer cmd, renderer::renderCallIn in);
	private:
		std::shared_ptr<context> mCtx;
		deletionQueue mDeletionQueue;
		meshletBindings mBindings;
		
		PFN_vkCmdDrawMeshTasksEXT mVkCmdDrawMeshTasksEXT;

		VkSampler mSampler;
		descriptorSet mDescriptorSet;
		bufferRegistry mVertexRegistry;
		bufferRegistry mIndexRegistry;
		bufferRegistry mPrimitiveRegistry;
		bufferRegistry mMeshletRegistry;
		bufferRegistry mPerInstanceRegistry;
		pipelineRegistry mPipelineRegistry;
		materialRegistry mMaterialRegistry;

		error initRegistry(VkDevice device, VmaAllocator allocator, submit& is, deviceLimits limits);
		error initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, deviceLimits limits, VkBuffer UBObuffer);
	};
}