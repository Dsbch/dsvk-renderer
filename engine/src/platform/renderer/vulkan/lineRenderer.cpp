#include <pch.h>
#include "lineRenderer.h"
#include "shader.h"

namespace engine
{
	error lineRenderer::init(
		std::shared_ptr<context> ctx,
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VmaAllocator allocator,
		submit& is,
		graphicsPreset preset,
		deviceLimits limits,
		std::shared_ptr<resourceManager> resourceManager
	)
	{
		mCtx = ctx;

		mResourceManager = resourceManager;

		mPreset = preset;

		mDeletionQueue.init(device);

		error err = initPipeline(
			device, 
			resourceManager->getDepthImage(false).img.format,
			resourceManager->getColorAttachmentImage(false).img.format
		);
		if (err)
			return err;

		return {};
	}

	error lineRenderer::destroy()
	{
		mDeletionQueue.flushDeletonQueue();

		return {};
	}

	error lineRenderer::addLine(VkDevice device, VmaAllocator allocator, submit& is, line l)
	{
		return mResourceManager->addLine(
			resourceManager::addLineParams{
				.device = device,
				.allocator = allocator,
				.is = is,
				.l = l,
			}
		);
	}

	error lineRenderer::drawLines(VkCommandBuffer cmd, uint32_t frameIndex)
	{
		VkClearValue clear{
			.color = VkClearColorValue{.float32 = { 0.0f, 0.0f, 0.0f, 0.0f} },
		};

		VkRenderingAttachmentInfo colorAttachment = attachmentInfo(
			mResourceManager->getColorAttachmentImage(false).img.view, 
			mPreset.msaa <= 1 ? nullptr : mResourceManager->getColorAttachmentImage(true).img.view,
			getResolveMode(mPreset.msaa), 
			&clear, 
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
		VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(
			mResourceManager->getDepthImage(false).img.view,
			mPreset.msaa <= 1 ? nullptr : mResourceManager->getDepthImage(true).img.view,
			getResolveMode(mPreset.msaa), 
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
		);

		std::vector<VkRenderingAttachmentInfo> colorAttachments = { colorAttachment };

		VkRenderingInfo renderInfo = renderingInfo(
			mResourceManager->getColorAttachmentImage(false).img.extent,
			colorAttachments, 
			&depthAttachment
		);

		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.getPipeline().first);

		linePushConstant pc{
			.frameIndex = frameIndex,
		};

		vkCmdPushConstants(cmd, mPipeline.getPipeline().second, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(linePushConstant), &pc);

		mResourceManager->bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.getPipeline().second);

		vkCmdDraw(cmd, uint32_t(mResourceManager->mLineBuffer.getLoadedBytes() / sizeof(glm::vec3)), 1, 0, 0);

		vkCmdEndRendering(cmd);

		return {};
	}

	error lineRenderer::initPipeline(VkDevice device, VkFormat depthFormat, VkFormat drawFormat)
	{
		auto vertexShader = mCtx->mAmanager->getDefaultLineVertexShader();
		if (!vertexShader)
			return vertexShader.err();

		auto pixelShader = mCtx->mAmanager->getDefaultLinePixelShader();
		if (!pixelShader)
			return pixelShader.err();

		mPipeline.init(device, graphicsPipeline::pipelineType::opaque);

		error err = mPipeline.buildLinePipeline(
			pixelShader.value(),
			vertexShader.value(),
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second },
			depthFormat,
			{ drawFormat },
			sampleCounts(mPreset.msaa)
		);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::graphicsPipe, .graphicsPipe = &mPipeline });

		return {};
	}
}
