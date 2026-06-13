#include <pch.h>
#include "meshletRenderer.h"
#include "renderer.h"
#include "computeRenderer.h"

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
			.totalDescriptorsCount = 17,

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
			.perDrawBufferUboBinding = 19,

			// Materials.
			.materialArrayBinding = 21,
			.accumBinding = 22,
			.revealBinding = 23,
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
		mPositionRegistry.init(device, allocator);
		mNormalRegistry.init(device, allocator);
		mTangentRegistry.init(device, allocator);
		mJointIndexRegistry.init(device, allocator);
		mWeightRegistry.init(device, allocator);

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

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mPositionRegistry});
		
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

		mDeletionQueue.addDestroyTask(destroyTask{ .type = pipelineReg, .pipelineReg = &mPipelineRegistry });

		auto samp = descriptorSet::createSampler(device, float(mPreset.anisotropicFiltering));
		if (!samp)
			return samp.err();

		mSampler = samp.value();

		mDeletionQueue.addDestroyTask(destroyTask{ .type = sampler, .sampler = &mSampler });

		auto defaultMat = mCtx->mAmanager->loadDetaultMaterial();
		if (!defaultMat)
			return defaultMat.err();

		err = mMaterialRegistry.init(mSampler, defaultMat.value());
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = matReg, .matReg = &mMaterialRegistry });

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
		const uint32_t bufferObjects = 13;
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
		info.front().imageView = sChain.getAccumImageView(mPreset.msaa > 1);

		std::vector<VkWriteDescriptorSet> wSet = descriptorSet::getWriteInfo(mBindings.accumBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info);

		mDescriptorSet.updateWrite(wSet);

		info.front().sampler = mSampler;
		info.front().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.front().imageView = sChain.getRevealImageView(mPreset.msaa > 1);

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

		error err = mPipelineRegistry.initAccumilatePipeline(
			device,
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mDescriptorSet.getDescriptorSet().second },
			sChain.getDepthImageFormat(),
			{ sChain.getAccumImageFormat(), sChain.getRevealImageFormat() },
			mPreset
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

		err = mPipelineRegistry.initCompositePipeline(
			device,
			pixel.value(),
			meshlets.value(),
			task.value(),
			{ mDescriptorSet.getDescriptorSet().second },
			sChain.getDepthImageFormat(),
			{ sChain.getDrawImageFormat() },
			mPreset
		);
		if (err)
			return err;

		return {};
	}

	error meshletRenderer::opaquePass(VkCommandBuffer cmd, renderer::renderCallIn in)
	{
		auto pipelinesMappings = mPipelineRegistry.getOpaquePipelines();

		for (auto& v : pipelinesMappings)
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

	error meshletRenderer::accumilationPass(VkCommandBuffer cmd, renderer::renderCallIn in)
	{
		auto blendingPipeline = mPipelineRegistry.getAccumilationPipeline();

		// Nothing to render.
		if (blendingPipeline.cmdPipelineEndOffset == 0)
			return {};

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipeline.pipeline);

		pushConstants pc{
			.commandBufferOffset = blendingPipeline.cmdPipelineStartOffset,
			.meshletCount = blendingPipeline.cmdPipelineEndOffset - blendingPipeline.cmdPipelineStartOffset,
		};

		vkCmdPushConstants(cmd, blendingPipeline.layout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

		// bind the descriptor set.
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipeline.layout, mBindings.descriptorSet, 1, &set, 0, nullptr);

		mVkCmdDrawMeshTasksEXT(cmd, uint32_t(pc.meshletCount) / mCtx->config.inner.render.shaderWorkGroup + 1, 1, 1);

		return {};
	}

	error meshletRenderer::compositePass(VkCommandBuffer cmd, renderer::renderCallIn in)
	{
		auto blendingPipeline = mPipelineRegistry.getCompositePipeline();

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipeline.getPipeline().first);

		// bind the descriptor set.
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, blendingPipeline.getPipeline().second, mBindings.descriptorSet, 1, &set, 0, nullptr);

		mVkCmdDrawMeshTasksEXT(cmd, 1, 1, 1);

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
				pipelineRegistry::meshes{
					.meshID = crntMesh.meshHash,
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

		for (int i = 0; i < m.meshData->size(); i++)
		{
			const mesh& crntMesh = m.meshData->operator[](i);
			const perMeshAttributes crntMeshAttrs = m.perMeshData->operator[](i);

			// Remove instance.
			mPipelineRegistry.removeInstance(m.mat.pixelShader->hash(), m.id, crntMesh.meshHash);

			// Remove animation data.
			mJointRegistry.deleteBlock(m.id);

			// Mesh isn't used.
			if (!mPipelineRegistry.meshIsUsed(crntMesh.meshHash))
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

	error meshletRenderer::updateDescriptors(renderer::renderCallIn in, submit& is)
	{
		// Update command buffer for mesh pipeline.
		error err = mPipelineRegistry.updateOpaqueCmdBuffer(is);
		if (err)
			return err;

		err = mPipelineRegistry.updateAccumilationCmdBuffer(is);
		if (err)
			return err;

		// update cmd opaque buffer.
		if (mPipelineRegistry.needOpaqueDescriptorUpdate())
		{
			auto writeInfo = mPipelineRegistry.getOpaqueCmdBufferWriteInfo(mBindings.cmdOpaqueBufferBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mPipelineRegistry.setOpaqueUpdated();
		}

		// update cmd accumilation buffer.
		if (mPipelineRegistry.needAccumilationDescriptorUpdate())
		{
			auto writeInfo = mPipelineRegistry.getAccumilationCmdBufferWriteInfo(mBindings.cmdAccumilationBufferBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mPipelineRegistry.setAccumilationUpdated();
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
