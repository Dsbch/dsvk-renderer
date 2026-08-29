#include <pch.h>
#include "meshletRenderer.h"
#include "renderer.h"
#include "computeRenderer.h"

namespace engine
{
	error meshletRenderer::init(
		std::shared_ptr<context> ctx,
		PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT,
		PFN_vkCmdDrawMeshTasksIndirectEXT vkCmdDrawMeshTasksIndirectEXT,
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VmaAllocator allocator,
		submit& is,
		deviceLimits limits,
		graphicsPreset preset,
		std::shared_ptr<resourceManager> resourceManager
	)
	{
		if (!vkCmdDrawMeshTasksEXT)
			return { "vkCmdDrawMeshTasksEXT nullptr" };

		mVkCmdDrawMeshTasksEXT = vkCmdDrawMeshTasksEXT;
		mVkCmdDrawMeshTasksIndirectEXT = vkCmdDrawMeshTasksIndirectEXT;

		mCtx = ctx;

		mResourceManager = resourceManager;

		mDeletionQueue.init(device);

		mPreset = preset;

		error err = initBlendingPipelines(device, allocator, is);
		if (err)
			return err;

		err = initVoxelPipelines(device, allocator, is);
		if (err)
			return err;

		err = mComputeRenderer.init(mCtx, device, physicalDevice, allocator, is, limits, mPreset, mResourceManager);
		if (err)
			return err;

		return {};
	}

	error meshletRenderer::destroy()
	{
		mDeletionQueue.flushDeletonQueue();

		mComputeRenderer.destroy();

		return {};
	}

