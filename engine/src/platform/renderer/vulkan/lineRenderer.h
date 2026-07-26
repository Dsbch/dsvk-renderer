#pragma once

#include <pch.h>
#include "pipeline.h"
#include "descriptorSet.h"
#include "buffer.h"
#include "platform/renderer/vertex.h"
#include "platform/renderer/renderer.h"
#include "deletionQueue.h"

namespace engine
{
	struct lineBindings
	{
		uint32_t descriptorSet;
		uint32_t totalDescriptorsCount;

		uint32_t vertexBinding;
		uint32_t perDrawDataBinding;
	};

	struct lineRenderer
	{
	public:
		error init(
			std::shared_ptr<context> ctx, 
			VkDevice device, 
			VkPhysicalDevice physicalDevice, 
			VmaAllocator allocator, 
			submit& is, 
			const std::vector<vulkanBuffer>& UBObuffer,
			VkFormat depthFormat, VkFormat drawFormat,
			graphicsPreset preset,
			deviceLimits limits
		);
		error destroy();
		error addLine(glm::vec3 p1, glm::vec3 p2);
		error updateDescriptors(VmaAllocator allocator, submit& is);
		error drawLines(VkCommandBuffer cmd, const swapChain& sChain, uint32_t frameIndex);
	private:
		std::shared_ptr<context> mCtx;
		graphicsPreset mPreset;

		deletionQueue mDeletionQueue;

		classicGraphicPipeline mPipeline;
		
		lineBindings mBindings;
		descriptorSet mDescriptorSet;
		vulkanBuffer mVertexBuffer;
		std::vector<lineVertex> mVertexData;
		bool mNeedDescrotprUpdate;

		error initPipeline(VkDevice device, VkFormat depthFormat, VkFormat drawFormat);
		error initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, const std::vector<vulkanBuffer>& UBObuffer, deviceLimits limits);
	};
}