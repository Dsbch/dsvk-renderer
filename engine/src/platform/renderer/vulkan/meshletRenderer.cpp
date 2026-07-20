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
		VkBuffer UBObuffer,
		const swapChain& sChain
	)
	{
		if (!vkCmdDrawMeshTasksEXT)
			return { "vkCmdDrawMeshTasksEXT nullptr" };

		mVkCmdDrawMeshTasksEXT = vkCmdDrawMeshTasksEXT;
		mVkCmdDrawMeshTasksIndirectEXT = vkCmdDrawMeshTasksIndirectEXT;

		mCtx = ctx;

		mBindings = meshletBindings{
			.descriptorSet = 0,
			.totalDescriptorsCount = 18,

			// Vertex attributes.
			.positionsBinding = 0,
			.normalBinding = 1,
			.tangentBinding = 2,
			.jointIndexBinding = 3,
			.wightBinding = 4,

			// Buffers.
			.perInstanceBinding = 11,
			.cmdOpaqueBufferBinding = 12,
			.cmdAccumilationBufferBinding = 13,
			.indexBinding = 14,
			.primitiveBinding = 15,
			.meshletBinding = 16,
			.jointsBinding = 17,
			.perMeshBinding = 18,
			.visabilityBuffer = 19,
			.perDrawBufferUboBinding = 20,

			// Materials.
			.materialArrayBinding = 51,
			.accumBinding = 52,
			.revealBinding = 53,
		};

		mVisabilityBufferSize = 2 << 24;

		mDeletionQueue.init(device);

		mPreset = preset;

		error err = initRegistry(device, allocator, is);
		if (err)
			return err;

		err = initDescriptors(device, physicalDevice, limits, UBObuffer);
		if (err)
			return err;

		err = initBlendingPipelines(device, sChain, allocator, is);
		if (err)
			return err;

		err = mComputeRenderer.init(mCtx, device, physicalDevice, allocator, is, limits, mPreset, UBObuffer, mVisabilityBuffer.getBuffer().buffer, sChain);
		if (err)
			return err;

		err = updateSwapchainDependentDescriptors(sChain);
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

	error meshletRenderer::initRegistry(VkDevice device, VmaAllocator allocator, submit& is)
	{
		mPositionRegistry.init(device, allocator);
		mNormalRegistry.init(device, allocator);
		mTangentRegistry.init(device, allocator);
		mJointIndexRegistry.init(device, allocator);
		mWeightRegistry.init(device, allocator);

		mIndexRegistry.init(device, allocator);

		mPrimitiveRegistry.init(device, allocator);

		mMeshletRegistry.init(device, allocator);

		mPerMeshRegistry.init(device, allocator);

		// Updated each frame used as MAPPED.
		mPerInstanceRegistry.init(device, allocator, { true, false });

		mJointRegistry.init(device, allocator, { true, false });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mPositionRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mNormalRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mTangentRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mJointIndexRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mWeightRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mIndexRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mPrimitiveRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mMeshletRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mPerInstanceRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mJointRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mPerMeshRegistry });

		auto samp = descriptorSet::createSampler(device, float(mPreset.anisotropicFiltering));
		if (!samp)
			return samp.err();

		mSampler = samp.value();

		mDeletionQueue.addDestroyTask(destroyTask{ .type = sampler, .sampler = &mSampler });

		auto defaultMat = mCtx->mAmanager->loadDetaultMaterial();
		if (!defaultMat)
			return defaultMat.err();

		error err = mMaterialRegistry.init(mSampler, defaultMat.value());
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = matReg, .matReg = &mMaterialRegistry });

		// Init visability buffers.
		std::vector<uint32_t> visDispatch{ 0, 0, 1, 1 };

		mVisabilityBuffer.init(device, allocator);

		err = mVisabilityBuffer.build(is, visDispatch.data(), mVisabilityBufferSize, sizeof(uint32_t) * 4, true);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = vulkanBuf, .vulkanBuf = &mVisabilityBuffer });

		return {};
	}

	error meshletRenderer::initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, deviceLimits limits, VkBuffer UBObuffer)
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

		const uint32_t combinedImageSamplers = 3;
		const uint32_t bufferObjects = 14;
		const uint32_t uniformObjects = 1;

		// add bindings for blending stage.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.accumBinding, limits.maxCombinedImageSamplers / combinedImageSamplers, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.revealBinding, limits.maxCombinedImageSamplers / combinedImageSamplers, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		// add bindings for materials.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.materialArrayBinding, limits.maxCombinedImageSamplers / combinedImageSamplers, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		// add bindings for vertex attributes.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.positionsBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.normalBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.tangentBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.jointIndexBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.wightBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		// add bindings for buffers.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perInstanceBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.cmdOpaqueBufferBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.cmdAccumilationBufferBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.indexBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.primitiveBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.meshletBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.jointsBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perMeshBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.visabilityBuffer, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perDrawBufferUboBinding, limits.maxUniformBuffers / uniformObjects, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			)
		);

		err = mDescriptorSet.build(VK_SHADER_STAGE_ALL, mBindings.totalDescriptorsCount);
		if (err)
			return err;

		// Set descriptor for ubo buffer write right away.
		std::vector<VkDescriptorBufferInfo> bufferInfo{
			VkDescriptorBufferInfo{.buffer = UBObuffer, .offset = 0, .range = VK_WHOLE_SIZE }
		};

		auto writeInfo = descriptorSet::getWriteInfo(mBindings.perDrawBufferUboBinding, bufferInfo, true);
		mDescriptorSet.updateWrite(writeInfo);

		// Set descriptor for visabilityBuffers right away.
		bufferInfo = {
			VkDescriptorBufferInfo{.buffer = mVisabilityBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE}
		};

		writeInfo = descriptorSet::getWriteInfo(mBindings.visabilityBuffer, bufferInfo);
		mDescriptorSet.updateWrite(writeInfo);

		mDeletionQueue.addDestroyTask(destroyTask{ .type = descSet, .descSet = &mDescriptorSet });

		return {};
	}

	error meshletRenderer::updateSwapchainDependentDescriptors(const swapChain& sChain)
	{
		std::vector<VkDescriptorImageInfo> info{ VkDescriptorImageInfo{} };
		info.front().sampler = mSampler;
		info.front().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.front().imageView = sChain.getAccumImageView(mPreset.msaa > 1);

		std::vector<VkWriteDescriptorSet> wSet = descriptorSet::getWriteInfo(mBindings.accumBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info);

		mDescriptorSet.updateWrite(wSet);

		info.front().sampler = mSampler;
		info.front().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.front().imageView = sChain.getRevealImageView(mPreset.msaa > 1);

		wSet = descriptorSet::getWriteInfo(mBindings.revealBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info);

		mDescriptorSet.updateWrite(wSet);

		return mComputeRenderer.updateSwapchainDependentDescriptors(sChain);
	}

	error meshletRenderer::initBlendingPipelines(VkDevice device, const swapChain& sChain, VmaAllocator allocator, submit& is)
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

		error err = mAccumilationPipeline.init(
			device,
			allocator,
			is,
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mDescriptorSet.getDescriptorSet().second },
			sChain.getDepthImageFormat(),
			{ sChain.getAccumImageFormat(), sChain.getRevealImageFormat() },
			sampleCounts(mPreset.msaa),
			pipelineData::pipelineType::accumilation
		);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = pipeData, .pipeData = &mAccumilationPipeline });

		meshlets = mCtx->mAmanager->getDefaultCompositeMeshShader();
		if (!meshlets)
			return meshlets.err();

		task = mCtx->mAmanager->getDefaultCompositeTaskShader();
		if (!task)
			return task.err();

		pixel = mCtx->mAmanager->getDefaultCompositePixelShader();
		if (!pixel)
			return pixel.err();

		err = mCompositePipeline.init(
			device,
			allocator,
			is,
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mDescriptorSet.getDescriptorSet().second },
			sChain.getDepthImageFormat(),
			{ sChain.getDrawImageFormat() },
			sampleCounts(mPreset.msaa),
			pipelineData::pipelineType::composite
		);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = pipeData, .pipeData = &mCompositePipeline });

		return {};
	}

	uint32_t meshletRenderer::getMaxCmdBufferSize() const
	{
		uint32_t opaque{};
		for (auto& [_, v] : mPipelines)
			opaque += v.getCommandBufferLoadedSize();

		return std::max(opaque, mAccumilationPipeline.getCommandBufferLoadedSize());
	}

	error meshletRenderer::opaquePass(VkCommandBuffer cmd, renderer::renderParams in, const swapChain& sChain)
	{
		uint32_t cmdBufferIndex = 0;
		for (auto& [_, v] : mPipelines)
		{
			auto pipeline = v.getPipelineRenderData();

			VkBuffer cmdBuf = v.getBuffer().getBuffer().buffer;
			uint32_t cmdBufSize = uint32_t(v.getBuffer().getLoadedBytes());

			// Has to render.
			if (pipeline.cmdBufferCount > 0)
			{
				// FIRST PASS.
				{
					// Dispatch indirect compute call for culling and lod level selection.
					error err = mComputeRenderer.cullMeshlets(
						cmd,
						in,
						computeRenderer::cullMeshletsParams{
							.cmdBufferCount = pipeline.cmdBufferCount,
							.cullStage = FIRST_OPAQUE_PASS_FLAG_BIT,
							.opaqueCmdBufferIndex = cmdBufferIndex,
						}
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
						cmd, mVisabilityBuffer.getBuffer().buffer,
						VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
						VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
						VK_PIPELINE_STAGE_2_CLEAR_BIT,
						VK_ACCESS_2_TRANSFER_WRITE_BIT,
						uint32_t(sizeof(uint32_t) * 2),
						0
					);

					vkCmdFillBuffer(cmd, mVisabilityBuffer.getBuffer().buffer, 0, sizeof(uint32_t) * 2, 0u);

					pipelineBufferBarier(
						cmd, mVisabilityBuffer.getBuffer().buffer,
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
							.cmdBufferCount = pipeline.cmdBufferCount,
							.opaqueCmdBufferIndex = cmdBufferIndex,
							.stage = FIRST_OPAQUE_PASS_FLAG_BIT,
							.compactRule = VISIBLE_FIRST_PASS_FLAG_BIT,
						}
						);
					if (err)
						return err;

					pipelineBufferBarier(
						cmd,
						mVisabilityBuffer.getBuffer().buffer,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_WRITE_BIT,
						VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
						VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
						uint32_t(mVisabilityBuffer.getLoadedBytes()),
						0
					);

					VkRenderingAttachmentInfo colorAttachment = attachmentInfo(sChain.getDrawImageView(false), mPreset.msaa <= 1 ? nullptr : sChain.getDrawImageView(true), getResolveMode(mPreset.msaa), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(sChain.getDepthImageView(false), mPreset.msaa <= 1 ? nullptr : sChain.getDepthImageView(true), getResolveMode(mPreset.msaa), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, false);

					std::vector<VkRenderingAttachmentInfo> colorAttachments = { colorAttachment };

					VkRenderingInfo renderInfo = renderingInfo(sChain.getDrawImageExtent(), colorAttachments, &depthAttachment);

					vkCmdBeginRendering(cmd, &renderInfo);

					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

					pushConstants pc{
						.cmdBufferCount = pipeline.cmdBufferCount,
						.opaqueCmdBufferIndex = cmdBufferIndex,
					};

					vkCmdPushConstants(cmd, pipeline.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

					// bind the descriptor set.
					auto set = mDescriptorSet.getDescriptorSet().first;
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, mBindings.descriptorSet, 1, &set, 0, nullptr);

					mVkCmdDrawMeshTasksIndirectEXT(
						cmd,
						mVisabilityBuffer.getBuffer().buffer,
						sizeof(uint32_t),
						1,
						12
					);

					vkCmdEndRendering(cmd);
				}

				// SECOND PASS.
				{
					// Build HZB.					
					sChain.transitionDepthImage(
						cmd,
						VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
						VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_READ_BIT
					);

					error err = mComputeRenderer.buildHZB(cmd, in, sChain);
					if (err)
						return err;

					// Wait for HZB to generate.
					for (auto& hzb : sChain.getHZB())
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
							.cmdBufferCount = pipeline.cmdBufferCount,
							.cullStage = SECOND_OPAQUE_PASS_FLAG_BIT,
							.opaqueCmdBufferIndex = cmdBufferIndex,
							.hzbLength = uint32_t(sChain.getHzbSize()),
						}
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
						cmd, mVisabilityBuffer.getBuffer().buffer,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_MEMORY_READ_BIT,
						VK_PIPELINE_STAGE_2_CLEAR_BIT,
						VK_ACCESS_2_TRANSFER_WRITE_BIT,
						uint32_t(sizeof(uint32_t) * 2),
						0
					);

					vkCmdFillBuffer(cmd, mVisabilityBuffer.getBuffer().buffer, 0, sizeof(uint32_t) * 2, 0u);

					pipelineBufferBarier(
						cmd, mVisabilityBuffer.getBuffer().buffer,
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
							.cmdBufferCount = pipeline.cmdBufferCount,
							.opaqueCmdBufferIndex = cmdBufferIndex,
							.stage = SECOND_OPAQUE_PASS_FLAG_BIT,
							.compactRule = VISIBLE_SECOND_PASS_FLAG_BIT,
						}
						);
					if (err)
						return err;

					pipelineBufferBarier(
						cmd,
						mVisabilityBuffer.getBuffer().buffer,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_WRITE_BIT,
						VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
						uint32_t(mVisabilityBuffer.getLoadedBytes()),
						0
					);

					// Transition depth after build HZB.
					sChain.transitionDepthImage(
						cmd,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
						VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
						VK_ACCESS_2_SHADER_READ_BIT,
						VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
						VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
					);

					VkRenderingAttachmentInfo colorAttachment = attachmentInfo(sChain.getDrawImageView(false), mPreset.msaa <= 1 ? nullptr : sChain.getDrawImageView(true), getResolveMode(mPreset.msaa), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(sChain.getDepthImageView(false), mPreset.msaa <= 1 ? nullptr : sChain.getDepthImageView(true), getResolveMode(mPreset.msaa), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, false);

					std::vector<VkRenderingAttachmentInfo> colorAttachments = { colorAttachment };

					VkRenderingInfo renderInfo = renderingInfo(sChain.getDrawImageExtent(), colorAttachments, &depthAttachment);

					vkCmdBeginRendering(cmd, &renderInfo);

					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

					pushConstants pc{
						.cmdBufferCount = pipeline.cmdBufferCount,
						.opaqueCmdBufferIndex = cmdBufferIndex,
					};

					vkCmdPushConstants(cmd, pipeline.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

					// bind the descriptor set.
					auto set = mDescriptorSet.getDescriptorSet().first;
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, mBindings.descriptorSet, 1, &set, 0, nullptr);

					mVkCmdDrawMeshTasksIndirectEXT(
						cmd,
						mVisabilityBuffer.getBuffer().buffer,
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

	error meshletRenderer::accumilationPass(VkCommandBuffer cmd, renderer::renderParams in, const swapChain& sChain)
	{
		// Add image barier, need to wait for opaque pass to finish for early depth test in accumilation pass.
		pipelineImageBarrier(
			cmd,
			sChain.getDepthImage(false),
			sChain.getDepthImageFormat(),
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
			sChain.getAccumImageView(false),
			mPreset.msaa <= 1 ? nullptr : sChain.getAccumImageView(true),
			getResolveMode(mPreset.msaa),
			&clear,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		clear.color = VkClearColorValue{ 1.0f, 0.0f, 0.0f, 1.0f };

		VkRenderingAttachmentInfo revealAttachment = attachmentInfo(
			sChain.getRevealImageView(false),
			mPreset.msaa <= 1 ? nullptr : sChain.getRevealImageView(true),
			getResolveMode(mPreset.msaa),
			&clear,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		std::vector<VkRenderingAttachmentInfo> colorAttachments = { accumAttachment, revealAttachment };

		VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(sChain.getDepthImageView(false), mPreset.msaa <= 1 ? nullptr : sChain.getDepthImageView(true), getResolveMode(mPreset.msaa), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, false);

		VkRenderingInfo renderInfo = renderingInfo(sChain.getDrawImageExtent(), colorAttachments, &depthAttachment);

		auto blendingPipeline = mAccumilationPipeline.getPipelineRenderData();

		// Nothing to render.
		if (blendingPipeline.cmdBufferCount == 0)
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
				.cmdBufferCount = blendingPipeline.cmdBufferCount,
				.cullStage = ACCUMILATION_PASS_FLAG_BIT,
				.hzbLength = uint32_t(sChain.getHzbSize()),
			}
			);
		if (err)
			return err;

		VkBuffer cmdBuf = mAccumilationPipeline.getBuffer().getBuffer().buffer;
		uint32_t cmdBufSize = uint32_t(mAccumilationPipeline.getBuffer().getLoadedBytes());

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
			cmd, mVisabilityBuffer.getBuffer().buffer,
			VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
			VK_PIPELINE_STAGE_2_CLEAR_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			uint32_t(sizeof(uint32_t) * 2),
			0
		);

		vkCmdFillBuffer(cmd, mVisabilityBuffer.getBuffer().buffer, 0, sizeof(uint32_t) * 2, 0u);

		pipelineBufferBarier(
			cmd, mVisabilityBuffer.getBuffer().buffer,
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
				.cmdBufferCount = blendingPipeline.cmdBufferCount,
				.stage = ACCUMILATION_PASS_FLAG_BIT,
				.compactRule = VISIBLE_FIRST_PASS_FLAG_BIT,
			}
			);
		if (err)
			return err;

		pipelineBufferBarier(
			cmd,
			mVisabilityBuffer.getBuffer().buffer,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
			VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
			uint32_t(mVisabilityBuffer.getSize()),
			0
		);

		pipelineBufferBarier(
			cmd,
			mVisabilityBuffer.getBuffer().buffer,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
			VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
			uint32_t(mVisabilityBuffer.getLoadedBytes()),
			0
		);

		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipeline.pipeline);

		pushConstants pc{
			.cmdBufferCount = blendingPipeline.cmdBufferCount,
		};

		vkCmdPushConstants(cmd, blendingPipeline.pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

		// bind the descriptor set.
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipeline.pipelineLayout, mBindings.descriptorSet, 1, &set, 0, nullptr);

		mVkCmdDrawMeshTasksIndirectEXT(
			cmd,
			mVisabilityBuffer.getBuffer().buffer,
			sizeof(uint32_t),
			1,
			12
		);

		vkCmdEndRendering(cmd);

		return {};
	}

	error meshletRenderer::compositePass(VkCommandBuffer cmd, renderer::renderParams in, const swapChain& sChain)
	{
		// Composite opaque and transperent.
		VkRenderingAttachmentInfo colorAttachment = attachmentInfo(sChain.getDrawImageView(false), mPreset.msaa <= 1 ? nullptr : sChain.getDrawImageView(true), getResolveMode(mPreset.msaa), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		std::vector<VkRenderingAttachmentInfo> colorAttachments = { colorAttachment };

		VkRenderingInfo renderInfo = renderingInfo(sChain.getDrawImageExtent(), colorAttachments, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		auto blendingPipeline = mCompositePipeline.getPipelineRenderData();

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipeline.pipeline);

		// bind the descriptor set.
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipeline.pipelineLayout, mBindings.descriptorSet, 1, &set, 0, nullptr);

		mVkCmdDrawMeshTasksEXT(cmd, 1, 1, 1);

		vkCmdEndRendering(cmd);

		return {};
	}

	error meshletRenderer::addToRender(VkDevice device, VmaAllocator allocator, submit& is, const swapChain& sChain, const model& m)
	{
		auto meshShader = mCtx->mAmanager->getDefaultMeshShader();
		if (!meshShader)
			return meshShader.err();

		auto taskShader = mCtx->mAmanager->getDefaultTaskShader();
		if (!taskShader)
			return taskShader.err();

		if (mPipelines.find(m.mat.pixelShader->hash()) == mPipelines.end())
		{
			pipelineData pipeline{};

			error err = pipeline.init(
				device,
				allocator,
				is,
				m.mat.pixelShader,
				meshShader.value(),
				taskShader.value(),
				{ mDescriptorSet.getDescriptorSet().second },
				sChain.getDepthImageFormat(),
				{ sChain.getDrawImageFormat() },
				sampleCounts(mPreset.msaa),
				pipelineData::pipelineType::opaque
			);
			if (err)
				return err;

			mPipelines[m.mat.pixelShader->hash()] = pipeline;

			mDeletionQueue.addDestroyTask(destroyTask{ .type = pipeData, .pipeData = &mPipelines[m.mat.pixelShader->hash()] });
		}

		if (mPipelines[m.mat.pixelShader->hash()].instanceExists(m.id))
			return {};

		// Upload material.
		auto materialOffsets = mMaterialRegistry.addMaterials(m.mat);
		if (!materialOffsets)
			return materialOffsets.err();

		perInstanceAttr attr = m.instanceAttributes;
		attr.jointIndex = std::numeric_limits<uint32_t>::max();
		attr.jointOffset = std::numeric_limits<uint32_t>::max();

		// Upload animation data.
		if (m.anims.jointMatrices && m.anims.jointMatrices->size() != 0)
		{
			auto jointHandle = mJointRegistry.addBlock(m.id, m.anims.jointMatrices->data(), m.anims.jointMatrices->size() * sizeof(glm::mat4), is);
			if (!jointHandle)
				return jointHandle.err();

			attr.jointIndex = jointHandle.value().bufferIndex;
			attr.jointOffset = jointHandle.value().offset / uint32_t(sizeof(glm::mat4));
		}

		attr.globalMaterialOffset = materialOffsets.value();

		auto perInstanceHandle = mPerInstanceRegistry.addBlock(
			m.id,
			&attr,
			sizeof(perInstanceAttr),
			is
		);
		if (!perInstanceHandle)
			return perInstanceHandle.err();

		pipelineData::addInstanceParams addParams{
			.pixelShaderID = m.mat.pixelShader->hash(),
			.instanceID = m.id,
			.perInstanceHandle = perInstanceHandle.value(),
			.meshesData = {},
			.isBlendGeometry = m.mat.hasBlendMaterials(),
		};

		for (int i = 0; i < m.meshData->size(); i++)
		{
			const mesh& crntMesh = m.meshData->operator[](i);
			perMeshAttributes crntMeshAttrs = m.perMeshData->operator[](i);

			bufferHandle vertexHandle{};
			// Upload common vertex attributes.
			auto handle = mPositionRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.positions.data(),
				crntMesh.positions.size() * sizeof(glm::vec4),
				is
			);
			if (!handle)
				return handle.err();

			handle = mNormalRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.normal.data(),
				crntMesh.normal.size() * sizeof(glm::vec4),
				is
			);
			if (!handle)
				return handle.err();

			handle = mTangentRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.tangent.data(),
				crntMesh.tangent.size() * sizeof(glm::vec4),
				is
			);
			if (!handle)
				return handle.err();

			vertexHandle = handle.value();

			bufferHandle weightHandle{};
			// Upload skin data if needed.
			if (crntMeshAttrs.isSkinned && !crntMesh.jointIndices.empty() && !crntMesh.weights.empty())
			{
				handle = mJointIndexRegistry.addBlock(
					crntMesh.meshHash,
					crntMesh.jointIndices.data(),
					crntMesh.jointIndices.size() * sizeof(glm::uvec4),
					is
				);
				if (!handle)
					return handle.err();

				handle = mWeightRegistry.addBlock(
					crntMesh.meshHash,
					crntMesh.weights.data(),
					crntMesh.weights.size() * sizeof(glm::vec4),
					is
				);
				if (!handle)
					return handle.err();

				weightHandle = handle.value();
			}

			auto perMeshHandle = mPerMeshRegistry.addBlock(
				crntMesh.meshHash,
				&crntMeshAttrs,
				sizeof(perMeshAttributes),
				is
			);
			if (!perMeshHandle)
				return perMeshHandle.err();

			std::vector<meshlet> meshlets = crntMesh.meshlets.data;

			auto indexHandle = mIndexRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.indices.data.data(),
				crntMesh.indices.data.size() * sizeof(uint32_t),
				is
			);
			if (!handle)
				return handle.err();

			auto primitivesHandle = mPrimitiveRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.primitives.data.data(),
				crntMesh.primitives.data.size() * sizeof(uint32_t),
				is
			);
			if (!handle)
				return handle.err();

			for (auto& m : meshlets)
			{
				// Set offset + index for vertex attribs.
				m.vertexBufferOffset = vertexHandle.offset / sizeof(glm::vec4);
				m.vertexBufferIndex = vertexHandle.bufferIndex;
				m.perMeshBufferOffset = perMeshHandle.value().offset / uint32_t(sizeof(perMeshAttributes));
				m.perMeshBufferIndex = perMeshHandle.value().bufferIndex;
				// Set offset + index for vertex anim attribs.
				m.weightBufferOffset = weightHandle.offset / sizeof(glm::vec4);
				m.weightBufferIndex = weightHandle.bufferIndex;

				m.triangleBufferOffset += primitivesHandle.value().offset / uint32_t(sizeof(uint32_t));
				m.triangleBufferIndex = primitivesHandle.value().bufferIndex;

				m.indexBufferOffset += indexHandle.value().offset / uint32_t(sizeof(uint32_t));
				m.indexBufferIndex = indexHandle.value().bufferIndex;
			}

			handle = mMeshletRegistry.addBlock(
				crntMesh.meshHash,
				meshlets.data(),
				meshlets.size() * sizeof(meshlet),
				is
			);
			if (!handle)
				return handle.err();

			addParams.meshesData.push_back(
				pipelineData::meshes{
					.meshID = crntMesh.meshHash,
					.meshHandle = perMeshHandle.value(),
					.meshletHandle = handle.value(),
					.meshlets = crntMesh.meshlets,
				}
				);
		}

		if (addParams.isBlendGeometry)
		{
			error err = mAccumilationPipeline.addInstance(addParams);
			if (err)
				return err;
		}

		error err = mPipelines[m.mat.pixelShader->hash()].addInstance(addParams);
		if (err)
			return err;

		return {};
	}

	error meshletRenderer::updateInstance(const model& m, submit& is)
	{
		// Get material offsets or upload as new.
		auto materialOffsets = mMaterialRegistry.getMaterialsOffset(m.mat);
		if (!materialOffsets)
		{
			materialOffsets = mMaterialRegistry.addMaterials(m.mat);
			if (!materialOffsets)
				return materialOffsets.err();
		}

		// Form new instance attrs.
		perInstanceAttr attr = m.instanceAttributes;
		attr.globalMaterialOffset = materialOffsets.value();

		auto jointHandle = mJointRegistry.findBlock(m.id);
		if (jointHandle)
		{
			attr.jointIndex = jointHandle.value().bufferIndex;
			attr.jointOffset = jointHandle.value().offset / uint32_t(sizeof(glm::mat4));
		}

		return mPerInstanceRegistry.updateBlock(m.id, &attr, sizeof(perInstanceAttr), is);
	}

	error meshletRenderer::updateAnimations(const model& m, submit& is)
	{
		// Update animation data.
		return mJointRegistry.updateBlock(m.id, m.anims.jointMatrices->data(), m.anims.jointMatrices->size() * sizeof(glm::mat4), is);
	}

	void meshletRenderer::removeFromRender(const model& m)
	{
		auto pipeline = mPipelines.find(m.mat.pixelShader->hash());

		if (pipeline == mPipelines.end())
			return;

		mPerInstanceRegistry.deleteBlock(m.id);

		for (int i = 0; i < m.meshData->size(); i++)
		{
			const mesh& crntMesh = m.meshData->operator[](i);
			const perMeshAttributes crntMeshAttrs = m.perMeshData->operator[](i);

			// Remove instance.
			pipeline->second.removeInstance(
				pipelineData::removeInstanceParams{
					.pixelShaderID = m.mat.pixelShader->hash(),
					.instanceID = m.id,
					.meshID = crntMesh.meshHash,
				}
				);

			mAccumilationPipeline.removeInstance(
				pipelineData::removeInstanceParams{
					.pixelShaderID = m.mat.pixelShader->hash(),
					.instanceID = m.id,
					.meshID = crntMesh.meshHash,
				}
				);

			// Remove animation data.
			mJointRegistry.deleteBlock(m.id);

			// Mesh isn't used.
			if (!pipeline->second.meshIsUsed(crntMesh.meshHash))
			{
				mPositionRegistry.deleteBlock(crntMesh.meshHash);

				mNormalRegistry.deleteBlock(crntMesh.meshHash);

				mTangentRegistry.deleteBlock(crntMesh.meshHash);

				if (crntMeshAttrs.isSkinned)
				{
					mJointIndexRegistry.deleteBlock(crntMesh.meshHash);
					mWeightRegistry.deleteBlock(crntMesh.meshHash);
				}

				mIndexRegistry.deleteBlock(crntMesh.meshHash);

				mPrimitiveRegistry.deleteBlock(crntMesh.meshHash);

				mMeshletRegistry.deleteBlock(crntMesh.meshHash);

				mMaterialRegistry.deleteMaterials(m.mat);

				mPerMeshRegistry.deleteBlock(crntMesh.meshHash);
			}
		}
	}

	error meshletRenderer::updateDescriptors(renderer::renderParams in, submit& is, VkDevice device, VmaAllocator allocator)
	{
		// Update command buffer for mesh pipeline.
		for (auto& [_, p] : mPipelines)
		{
			error err = p.updateCommandBuffer(
				pipelineData::updateCommandBufferParams{
					.device = device,
					.allocator = allocator,
					.is = is,
				}
				);
			if (err)
				return err;
		}

		error err = mAccumilationPipeline.updateCommandBuffer(
			pipelineData::updateCommandBufferParams{
				.device = device,
				.allocator = allocator,
				.is = is,
			}
			);
		if (err)
			return err;


		bool needUpdate = false;

		for (auto& [_, p] : mPipelines)
			needUpdate |= p.needDescriptorUpdate();

		// update cmd opaque buffer.
		if (needUpdate)
		{
			std::vector<VkDescriptorBufferInfo> buffersInfo{};
			for (auto& [_, p] : mPipelines)
			{
				auto info = p.getBufferInfo();
				buffersInfo.insert(buffersInfo.end(), info.begin(), info.end());
			}

			auto writeInfo = descriptorSet::getWriteInfo(mBindings.cmdOpaqueBufferBinding, buffersInfo);

			mDescriptorSet.updateWrite(writeInfo);

			error err = mComputeRenderer.updateOpaqueCmdBufferDescriptors(buffersInfo);
			if (err)
				return err;

			for (auto& [_, p] : mPipelines)
				p.setUpdated();
		}

		// update cmd accumilation buffer.
		if (mAccumilationPipeline.needDescriptorUpdate())
		{
			std::vector<VkDescriptorBufferInfo> buffersInfo{};

			auto info = mAccumilationPipeline.getBufferInfo();
			buffersInfo.insert(buffersInfo.end(), info.begin(), info.end());

			auto writeInfo = descriptorSet::getWriteInfo(mBindings.cmdAccumilationBufferBinding, buffersInfo);

			mDescriptorSet.updateWrite(writeInfo);

			error err = mComputeRenderer.updateAccumilationCmdBufferDescriptors(buffersInfo);
			if (err)
				return err;

			mAccumilationPipeline.setUpdated();
		}

		if (mPositionRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPositionRegistry.getWriteInfo(mBindings.positionsBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mPositionRegistry.setUpdated();
		}

		if (mNormalRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mNormalRegistry.getWriteInfo(mBindings.normalBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mNormalRegistry.setUpdated();
		}

		if (mTangentRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mTangentRegistry.getWriteInfo(mBindings.tangentBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mTangentRegistry.setUpdated();
		}

		if (mJointIndexRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mJointIndexRegistry.getWriteInfo(mBindings.jointIndexBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mJointIndexRegistry.setUpdated();
		}

		if (mWeightRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mWeightRegistry.getWriteInfo(mBindings.wightBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mWeightRegistry.setUpdated();
		}

		if (mIndexRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mIndexRegistry.getWriteInfo(mBindings.indexBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mIndexRegistry.setUpdated();
		}

		if (mPrimitiveRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPrimitiveRegistry.getWriteInfo(mBindings.primitiveBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mPrimitiveRegistry.setUpdated();
		}

		if (mMeshletRegistry.needDescriptorUpdate())
		{
			auto bufInfo = mMeshletRegistry.getBufferInfo();

			error err = mComputeRenderer.updateMeshletBufferDescriptors(bufInfo);
			if (err)
				return err;

			auto writeInfo = mMeshletRegistry.getWriteInfo(mBindings.meshletBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mMeshletRegistry.setUpdated();
		}

		if (mPerInstanceRegistry.needDescriptorUpdate())
		{
			auto bufInfo = mPerInstanceRegistry.getBufferInfo();

			error err = mComputeRenderer.updatePerInstancetBufferDescriptors(bufInfo);
			if (err)
				return err;

			auto writeInfo = mPerInstanceRegistry.getWriteInfo(mBindings.perInstanceBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mPerInstanceRegistry.setUpdated();
		}

		if (mJointRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mJointRegistry.getWriteInfo(mBindings.jointsBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mJointRegistry.setUpdated();
		}

		if (mPerMeshRegistry.needDescriptorUpdate())
		{
			auto bufInfo = mPerMeshRegistry.getBufferInfo();

			error err = mComputeRenderer.updatePerMeshBufferDescriptors(bufInfo);
			if (err)
				return err;

			auto writeInfo = mPerMeshRegistry.getWriteInfo(mBindings.perMeshBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mPerMeshRegistry.setUpdated();
		}

		// Update materials.
		if (mMaterialRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mMaterialRegistry.getWriteInfo(mBindings.materialArrayBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mMaterialRegistry.setUpdated();
		}

		// Resize visability buffer if needed.
		if (uint32_t max = getMaxCmdBufferSize(); max > mVisabilityBufferSize)
		{
			mVisabilityBuffer.destroy();

			mVisabilityBufferSize = uint32_t(1.5f * max);

			std::vector<uint32_t> visDispatch{ 0, 0, 1, 1 };

			error err = mVisabilityBuffer.build(is, visDispatch.data(), mVisabilityBufferSize, sizeof(uint32_t) * 4, true);
			if (err)
				return err;

			std::vector<VkDescriptorBufferInfo> bufferInfo = {
					VkDescriptorBufferInfo{.buffer = mVisabilityBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE}
			};

			err = mComputeRenderer.updateVisabilityBufferDescriptors(bufferInfo);
			if (err)
				return err;

			auto writeInfo = descriptorSet::getWriteInfo(mBindings.visabilityBuffer, bufferInfo);
			mDescriptorSet.updateWrite(writeInfo);
		}

		return {};
	}
}
