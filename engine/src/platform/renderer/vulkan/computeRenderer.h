#pragma once

#include <pch.h>
#include "deletionQueue.h"
#include "helper.h"

namespace engine
{
	struct computeBindings
	{
		uint32_t descriptorSet;
		uint32_t totalDescriptorsCount;
		uint32_t colorAttachment;
	};

	struct computeRenderer
	{
	public:
		error init(std::shared_ptr<context> ctx, VkDevice device, VkPhysicalDevice physicalDevice, deviceLimits limits, VkImageView colorAttachmentView);
		error destroy();
		void clear(VkCommandBuffer cmd, VkExtent3D colorAttachmentExtent);
		void updateDescriptors(VkImageView colorAttachmentView);
	private:
		std::shared_ptr<context> mCtx;
		descriptorSet mDescriptorSet;
		computeBindings mBinding;
		computePipeline mPipeline;
		deletionQueue mDeletionQueue;

		error initPipeline(VkDevice device);
		error initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, deviceLimits limits, VkImageView colorAttachmentView);
	};
}