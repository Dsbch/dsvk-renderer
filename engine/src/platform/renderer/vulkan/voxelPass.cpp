#include <pch.h>

#include "voxelPass.h"

namespace engine
{
	error engine::voxelPass::init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanContext, std::shared_ptr<resourceManager> resourceManager)
	{
		mCtx = ctx;
		mResourceManager = resourceManager;
		mVulkanCtx = vulkanContext;

		auto meshlets = mCtx->mAmanager->getDefaultVoxelMeshShader();
		if (!meshlets)
			return meshlets.err();

		auto task = mCtx->mAmanager->getDefaultVoxelTaskShader();
		if (!task)
			return task.err();

		auto pixel = mCtx->mAmanager->getDefaultVoxelPixelShader();
		if (!pixel)
			return pixel.err();

		mVoxelizationPipeline.init(mVulkanCtx->device, graphicsPipeline::pipelineType::voxelization);

		error err = mVoxelizationPipeline.build(
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second },
			{},
			{},
			sampleCounts(mVulkanCtx->preset.msaa)
		);
		if (err)
			return err;

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::graphicsPipe, .graphicsPipe = &mVoxelizationPipeline });

		meshlets = mCtx->mAmanager->getDefaultVoxelRayMarchMeshShader();
		if (!meshlets)
			return meshlets.err();

		task = mCtx->mAmanager->getDefaultVoxelRayMarchTaskShader();
		if (!task)
			return task.err();

		pixel = mCtx->mAmanager->getDefaultVoxelRayMarchPixelShader();
		if (!pixel)
			return pixel.err();

		mVoxelRayMarchingPipeline.init(mVulkanCtx->device, graphicsPipeline::pipelineType::voxelization);

		err = mVoxelRayMarchingPipeline.build(
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second },
			mResourceManager->getDepthImage(false).img.format,
			{ mResourceManager->getColorAttachmentImage(false).img.format },
			sampleCounts(mVulkanCtx->preset.msaa)
		);
		if (err)
			return err;

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::graphicsPipe, .graphicsPipe = &mVoxelRayMarchingPipeline });

		return {};
	}
	error voxelPass::destroy()
	{
		return {};
	}

	error voxelPass::voxelizeScene(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		VkClearColorValue clearValue{ .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } };

		VkImageSubresourceRange range{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = VK_REMAINING_MIP_LEVELS,
			.baseArrayLayer = 0,
			.layerCount = 1,        // 3D images always have arrayLayers = 1
		};

		pipelineImageBarrier(
			cmd,
			mResourceManager->getClipMap().img.image,
			mResourceManager->getClipMap().img.format,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_CLEAR_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT
		);

		vkCmdClearColorImage(cmd, mResourceManager->getClipMap().img.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &range);

		pipelineImageBarrier(
			cmd,
			mResourceManager->getClipMap().img.image,
			mResourceManager->getClipMap().img.format,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_2_CLEAR_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT
		);

		uint32_t cmdBufferIndex = 0;
		for (auto& [_, v] : mResourceManager->getOpaqueCmdBuffers())
		{
			VkBuffer cmdBuf = v.getBuffer(frameIndex).getBuffer().buffer;
			uint32_t cmdBufSize = uint32_t(v.getBuffer(frameIndex).getLoadedBytes());
			uint32_t cmdBufferCount = uint32_t(cmdBufSize / sizeof(meshletShaderCMD));

			// Has to render.  
			if (cmdBufferCount > 0)
			{
				VkRenderingInfo renderInfo = renderingInfo(mResourceManager->getColorAttachmentImage(false).img.extent);

				vkCmdBeginRendering(cmd, &renderInfo);

				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mVoxelizationPipeline.getPipeline().first);

				pushConstants pc{
					.frameIndex = frameIndex,
					.cmdBufferCount = cmdBufferCount,
					.cmdOpaqueBufferIndex = cmdBufferIndex * mCtx->config.inner.graphics.framesInFlight + frameIndex,
				};

				vkCmdPushConstants(cmd, mVoxelizationPipeline.getPipeline().second, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

				mResourceManager->bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mVoxelizationPipeline.getPipeline().second);

				mVulkanCtx->vkCmdDrawMeshTasksEXT(
					cmd,
					cmdBufferCount / mCtx->config.inner.render.shaderWorkGroup + 1,
					1,
					1
				);

				vkCmdEndRendering(cmd);
			}

			cmdBufferIndex++;
		}

		return {};
	}

	error voxelPass::visualizeVoxelScene(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		pipelineImageBarrier(
			cmd,
			mResourceManager->getClipMap().img.image,
			mResourceManager->getClipMap().img.format,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_READ_BIT
		);

		VkClearValue clear{
			.color = VkClearColorValue{.float32 = { 0.0f, 0.0f, 0.0f, 0.0f} },
		};

		VkRenderingAttachmentInfo colorAttachment = attachmentInfo(
			mResourceManager->getColorAttachmentImage(false).img.view,
			mVulkanCtx->preset.msaa <= 1 ? nullptr : mResourceManager->getColorAttachmentImage(true).img.view,
			getResolveMode(mVulkanCtx->preset.msaa),
			&clear,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		std::vector<VkRenderingAttachmentInfo> colorAttachments = { colorAttachment };

		VkRenderingInfo renderInfo = renderingInfo(
			mResourceManager->getColorAttachmentImage(false).img.extent,
			colorAttachments,
			nullptr
		);

		vkCmdBeginRendering(cmd, &renderInfo);

		auto [pipeline, pipelineLayout] = mVoxelRayMarchingPipeline.getPipeline();

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		pushConstants pc{
			.frameIndex = frameIndex,
		};

		vkCmdPushConstants(cmd, mVoxelRayMarchingPipeline.getPipeline().second, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

		mResourceManager->bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout);

		mVulkanCtx->vkCmdDrawMeshTasksEXT(cmd, 1, 1, 1);

		vkCmdEndRendering(cmd);

		return {};
	}
}
