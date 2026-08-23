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
		const std::vector<vulkanBuffer>& UBObuffer,
		VkFormat depthFormat,
		VkFormat drawFormat,
		graphicsPreset preset,
		deviceLimits limits
	)
	{
		mCtx = ctx;

		mBindings = lineBindings{
			.descriptorSet = 0,
			.totalDescriptorsCount = 2,
			.vertexBinding = 0,
			.perDrawDataBinding = 1,
		};

		mNeedDescrotprUpdate = false;

		mPreset = preset;

		mDeletionQueue.init(device);

		mVertexBuffer.init(device, allocator, { true, false });

		error err = mVertexBuffer.build(is, nullptr, sizeof(glm::vec3) * 2 * 5000, 0);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::vulkanBuf, .vulkanBuf = &mVertexBuffer });

		err = initDescriptors(device, physicalDevice, UBObuffer, limits);
		if (err)
			return err;

		err = initPipeline(device, depthFormat, drawFormat);
		if (err)
			return err;

		return {};
	}

	error lineRenderer::destroy()
	{
		mDeletionQueue.flushDeletonQueue();

		return {};
	}

	error lineRenderer::addLine(glm::vec3 p1, glm::vec3 p2)
	{
		mNeedDescrotprUpdate = true;

		mVertexData.push_back(lineVertex{ p1 });
		mVertexData.push_back(lineVertex{ p2 });

		return {};
	}

	error lineRenderer::updateDescriptors(VmaAllocator allocator, submit& is)
	{
		if (mNeedDescrotprUpdate)
		{
			mVertexBuffer.markBytesAsDead(mVertexBuffer.getLoadedBytes());

			error err = mVertexBuffer.updateBuffer(is, mVertexData.data(), sizeof(lineVertex) * mVertexData.size(), 0);
			if (err && err.is(errCodeBufferOverFlow))
			{
				mVertexBuffer.destroy();

				err = mVertexBuffer.build(is, mVertexData.data(), sizeof(lineVertex) * mVertexData.size() * 2, sizeof(lineVertex) * mVertexData.size());
				if (err)
					return err;
			}
			if (err)
				return err;
		}

		// Set ubo buffer write right away.
		std::vector<VkDescriptorBufferInfo> bufferInfo{
			VkDescriptorBufferInfo{.buffer = mVertexBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE}
		};

		auto writeInfo = descriptorSet::getWriteInfo(mBindings.vertexBinding, bufferInfo);
		mDescriptorSet.updateWrite(writeInfo);

		mNeedDescrotprUpdate = false;

		return {};
	}

	error lineRenderer::drawLines(VkCommandBuffer cmd, const swapChain& sChain, uint32_t frameIndex)
	{
		VkClearValue clear{
			.color = VkClearColorValue{.float32 = { 0.0f, 0.0f, 0.0f, 0.0f} },
		};

		VkRenderingAttachmentInfo colorAttachment = attachmentInfo(sChain.getDrawImageView(false), mPreset.msaa <= 1 ? nullptr : sChain.getDrawImageView(true), getResolveMode(mPreset.msaa), &clear, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(sChain.getDepthImageView(false), mPreset.msaa <= 1 ? nullptr : sChain.getDepthImageView(true), getResolveMode(mPreset.msaa), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		std::vector<VkRenderingAttachmentInfo> colorAttachments = { colorAttachment };

		VkRenderingInfo renderInfo = renderingInfo(sChain.getDrawImageExtent(), colorAttachments, &depthAttachment);

		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.getPipeline().first);

		linePushConstant pc{
			.frameIndex = frameIndex,
		};

		vkCmdPushConstants(cmd, mPipeline.getPipeline().second, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(linePushConstant), &pc);

		// bind the descriptor set.
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.getPipeline().second, mBindings.descriptorSet, 1, &set, 0, nullptr);

		vkCmdDraw(cmd, uint32_t(mVertexData.size()), 1, 0, 0);

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
			{ mDescriptorSet.getDescriptorSet().second },
			depthFormat,
			{ drawFormat },
			sampleCounts(mPreset.msaa)
		);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::graphicsPipe, .graphicsPipe = &mPipeline });

		return {};
	}

	error lineRenderer::initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, const std::vector<vulkanBuffer>& UBObuffer, deviceLimits limits)
	{
		error err = mDescriptorSet.init(
			device,
			physicalDevice,
			poolConstraints{
				.maxBuffersDescriptors = limits.maxStorageBuffers,
				.maxUniformBuffersDescriptors = limits.maxUniformBuffers,
			}
			);
		if (err)
			return err;

		const uint32_t bufferObjects = 1;
		const uint32_t uniformObjects = 1;

		// add bindings for buffers.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.vertexBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perDrawDataBinding, limits.maxUniformBuffers / uniformObjects, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			)
		);

		err = mDescriptorSet.build(VK_SHADER_STAGE_ALL, mBindings.totalDescriptorsCount);
		if (err)
			return err;

		// Set ubo buffer write right away.
		std::vector<VkDescriptorBufferInfo> bufferInfo{};
		for (auto& b : UBObuffer)
			bufferInfo.push_back(VkDescriptorBufferInfo{ .buffer = b.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE });

		auto writeInfo = descriptorSet::getWriteInfo(mBindings.perDrawDataBinding, bufferInfo, true);
		mDescriptorSet.updateWrite(writeInfo);

		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::descSet, .descSet = &mDescriptorSet });

		return {};
	}
}
