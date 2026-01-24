#pragma once

#include <pch.h>
#include "pipeline.h"
#include "descriptorSet.h"
#include "buffer.h"
#include "platform/renderer/vertex.h"
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
			VkBuffer UBObuffer,
			VkFormat depthFormat, VkFormat drawFormat
		);
		error destroy();
		error addLine(glm::vec3 p1, glm::vec3 p2);
		error updateDescriptors(VmaAllocator allocator, submit& is);
		error drawLines(VkCommandBuffer cmd);
	private:
		std::shared_ptr<context> mCtx;

		deletionQueue mDeletionQueue;

		classicGraphicPipeline mPipeline;
		
		lineBindings mBindings;
		descriptorSet mDescriptorSet;
		vulkanBuffer mVertexBuffer;
		std::vector<lineVertex> mVertexData;
		bool mNeedDescrotprUpdate;

		error initPipeline(VkDevice device, VkFormat depthFormat, VkFormat drawFormat);
		error initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer UBObuffer);
	};
}