#include <pch.h>
#include "computeRenderer.h"

namespace engine
{
	error computeRenderer::init(
		std::shared_ptr<context> ctx,
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		deviceLimits limits,
		VkImageView colorAttachmentView
	)
	{
		mCtx = ctx;
		mDeletionQueue.init(device);

		mBinding = computeBindings{
			.descriptorSet = 0,
			.totalDescriptorsCount = 1,
			.colorAttachment = 0,
		};

		error err = initDescriptors(device, physicalDevice, limits, colorAttachmentView);
		if (err)
			return err;

		err = initPipeline(device);
		if (err)
			return err;

		return {};
	}

	error computeRenderer::destroy()
	{
		mDeletionQueue.flushDeletonQueue();
		
		return {};
	}

	void computeRenderer::updateDescriptors(VkImageView colorAttachmentView)
	{
		std::vector<VkDescriptorImageInfo> colorAttachmentInfo = {
			{.sampler = VK_NULL_HANDLE, .imageView = colorAttachmentView, .imageLayout = VK_IMAGE_LAYOUT_GENERAL,}
		};

		auto writeInfo = descriptorSet::getWriteInfo(mBinding.colorAttachment, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, colorAttachmentInfo);
		mDescriptorSet.updateWrite(writeInfo);
	}

	error computeRenderer::initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, deviceLimits limits, VkImageView colorAttachmentView)
	{
		error err = mDescriptorSet.init(
			device,
			physicalDevice,
			poolConstraints{
				.maxImageDescriptors = limits.maxImage,
				.maxCombinedImageDescriptors = limits.maxCombinedImageSamplers,
				.maxBuffersDescriptors = limits.maxStorageBuffers,
				.maxUniformBuffersDescriptors = limits.maxUniformBuffers,
			}
			);
		if (err)
			return err;

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(mBinding.colorAttachment, uint32_t(1), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		);

		err = mDescriptorSet.build(VK_SHADER_STAGE_COMPUTE_BIT, mBinding.totalDescriptorsCount);
		if (err)
			return err;

		updateDescriptors(colorAttachmentView);

		mDeletionQueue.addDestroyTask(destroyTask{ .type = descSet, .descSet = &mDescriptorSet });

		return {};
	}

	error computeRenderer::initPipeline(VkDevice device)
	{
		VkShaderModule computeDrawShader;
		auto shader = mCtx->mAmanager->getDefaultComputeShader();
		if (!shader)
		{
			return shader.err();
		}

		computeDrawShader = static_cast<vulkanShader*>(shader.value().get())->mShaderModule;

		mPipeline.init(device);
		mPipeline.setShader(computeDrawShader);

		auto buildErr = mPipeline.build(VK_NULL_HANDLE, { mDescriptorSet.getDescriptorSet().second });
		if (buildErr)
			return buildErr;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = computePipe, .computePipe = &mPipeline });

		return {};
	}
	void computeRenderer::clear(VkCommandBuffer cmd, VkExtent3D colorAttachmentExtent)
	{
		// bind the compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline.getPipeline().first);

		// bind the descriptor set containing the draw image for the compute pipeline
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline.getPipeline().second, mBinding.descriptorSet, 1, &set, 0, nullptr);

		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, uint32_t(std::ceil(double(colorAttachmentExtent.width) / 32.0)), uint32_t(std::ceil(double(colorAttachmentExtent.height) / 32.0)), 1);
	}
}
