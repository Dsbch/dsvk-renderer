#include <pch.h>
#include "meshletRenderer.h"
#include "renderer.h"

namespace engine
{
	error meshletRenderer::init(
		std::shared_ptr<context> ctx,
		PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT,
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

		mCtx = ctx;

		mBindings = meshletBindings{
			.descriptorSet = 0,
			.totalDescriptorsCount = 13,

			.accumBinding = 11,
			.revealBinding = 12,
			.vertexBinding = 0,
			.animVertexBinding = 1,
			.perInstanceBinding = 2,
			.meshletCmdBinding = 3,
			.indexBinding = 4,
			.primitiveBinding = 5,
			.meshletBinding = 6,
			.jointsBinding = 7,
			.perMeshBinding = 8,

			.perDrawBufferUboBinding = 9,

			.materialArrayBinding = 10,
		};

		mDeletionQueue.init(device);

		mPreset = preset;

		error err = initRegistry(device, allocator, is);
		if (err)
			return err;

		err = initDescriptors(device, physicalDevice, limits, UBObuffer);
		if (err)
			return err;

		err = initBlendingPipelines(device, sChain);
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

		return {};
	}

	error meshletRenderer::initRegistry(VkDevice device, VmaAllocator allocator, submit& is)
	{
		mVertexRegistry.init(device, allocator);

		mAnimVertexRegistry.init(device, allocator);

		mIndexRegistry.init(device, allocator);

		mPrimitiveRegistry.init(device, allocator);

		mMeshletRegistry.init(device, allocator);

		mPerMeshRegistry.init(device, allocator);

		error err = mPipelineRegistry.init(device, allocator, is);
		if (err)
			return err;

		// Updated each frame used as MAPPED.
		mPerInstanceRegistry.init(device, allocator, true);

		mJointRegistry.init(device, allocator, true);

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mVertexRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mIndexRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mPrimitiveRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mMeshletRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mPerInstanceRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mJointRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mPerMeshRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mAnimVertexRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = pipelineReg, .pipelineReg = &mPipelineRegistry });

		auto samp = descriptorSet::createSampler(device, float(mPreset.anisotropicFiltering));
		if (!samp)
		{
			return samp.err();
		}

		mSampler = samp.value();

		mDeletionQueue.addDestroyTask(destroyTask{ .type = sampler, .sampler = &mSampler });

		err = mMaterialRegistry.init(mSampler);
		if (err)
			return err;

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
		const uint32_t bufferObjects = 9;
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

		// add bindings for buffers.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.vertexBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.animVertexBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perInstanceBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.meshletCmdBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
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

		mDeletionQueue.addDestroyTask(destroyTask{ .type = descSet, .descSet = &mDescriptorSet });

		return {};
	}

	error meshletRenderer::updateSwapchainDependentDescriptors(const swapChain& sChain)
	{
		std::vector<VkDescriptorImageInfo> info{ VkDescriptorImageInfo{} };
		info.front().sampler = mSampler;
		info.front().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.front().imageView = mPreset.msaa <= 1 ? sChain.getAccumImageView() : sChain.getAccumResolveImageView();

		std::vector<VkWriteDescriptorSet> wSet = descriptorSet::getWriteInfo(mBindings.accumBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info);

		mDescriptorSet.updateWrite(wSet);

		info.front().sampler = mSampler;
		info.front().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.front().imageView = mPreset.msaa <= 1 ? sChain.getRevealImageView() : sChain.getRevealResolveImageView();

		wSet = descriptorSet::getWriteInfo(mBindings.revealBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info);

		mDescriptorSet.updateWrite(wSet);

		return {};
	}

	error meshletRenderer::initBlendingPipelines(VkDevice device, const swapChain& sChain)
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

		error err = mPipelineRegistry.createPipeline(
			device,
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mDescriptorSet.getDescriptorSet().second },
			sChain.getDepthImageFormat(),
			{ sChain.getAccumImageFormat(), sChain.getRevealImageFormat() },
			mPreset,
			true
		);
		if (err)
			return err;

		meshlets = mCtx->mAmanager->getDefaultCompositeMeshShader();
		if (!meshlets)
			return meshlets.err();

		task = mCtx->mAmanager->getDefaultCompositeTaskShader();
		if (!task)
			return task.err();

		pixel = mCtx->mAmanager->getDefaultCompositePixelShader();
		if (!pixel)
			return pixel.err();

		err = mPipelineRegistry.createPipeline(
			device,
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mDescriptorSet.getDescriptorSet().second },
			sChain.getDepthImageFormat(),
			{ sChain.getDrawImageFormat() },
			mPreset,
			false,
			true
		);
		if (err)
			return err;

		return {};
	}

	error meshletRenderer::drawOpaqueGeometry(VkCommandBuffer cmd, renderer::renderCallIn in)
	{
		auto pipelines = mPipelineRegistry.getOpaquePipelines();

		for (auto& [_, v] : pipelines)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, v.pipeline);

			pushConstants pc{
				.commandBufferOffset = v.cmdPipelineStartOffset,
				.meshletCount = v.cmdPipelineEndOffset - v.cmdPipelineStartOffset,
			};

			vkCmdPushConstants(cmd, v.layout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

			// bind the descriptor set.
			auto set = mDescriptorSet.getDescriptorSet().first;
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, v.layout, mBindings.descriptorSet, 1, &set, 0, nullptr);

			mVkCmdDrawMeshTasksEXT(cmd, uint32_t(pc.meshletCount) / mCtx->config.inner.render.shaderWorkGroup + 1, 1, 1);
		}

		return {};
	}

	error meshletRenderer::drawTransperentGeometry(VkCommandBuffer cmd, renderer::renderCallIn in)
	{
		auto blendingPipelines = mPipelineRegistry.getBlendPipelines();

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipelines.first.pipeline);

		pushConstants pc{
			.commandBufferOffset = blendingPipelines.first.cmdPipelineStartOffset,
			.meshletCount = blendingPipelines.first.cmdPipelineEndOffset - blendingPipelines.first.cmdPipelineStartOffset,
		};

		vkCmdPushConstants(cmd, blendingPipelines.first.layout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

		// bind the descriptor set.
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipelines.first.layout, mBindings.descriptorSet, 1, &set, 0, nullptr);

		mVkCmdDrawMeshTasksEXT(cmd, uint32_t(pc.meshletCount) / mCtx->config.inner.render.shaderWorkGroup + 1, 1, 1);

		return {};
	}

	error meshletRenderer::compositeOpaqueAndTransperent(VkCommandBuffer cmd, renderer::renderCallIn in)
	{
		auto blendingPipelines = mPipelineRegistry.getBlendPipelines();

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipelines.second.pipeline.getPipeline().first);

		// bind the descriptor set.
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipelines.second.pipeline.getPipeline().second, mBindings.descriptorSet, 1, &set, 0, nullptr);

		mVkCmdDrawMeshTasksEXT(cmd, 1, 1, 1);

		return {};
	}

	error vulkanRenderer::drawUI(VkCommandBuffer cmd)
	{
		// Imgui can't work with msaa color attachments.
		VkRenderingAttachmentInfo colorAttachment = attachmentInfo(mPreset.msaa <= 1 ? mSwapChain.getDrawImageView() : mSwapChain.getResolveImageView(), nullptr, VK_RESOLVE_MODE_NONE, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		std::vector<VkRenderingAttachmentInfo> colorAttachments = { colorAttachment };

		VkRenderingInfo renderInfo = renderingInfo(mSwapChain.getDrawImageExtent(), colorAttachments, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		// Draw UI.
		mUi.onRender(cmd);

		vkCmdEndRendering(cmd);

		return {};
	}

	error meshletRenderer::addToRender(VkDevice device, submit& is, const swapChain& sChain, const model& m)
	{
		auto meshShader = mCtx->mAmanager->getDefaultMeshShader();
		if (!meshShader)
			return meshShader.err();

		auto taskShader = mCtx->mAmanager->getDefaultTaskShader();
		if (!taskShader)
			return taskShader.err();

		error err = mPipelineRegistry.createPipeline(
			device,
			m.mat.pixelShader,
			meshShader.value(),
			taskShader.value(),
			{ mDescriptorSet.getDescriptorSet().second },
			sChain.getDepthImageFormat(),
			{ sChain.getDrawImageFormat() },
			mPreset
		);
		if (err)
			return err;

		if (mPipelineRegistry.instanceExists(m.id))
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

		pipelineRegistry::addInstanceParams addParams{
			.pixelShaderID = m.mat.pixelShader->hash(),
			.instanceID = m.id,
			.perInstanceHandle = perInstanceHandle.value(),
			.meshesData = {},
		};

		for (int i = 0; i < m.meshData->size(); i++)
		{
			const mesh& crntMesh = m.meshData->operator[](i);
			const perMeshAttributes crntMeshAttrs = m.perMeshData->operator[](i);

			// Upload geometry.
			uint32_t vertexSize = crntMeshAttrs.isSkinned ? uint32_t(sizeof(animVertex)) : uint32_t(sizeof(vertex));

			bufferHandle vertexHandle{};
			if (crntMeshAttrs.isSkinned)
			{
				auto handle = mAnimVertexRegistry.addBlock(
					crntMesh.hash,
					crntMesh.animVertices.data(),
					crntMesh.animVertices.size() * vertexSize,
					is
				);
				if (!handle)
					return handle.err();

				vertexHandle = handle.value();
			}
			else
			{
				auto handle = mVertexRegistry.addBlock(
					crntMesh.hash,
					crntMesh.vertices.data(),
					crntMesh.vertices.size() * vertexSize,
					is
				);
				if (!handle)
					return handle.err();

				vertexHandle = handle.value();
			}

			auto perMeshHandle = mPerMeshRegistry.addBlock(
				crntMesh.hash,
				&crntMeshAttrs,
				sizeof(perMeshAttributes),
				is
			);
			if (!perMeshHandle)
				return perMeshHandle.err();

			std::vector<meshlet> meshlets = crntMesh.meshlets.data;

			for (auto& m : meshlets)
			{
				m.vertexBufferOffset += vertexHandle.offset / vertexSize;
				m.vertexBufferIndex = vertexHandle.bufferIndex;
				m.perMeshBufferOffset = perMeshHandle.value().offset / uint32_t(sizeof(perMeshAttributes));
				m.perMeshBufferIndex = perMeshHandle.value().bufferIndex;
			}

			auto handle = mIndexRegistry.addBlock(
				crntMesh.hash,
				crntMesh.indices.data.data(),
				crntMesh.indices.data.size() * sizeof(uint32_t),
				is
			);
			if (!handle)
				return handle.err();

			for (auto& m : meshlets)
			{
				m.indexBufferOffset += handle.value().offset / uint32_t(sizeof(uint32_t));
				m.indexBufferIndex = handle.value().bufferIndex;
			}

			handle = mPrimitiveRegistry.addBlock(
				crntMesh.hash,
				crntMesh.primitives.data.data(),
				crntMesh.primitives.data.size() * sizeof(uint32_t),
				is
			);
			if (!handle)
				return handle.err();

			for (auto& m : meshlets)
			{
				m.triangleBufferOffset += handle.value().offset / uint32_t(sizeof(uint32_t));
				m.triangleBufferIndex = handle.value().bufferIndex;
			}

			handle = mMeshletRegistry.addBlock(
				crntMesh.hash,
				meshlets.data(),
				meshlets.size() * sizeof(meshlet),
				is
			);
			if (!handle)
				return handle.err();

			addParams.meshesData.push_back(pipelineRegistry::meshes{
					.meshID = crntMesh.hash,
					.meshletHandle = handle.value(),
					.meshlets = crntMesh.meshlets,
				}
				);
		}

		err = mPipelineRegistry.addInstance(addParams);
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
		mPerInstanceRegistry.deleteBlock(m.id);

		for (auto& crntMesh : *m.meshData.get())
		{
			// Remove instance.
			mPipelineRegistry.removeInstance(m.mat.pixelShader->hash(), m.id, crntMesh.hash);

			// Remove animation data.
			mJointRegistry.deleteBlock(m.id);

			// Mesh isn't used.
			if (!mPipelineRegistry.meshIsUsed(crntMesh.hash))
			{
				mVertexRegistry.deleteBlock(crntMesh.hash);

				mAnimVertexRegistry.deleteBlock(crntMesh.hash);

				mIndexRegistry.deleteBlock(crntMesh.hash);

				mPrimitiveRegistry.deleteBlock(crntMesh.hash);

				mMeshletRegistry.deleteBlock(crntMesh.hash);

				mMaterialRegistry.deleteMaterials(m.mat);

				mPerMeshRegistry.deleteBlock(crntMesh.hash);
			}
		}
	}

	error meshletRenderer::updateDescriptors(renderer::renderCallIn in, submit& is)
	{
		// Update command buffer for mesh pipeline.
		error err = mPipelineRegistry.updateCommandBuffer(is);
		if (err)
			return err;

		// update buffers.
		if (mPipelineRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPipelineRegistry.getWriteInfo(mBindings.meshletCmdBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mPipelineRegistry.setUpdated();
		}

		if (mVertexRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mVertexRegistry.getWriteInfo(mBindings.vertexBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mVertexRegistry.setUpdated();
		}

		if (mAnimVertexRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mAnimVertexRegistry.getWriteInfo(mBindings.animVertexBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mAnimVertexRegistry.setUpdated();
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
			auto writeInfo = mMeshletRegistry.getWriteInfo(mBindings.meshletBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mMeshletRegistry.setUpdated();
		}

		if (mPerInstanceRegistry.needDescriptorUpdate())
		{
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

		return {};
	}
}