	error meshletRenderer::initBlendingPipelines(VkDevice device, VmaAllocator allocator, submit& is)
	{
		auto meshlets = mCtx->mAmanager->getDefaultAccumilateMeshShader();
		if (!meshlets)
			return meshlets.err();

		auto task = mCtx->mAmanager->getDefaultAccumilateTaskShader();
		if (!task)
			return task.err();

		auto pixel = mCtx->mAmanager->getDefaultAccumilatePixelShader();
		if (!pixel)
			return pixel.err();

		mAccumilationPipeline.init(device, graphicsPipeline::pipelineType::accumilation);

		error err = mAccumilationPipeline.build(
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second },
			mResourceManager->getDepthImage(false).img.format,
			{ mResourceManager->getAccumImage(false).img.format, mResourceManager->getRevealImage(false).img.format },
			sampleCounts(mPreset.msaa)
		);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::graphicsPipe, .graphicsPipe = &mAccumilationPipeline });

		meshlets = mCtx->mAmanager->getDefaultCompositeMeshShader();
		if (!meshlets)
			return meshlets.err();

		task = mCtx->mAmanager->getDefaultCompositeTaskShader();
		if (!task)
			return task.err();

		pixel = mCtx->mAmanager->getDefaultCompositePixelShader();
		if (!pixel)
			return pixel.err();

		mCompositePipeline.init(device, graphicsPipeline::pipelineType::composite);

		err = mCompositePipeline.build(
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second },
			mResourceManager->getDepthImage(false).img.format,
			{ mResourceManager->getColorAttachmentImage(false).img.format },
			sampleCounts(mPreset.msaa)
		);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::graphicsPipe, .graphicsPipe = &mCompositePipeline });

		return {};
	}

	error meshletRenderer::initVoxelPipelines(VkDevice device, VmaAllocator allocator, submit& is)
	{
		auto meshlets = mCtx->mAmanager->getDefaultVoxelMeshShader();
		if (!meshlets)
			return meshlets.err();

		auto task = mCtx->mAmanager->getDefaultVoxelTaskShader();
		if (!task)
			return task.err();

		auto pixel = mCtx->mAmanager->getDefaultVoxelPixelShader();
		if (!pixel)
			return pixel.err();

		mVoxelizationPipeline.init(device, graphicsPipeline::pipelineType::voxelization);

		error err = mVoxelizationPipeline.build(
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second },
			{},
			{},
			sampleCounts(mPreset.msaa)
		);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::graphicsPipe, .graphicsPipe = &mVoxelizationPipeline });

		return {};
	}

	aabb meshletRenderer::getSceneBoundingBox() const
	{
		aabb result{
			.min = glm::vec3{std::numeric_limits<float>::max()},
			.max = glm::vec3{std::numeric_limits<float>::lowest()},
		};

		if (mSceneAABB.size() == 0)
			return aabb{};

		for (auto& [_, v] : mSceneAABB)
		{
			result.max = glm::max(result.max, v.max);
			result.min = glm::min(result.min, v.min);
		}

		return result;
	}

	error meshletRenderer::opaquePass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		uint32_t cmdBufferIndex = 0;
		for (auto& [k, v] : mOpaquePipelines)
		{
			auto [pipeline, pipelineLayout] = v.getPipeline();

			VkBuffer cmdBuf = mResourceManager->mOpaqueCommandBuffers[k].getBuffer(frameIndex).getBuffer().buffer;
			uint32_t cmdBufSize = uint32_t(mResourceManager->mOpaqueCommandBuffers[k].getBuffer(frameIndex).getLoadedBytes());
			uint32_t cmdBufferCount = uint32_t(cmdBufSize / sizeof(meshletShaderCMD));

			// Has to render.
			if (cmdBufferCount > 0)
			{
				// FIRST PASS.
				{
					// Dispatch indirect compute call for culling and lod level selection.
					error err = mComputeRenderer.cullMeshlets(
						cmd,
						in,
						computeRenderer::cullMeshletsParams{
							.cmdBufferCount = cmdBufferCount,
							.cullStage = FIRST_OPAQUE_PASS_FLAG_BIT,
							.opaqueCmdBufferIndex = cmdBufferIndex * mCtx->config.inner.graphics.framesInFlight + frameIndex,
						},
						frameIndex
						);
					if (err)
						return err;

					// Wait for compute call to finish it writes.
					pipelineBufferBarier(
						cmd,
						cmdBuf,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_WRITE_BIT,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_READ_BIT,
						cmdBufSize,
						0
					);

					pipelineBufferBarier(
						cmd, mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
						VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
						VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
						VK_PIPELINE_STAGE_2_CLEAR_BIT,
						VK_ACCESS_2_TRANSFER_WRITE_BIT,
						uint32_t(sizeof(uint32_t) * 2),
						0
					);

					vkCmdFillBuffer(cmd, mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer, 0, sizeof(uint32_t) * 2, 0u);

					pipelineBufferBarier(
						cmd, mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
						VK_PIPELINE_STAGE_2_CLEAR_BIT,
						VK_ACCESS_2_TRANSFER_WRITE_BIT,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT,
						uint32_t(sizeof(uint32_t) * 2),
						0
					);

					// Compact cmd buffer to visability buffer.
					err = mComputeRenderer.compactCommandBuffer(
						cmd,
						in,
						computeRenderer::compactCommandBufferParams{
							.cmdBufferCount = cmdBufferCount,
							.opaqueCmdBufferIndex = cmdBufferIndex * mCtx->config.inner.graphics.framesInFlight + frameIndex,
							.stage = FIRST_OPAQUE_PASS_FLAG_BIT,
							.compactRule = VISIBLE_FIRST_PASS_FLAG_BIT,
						},
						frameIndex
						);
					if (err)
						return err;

					pipelineBufferBarier(
						cmd,
						mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_WRITE_BIT,
						VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
						VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
						uint32_t(mResourceManager->mVisabilityBuffer[frameIndex].getLoadedBytes()),
						0
					);

					VkRenderingAttachmentInfo colorAttachment = attachmentInfo(
						mResourceManager->getColorAttachmentImage(false).img.view,
						mPreset.msaa <= 1 ? nullptr : mResourceManager->getColorAttachmentImage(true).img.view,
						getResolveMode(mPreset.msaa),
						nullptr,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
					);
					VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(
						mResourceManager->getDepthImage(false).img.view,
						mPreset.msaa <= 1 ? nullptr : mResourceManager->getDepthImage(true).img.view,
						getResolveMode(mPreset.msaa),
						VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
						false
					);

					std::vector<VkRenderingAttachmentInfo> colorAttachments = { colorAttachment };

					VkRenderingInfo renderInfo = renderingInfo(
						mResourceManager->getColorAttachmentImage(false).img.extent,
						colorAttachments,
						&depthAttachment
					);

					vkCmdBeginRendering(cmd, &renderInfo);

					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

					pushConstants pc{
						.frameIndex = frameIndex,
						.cmdBufferCount = cmdBufferCount,
						.cmdOpaqueBufferIndex = cmdBufferIndex * mCtx->config.inner.graphics.framesInFlight + frameIndex,
					};

					vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

					mResourceManager->bindDescriptorSets(
						cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineLayout
					);

					mVkCmdDrawMeshTasksIndirectEXT(
						cmd,
						mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
						sizeof(uint32_t),
						1,
						12
					);

					vkCmdEndRendering(cmd);
				}

				// SECOND PASS.
				{
					// Build HZB.					
					mResourceManager->transitionDepthImage(
						cmd,
						VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
						VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_READ_BIT
					);

					error err = mComputeRenderer.buildHZB(cmd, in, frameIndex);
					if (err)
						return err;

					// Wait for HZB to generate.
					for (auto& hzb : mResourceManager->getHZB())
					{
						pipelineImageBarrier(
							cmd,
							hzb.img.image,
							hzb.img.format,
							VK_IMAGE_LAYOUT_GENERAL,
							VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
							VK_ACCESS_2_SHADER_WRITE_BIT,
							VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
							VK_ACCESS_2_SHADER_READ_BIT
						);
					}

					// Dispatch compute call for culling and lod level selection.
					err = mComputeRenderer.cullMeshlets(
						cmd,
						in,
						computeRenderer::cullMeshletsParams{
							.cmdBufferCount = cmdBufferCount,
							.cullStage = SECOND_OPAQUE_PASS_FLAG_BIT,
							.opaqueCmdBufferIndex = cmdBufferIndex * mCtx->config.inner.graphics.framesInFlight + frameIndex,
							.hzbLength = uint32_t(mResourceManager->getHZB().size()),
						},
						frameIndex
						);
					if (err)
						return err;

					// Wait for compute call to finish it writes.
					pipelineBufferBarier(
						cmd,
						cmdBuf,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_WRITE_BIT,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_READ_BIT,
						cmdBufSize,
						0
					);

					pipelineBufferBarier(
						cmd, mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_MEMORY_READ_BIT,
						VK_PIPELINE_STAGE_2_CLEAR_BIT,
						VK_ACCESS_2_TRANSFER_WRITE_BIT,
						uint32_t(sizeof(uint32_t) * 2),
						0
					);

					vkCmdFillBuffer(cmd, mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer, 0, sizeof(uint32_t) * 2, 0u);

					pipelineBufferBarier(
						cmd, mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
						VK_PIPELINE_STAGE_2_CLEAR_BIT,
						VK_ACCESS_2_TRANSFER_WRITE_BIT,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT,
						uint32_t(sizeof(uint32_t) * 2),
						0
					);

					// Compact cmd buffer to visability buffer.
					err = mComputeRenderer.compactCommandBuffer(
						cmd,
						in,
						computeRenderer::compactCommandBufferParams{
							.cmdBufferCount = cmdBufferCount,
							.opaqueCmdBufferIndex = cmdBufferIndex * mCtx->config.inner.graphics.framesInFlight + frameIndex,
							.stage = SECOND_OPAQUE_PASS_FLAG_BIT,
							.compactRule = VISIBLE_SECOND_PASS_FLAG_BIT,
						},
						frameIndex
						);
					if (err)
						return err;

					pipelineBufferBarier(
						cmd,
						mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_WRITE_BIT,
						VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
						uint32_t(mResourceManager->mVisabilityBuffer[frameIndex].getLoadedBytes()),
						0
					);

					// Transition depth after build HZB.
					mResourceManager->transitionDepthImage(
						cmd,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_READ_BIT,
						VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
						VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
					);

					VkRenderingAttachmentInfo colorAttachment = attachmentInfo(
						mResourceManager->getColorAttachmentImage(false).img.view,
						mPreset.msaa <= 1 ? nullptr : mResourceManager->getColorAttachmentImage(true).img.view,
						getResolveMode(mPreset.msaa),
						nullptr,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
					);
					VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(
						mResourceManager->getDepthImage(false).img.view,
						mPreset.msaa <= 1 ? nullptr : mResourceManager->getDepthImage(true).img.view,
						getResolveMode(mPreset.msaa),
						VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
						false
					);

					std::vector<VkRenderingAttachmentInfo> colorAttachments = { colorAttachment };

					VkRenderingInfo renderInfo = renderingInfo(mResourceManager->getColorAttachmentImage(false).img.extent, colorAttachments, &depthAttachment);

					vkCmdBeginRendering(cmd, &renderInfo);

					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

					pushConstants pc{
						.frameIndex = frameIndex,
						.cmdBufferCount = cmdBufferCount,
						.cmdOpaqueBufferIndex = cmdBufferIndex * mCtx->config.inner.graphics.framesInFlight + frameIndex,
					};

					vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

					mResourceManager->bindDescriptorSets(
						cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineLayout
					);

					mVkCmdDrawMeshTasksIndirectEXT(
						cmd,
						mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
						sizeof(uint32_t),
						1,
						12
					);

					vkCmdEndRendering(cmd);
				}
			}

			cmdBufferIndex++;
		}

		return {};
	}

	error meshletRenderer::accumilationPass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		// Add image barier, need to wait for opaque pass to finish for early depth test in accumilation pass.
		pipelineImageBarrier(
			cmd,
			mResourceManager->getDepthImage(false).img.image,
			mResourceManager->getDepthImage(false).img.format,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
		);

		VkClearValue clear{
			.color = VkClearColorValue{.float32 = { 0.0f, 0.0f, 0.0f, 0.0f} },
		};

		VkRenderingAttachmentInfo accumAttachment = attachmentInfo(
			mResourceManager->getAccumImage(false).img.view,
			mPreset.msaa <= 1 ? nullptr : mResourceManager->getAccumImage(true).img.view,
			getResolveMode(mPreset.msaa),
			&clear,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		clear.color = VkClearColorValue{ 1.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingAttachmentInfo revealAttachment = attachmentInfo(
			mResourceManager->getRevealImage(false).img.view,
			mPreset.msaa <= 1 ? nullptr : mResourceManager->getRevealImage(true).img.view,
			getResolveMode(mPreset.msaa),
			&clear,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		std::vector<VkRenderingAttachmentInfo> colorAttachments = { accumAttachment, revealAttachment };

		VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(
			mResourceManager->getDepthImage(false).img.view,
			mPreset.msaa <= 1 ? nullptr : mResourceManager->getDepthImage(true).img.view,
			getResolveMode(mPreset.msaa),
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			false
		);

		VkRenderingInfo renderInfo = renderingInfo(
			mResourceManager->getColorAttachmentImage(false).img.extent,
			colorAttachments,
			&depthAttachment
		);

		auto [pipeline, pipelineLayout] = mAccumilationPipeline.getPipeline();

		VkBuffer cmdBuf = mResourceManager->mAccumilationCommandBuffer.getBuffer(frameIndex).getBuffer().buffer;
		uint32_t cmdBufSize = uint32_t(mResourceManager->mAccumilationCommandBuffer.getBuffer(frameIndex).getLoadedBytes());
		uint32_t cmdBufferCount = uint32_t(cmdBufSize / sizeof(meshletShaderCMD));

		// Nothing to render.
		if (cmdBufferCount == 0)
		{
			vkCmdBeginRendering(cmd, &renderInfo);
			vkCmdEndRendering(cmd);

			return {};
		}

		// Dispatch compute call for culling and lod level selection.
		// For now it's only going to set falgs for each cmd buffer entry.
		// Later I will need to implement prefix sum on GPU to increase amplification rate.
		error err = mComputeRenderer.cullMeshlets(
			cmd,
			in,
			computeRenderer::cullMeshletsParams{
				.cmdBufferCount = cmdBufferCount,
				.cullStage = ACCUMILATION_PASS_FLAG_BIT,
				.hzbLength = uint32_t(mResourceManager->getHZB().size()),
			},
			frameIndex
			);
		if (err)
			return err;

		// Wait for compute call to finish it writes.
		pipelineBufferBarier(
			cmd,
			cmdBuf,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_READ_BIT,
			cmdBufSize,
			0
		);

		pipelineBufferBarier(
			cmd, mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
			VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
			VK_PIPELINE_STAGE_2_CLEAR_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			uint32_t(sizeof(uint32_t) * 2),
			0
		);

		vkCmdFillBuffer(cmd, mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer, 0, sizeof(uint32_t) * 2, 0u);

		pipelineBufferBarier(
			cmd, mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
			VK_PIPELINE_STAGE_2_CLEAR_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT,
			uint32_t(sizeof(uint32_t) * 2),
			0
		);

		// Compact cmd buffer to visability buffer.
		err = mComputeRenderer.compactCommandBuffer(
			cmd,
			in,
			computeRenderer::compactCommandBufferParams{
				.cmdBufferCount = cmdBufferCount,
				.stage = ACCUMILATION_PASS_FLAG_BIT,
				.compactRule = VISIBLE_FIRST_PASS_FLAG_BIT,
			},
			frameIndex
			);
		if (err)
			return err;

		pipelineBufferBarier(
			cmd,
			mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
			VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
			uint32_t(mResourceManager->mVisabilityBuffer[frameIndex].getSize()),
			0
		);

		pipelineBufferBarier(
			cmd,
			mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
			VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
			uint32_t(mResourceManager->mVisabilityBuffer[frameIndex].getLoadedBytes()),
			0
		);

		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		pushConstants pc{
			.frameIndex = frameIndex,
			.cmdBufferCount = cmdBufferCount,
			.cmdOpaqueBufferIndex = frameIndex,
		};

		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

		mResourceManager->bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout);

		mVkCmdDrawMeshTasksIndirectEXT(
			cmd,
			mResourceManager->mVisabilityBuffer[frameIndex].getBuffer().buffer,
			sizeof(uint32_t),
			1,
			12
		);

		vkCmdEndRendering(cmd);

		return {};
	}

	error meshletRenderer::compositePass(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		// Composite opaque and transperent.
		VkRenderingAttachmentInfo colorAttachment = attachmentInfo(
			mResourceManager->getColorAttachmentImage(false).img.view,
			mPreset.msaa <= 1 ? nullptr : mResourceManager->getColorAttachmentImage(true).img.view,
			getResolveMode(mPreset.msaa),
			nullptr,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		std::vector<VkRenderingAttachmentInfo> colorAttachments = { colorAttachment };

		VkRenderingInfo renderInfo = renderingInfo(
			mResourceManager->getColorAttachmentImage(false).img.extent,
			colorAttachments,
			nullptr
		);

		vkCmdBeginRendering(cmd, &renderInfo);

		auto [pipeline, pipelineLayout] = mCompositePipeline.getPipeline();

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		mResourceManager->bindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout);

		mVkCmdDrawMeshTasksEXT(cmd, 1, 1, 1);

		vkCmdEndRendering(cmd);

		return {};
	}

	voxelDrawParams meshletRenderer::getVoxelSceneParams() const
	{
		// For now it's just hardcoded values. Need to figure out where to put them next.
		const uint32_t voxelSceneUpperBound = 64;
		const uint32_t voxelGridExtent = 512;

		aabb box = getSceneBoundingBox();

		glm::mat4 view = glm::translate(glm::mat4{ 1.0f }, -(box.max + box.min) * 0.5f);

		const float halfExtent = float(voxelSceneUpperBound) * 0.5f;

		glm::mat4 proj = glm::ortho(
			-halfExtent, halfExtent,
			-halfExtent, halfExtent,
			-halfExtent, halfExtent);

		voxelDrawParams result{
			.voxelGridExtent = voxelGridExtent,
			.voxelSceneUpperBound = voxelSceneUpperBound,
			.viewVoxel = view,
			.projectionVoxel = proj,
			.viewProjectionVoxel = proj * view,
		};

		return result;
	}

	error meshletRenderer::voxilizeOpaqueGeometry(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		uint32_t cmdBufferIndex = 0;
		for (auto& [k, v] : mOpaquePipelines)
		{
			VkBuffer cmdBuf = mResourceManager->mOpaqueCommandBuffers[k].getBuffer(frameIndex).getBuffer().buffer;
			uint32_t cmdBufSize = uint32_t(mResourceManager->mOpaqueCommandBuffers[k].getBuffer(frameIndex).getLoadedBytes());
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

				mVkCmdDrawMeshTasksEXT(
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

	error meshletRenderer::addToRender(const model& m, VkDevice device, VmaAllocator allocator, submit& is, uint32_t frameIndex)
	{
		mSceneAABB[m.id] = m.calculateWorldSpaceAABB();

		if (!mOpaquePipelines.contains(m.mat.pixelShader->hash()))
		{
			auto meshShader = mCtx->mAmanager->getDefaultMeshShader();
			if (!meshShader)
				return meshShader.err();

			auto taskShader = mCtx->mAmanager->getDefaultTaskShader();
			if (!taskShader)
				return taskShader.err();

			graphicsPipeline pipeline{};

			pipeline.init(device, graphicsPipeline::pipelineType::opaque);

			error err = pipeline.build(
				m.mat.pixelShader,
				meshShader.value(),
				taskShader.value(),
				{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second },
				mResourceManager->getDepthImage(false).img.format,
				{ mResourceManager->getColorAttachmentImage(false).img.format },
				sampleCounts(mPreset.msaa)
			);
			if (err)
				return err;

			mOpaquePipelines[m.mat.pixelShader->hash()] = pipeline;

			mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::graphicsPipe, .graphicsPipe = &mOpaquePipelines[m.mat.pixelShader->hash()] });
		}

		return mResourceManager->addToRender(resourceManager::instanceParams{
				.device = device,
				.allocator = allocator,
				.is = is,
				.m = m,
				.frameIndex = frameIndex
			}
		);
	}

	error meshletRenderer::updateInstance(const model& m, VkDevice device, VmaAllocator allocator, submit& is, uint32_t frameIndex)
	{
		return mResourceManager->updateInstance(resourceManager::instanceParams{
				.device = device,
				.allocator = allocator,
				.is = is,
				.m = m,
				.frameIndex = frameIndex
			}
		);
	}

	error meshletRenderer::updateAnimations(const model& m, VkDevice device, VmaAllocator allocator, submit& is, uint32_t frameIndex)
	{
		return mResourceManager->updateAnimations(resourceManager::instanceParams{
				.device = device,
				.allocator = allocator,
				.is = is,
				.m = m,
				.frameIndex = frameIndex
			}
		);
	}

	void meshletRenderer::removeFromRender(const model& m, VkDevice device, VmaAllocator allocator, submit& is, uint32_t frameIndex)
	{
		mSceneAABB.erase(m.id);

		return mResourceManager->removeFromRender(resourceManager::instanceParams{
				.device = device,
				.allocator = allocator,
				.is = is,
				.m = m,
				.frameIndex = frameIndex
			}
		);
	}
}
