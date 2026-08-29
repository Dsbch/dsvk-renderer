#include <pch.h>
#include "resourceManager.h"

namespace engine
{
	void resourceManager::init(std::shared_ptr<context> ctx, VkDevice device, graphicsPreset preset, deviceLimits limits)
	{
		mCtx = ctx;
		mPreset = preset;
		mDeviceLimits = limits;

		mBindings = {};
	
		mDeletionQueue.init(device);
	}

	error resourceManager::build(buildParams params)
	{
		error err = buildResources(params);
		if (err)
			return err;

		err = buildDescriptors(params);
		if (err)
			return err;

		return {};
	}

	void resourceManager::destroy()
	{
		mDeletionQueue.flushDeletonQueue();

		destroyViewPortDependantResources();
	}

	std::pair<VkDescriptorSet, VkDescriptorSetLayout> resourceManager::getBufferDescriptorSet() const
	{
		return mBufferDescriptorSet.getDescriptorSet();
	}

	std::pair<VkDescriptorSet, VkDescriptorSetLayout> resourceManager::getTextureDescriptorSet() const
	{
		return mTextureDescriptorSet.getDescriptorSet();
	}

	error resourceManager::updateDescriptors(updateDescriptorsParams params)
	{
		if (mLineBuffer.needDescriptorUpdate())
		{
			std::vector<VkDescriptorBufferInfo> bufferInfo{
				VkDescriptorBufferInfo{.buffer = mLineBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE}
			};

			auto writeInfo = descriptorSet::getWriteInfo(mBindings.lineBuffer, bufferInfo);
			mBufferDescriptorSet.updateWrite(writeInfo);

			mLineBuffer.setUpdated();
		}

		// Update command buffer for mesh pipeline.
		for (auto& [_, p] : mOpaqueCommandBuffers)
		{
			error err = p.updateCommandBuffer(
				commandBuffer::updateCommandBufferParams{
					.device = params.device,
					.allocator = params.allocator,
					.is = params.is,
					.frameIndex = params.frameIndex,
				}
				);
			if (err)
				return err;
		}

		error err = mAccumilationCommandBuffer.updateCommandBuffer(
			commandBuffer::updateCommandBufferParams{
				.device = params.device,
				.allocator = params.allocator,
				.is = params.is,
				.frameIndex = params.frameIndex,
			}
			);
		if (err)
			return err;

		bool needUpdate = false;

		for (auto& [_, p] : mOpaqueCommandBuffers)
			needUpdate |= p.needDescriptorUpdate();

		// update cmd opaque buffer.
		if (needUpdate)
		{
			std::vector<VkDescriptorBufferInfo> buffersInfo{};

			for (auto& [_, p] : mOpaqueCommandBuffers)
			{
				auto info = p.getBufferInfo();
				buffersInfo.insert(buffersInfo.end(), info.begin(), info.end());
			}

			auto writeInfo = descriptorSet::getWriteInfo(mBindings.cmdOpaqueBufferBinding, buffersInfo);

			mBufferDescriptorSet.updateWrite(writeInfo);

			for (auto& [_, p] : mOpaqueCommandBuffers)
				p.setUpdated();
		}

		// update cmd accumilation buffer.
		if (mAccumilationCommandBuffer.needDescriptorUpdate())
		{
			std::vector<VkDescriptorBufferInfo> buffersInfo{};

			auto info = mAccumilationCommandBuffer.getBufferInfo();
			buffersInfo.insert(buffersInfo.end(), info.begin(), info.end());

			auto writeInfo = descriptorSet::getWriteInfo(mBindings.cmdAccumilationBufferBinding, buffersInfo);

			mBufferDescriptorSet.updateWrite(writeInfo);

			mAccumilationCommandBuffer.setUpdated();
		}

		if (mPositionRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPositionRegistry.getWriteInfo(mBindings.positionsBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mPositionRegistry.setUpdated();
		}

		if (mNormalRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mNormalRegistry.getWriteInfo(mBindings.normalBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mNormalRegistry.setUpdated();
		}

		if (mTangentRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mTangentRegistry.getWriteInfo(mBindings.tangentBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mTangentRegistry.setUpdated();
		}

		if (mJointIndexRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mJointIndexRegistry.getWriteInfo(mBindings.jointIndexBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mJointIndexRegistry.setUpdated();
		}

		if (mWeightRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mWeightRegistry.getWriteInfo(mBindings.weightBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mWeightRegistry.setUpdated();
		}

		if (mIndexRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mIndexRegistry.getWriteInfo(mBindings.indexBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mIndexRegistry.setUpdated();
		}

		if (mPrimitiveRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPrimitiveRegistry.getWriteInfo(mBindings.primitiveBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mPrimitiveRegistry.setUpdated();
		}

		if (mMeshletRegistry.needDescriptorUpdate())
		{
			auto bufInfo = mMeshletRegistry.getBufferInfo();
			auto writeInfo = mMeshletRegistry.getWriteInfo(mBindings.meshletBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mMeshletRegistry.setUpdated();
		}

		if (mPerInstanceRegistry.needDescriptorUpdate())
		{
			auto bufInfo = mPerInstanceRegistry.getBufferInfo();
			auto writeInfo = mPerInstanceRegistry.getWriteInfo(mBindings.perInstanceBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mPerInstanceRegistry.setUpdated();
		}

		if (mJointRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mJointRegistry.getWriteInfo(mBindings.jointsBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mJointRegistry.setUpdated();
		}

		if (mPerMeshRegistry.needDescriptorUpdate())
		{
			auto bufInfo = mPerMeshRegistry.getBufferInfo();
			auto writeInfo = mPerMeshRegistry.getWriteInfo(mBindings.perMeshBinding);
			mBufferDescriptorSet.updateWrite(writeInfo);
			mPerMeshRegistry.setUpdated();
		}

		// Update materials.
		if (mMaterialRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mMaterialRegistry.getWriteInfo(mBindings.materialArrayBinding);
			mTextureDescriptorSet.updateWrite(writeInfo);
			mMaterialRegistry.setUpdated();
		}

		// Resize visability buffer if needed.
		if (uint32_t max = getMaxCmdBufferSize(params.frameIndex) * sizeof(uint32_t) / sizeof(meshletShaderCMD); max > (mVisabilityBuffer[params.frameIndex].getSize() - 4 * sizeof(uint32_t)))
		{
			mVisabilityBuffer[params.frameIndex].destroy();

			std::vector<uint32_t> visDispatch{ 0, 0, 1, 1 };

			error err = mVisabilityBuffer[params.frameIndex].build(params.is, visDispatch.data(), max + 4 * sizeof(uint32_t), sizeof(uint32_t) * 4, true);
			if (err)
				return err;

			std::vector<VkDescriptorBufferInfo> bufferInfo{};

			for (auto& b : mVisabilityBuffer)
				bufferInfo.push_back(VkDescriptorBufferInfo{ .buffer = b.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE });

			auto writeInfo = descriptorSet::getWriteInfo(mBindings.visabilityBuffer, bufferInfo);
			mBufferDescriptorSet.updateWrite(writeInfo);
		}

		return {};
	}

	error resourceManager::addToRender(instanceParams params)
	{
		if (!mOpaqueCommandBuffers.contains(params.m.mat.pixelShader->hash()))
		{
			commandBuffer cmd{};

			cmd.init(params.device, params.allocator, params.is, mCtx->config.inner.graphics.framesInFlight);
			error err = cmd.build(params.is);
			if (err)
				return err;

			mOpaqueCommandBuffers[params.m.mat.pixelShader->hash()] = cmd;

			mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::cmdBuf, .cmdBuf = &mOpaqueCommandBuffers[params.m.mat.pixelShader->hash()] });
		}

		if (mOpaqueCommandBuffers[params.m.mat.pixelShader->hash()].instanceExists(params.m.id, params.frameIndex))
			return {};

		// Upload material.
		auto materialOffsets = mMaterialRegistry.addMaterials(params.m.mat);
		if (!materialOffsets)
			return materialOffsets.err();

		perInstanceAttr attr = params.m.instanceAttributes;
		attr.jointIndex = std::numeric_limits<uint32_t>::max();
		attr.jointOffset = std::numeric_limits<uint32_t>::max();

		// Upload animation data.
		if (params.m.anims.jointMatrices && params.m.anims.jointMatrices->size() != 0)
		{
			auto jointHandle = mJointRegistry.addBlock(
				params.m.id,
				params.m.anims.jointMatrices->data(),
				params.m.anims.jointMatrices->size() * sizeof(glm::mat4),
				params.is
			);
			if (!jointHandle)
				return jointHandle.err();

			attr.jointIndex = jointHandle.value().bufferIndex;
			attr.jointOffset = jointHandle.value().offset / uint32_t(sizeof(glm::mat4));
		}

		attr.globalMaterialOffset = materialOffsets.value();

		auto perInstanceHandle = mPerInstanceRegistry.addBlock(
			params.m.id,
			&attr,
			sizeof(perInstanceAttr),
			params.is
		);
		if (!perInstanceHandle)
			return perInstanceHandle.err();

		commandBuffer::addInstanceParams addParams{
			.instanceID = params.m.id,
			.perInstanceHandle = perInstanceHandle.value(),
			.meshesData = {},
			.isBlendGeometry = params.m.mat.hasBlendMaterials(),
			.frameIndex = params.frameIndex,
		};

		for (int i = 0; i < params.m.meshData->size(); i++)
		{
			const mesh& crntMesh = params.m.meshData->operator[](i);
			perMeshAttributes crntMeshAttrs = params.m.perMeshData->operator[](i);

			bufferHandle vertexHandle{};
			// Upload common vertex attributes.
			auto handle = mPositionRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.positions.data(),
				crntMesh.positions.size() * sizeof(glm::vec4),
				params.is
			);
			if (!handle)
				return handle.err();

			handle = mNormalRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.normal.data(),
				crntMesh.normal.size() * sizeof(glm::vec4),
				params.is
			);
			if (!handle)
				return handle.err();

			handle = mTangentRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.tangent.data(),
				crntMesh.tangent.size() * sizeof(glm::vec4),
				params.is
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
					params.is
				);
				if (!handle)
					return handle.err();

				handle = mWeightRegistry.addBlock(
					crntMesh.meshHash,
					crntMesh.weights.data(),
					crntMesh.weights.size() * sizeof(glm::vec4),
					params.is
				);
				if (!handle)
					return handle.err();

				weightHandle = handle.value();
			}

			auto perMeshHandle = mPerMeshRegistry.addBlock(
				crntMesh.meshHash,
				&crntMeshAttrs,
				sizeof(perMeshAttributes),
				params.is
			);
			if (!perMeshHandle)
				return perMeshHandle.err();

			std::vector<meshlet> meshlets = crntMesh.meshlets.data;

			auto indexHandle = mIndexRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.indices.data.data(),
				crntMesh.indices.data.size() * sizeof(uint32_t),
				params.is
			);
			if (!handle)
				return handle.err();

			auto primitivesHandle = mPrimitiveRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.primitives.data.data(),
				crntMesh.primitives.data.size() * sizeof(uint32_t),
				params.is
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
				params.is
			);
			if (!handle)
				return handle.err();

			addParams.meshesData.push_back(
				commandBuffer::meshes{
					.meshID = crntMesh.meshHash,
					.meshHandle = perMeshHandle.value(),
					.meshletHandle = handle.value(),
					.meshlets = crntMesh.meshlets,
				}
				);
		}

		if (addParams.isBlendGeometry)
		{
			error err = mAccumilationCommandBuffer.addInstance(addParams);
			if (err)
				return err;
		}

		// Have to add because some materials can have BLEND enabled for material but have opaque geometry too.
		// Cutoff goes here too.
		error err = mOpaqueCommandBuffers[params.m.mat.pixelShader->hash()].addInstance(addParams);
		if (err)
			return err;


		return {};
	}

	error resourceManager::updateInstance(instanceParams params)
	{
		// Get material offsets or upload as new.
		auto materialOffsets = mMaterialRegistry.getMaterialsOffset(params.m.mat);
		if (!materialOffsets)
		{
			materialOffsets = mMaterialRegistry.addMaterials(params.m.mat);
			if (!materialOffsets)
				return materialOffsets.err();
		}

		// Form new instance attrs.
		perInstanceAttr attr = params.m.instanceAttributes;
		attr.globalMaterialOffset = materialOffsets.value();

		auto jointHandle = mJointRegistry.findBlock(params.m.id);
		if (jointHandle)
		{
			attr.jointIndex = jointHandle.value().bufferIndex;
			attr.jointOffset = jointHandle.value().offset / uint32_t(sizeof(glm::mat4));
		}

		return mPerInstanceRegistry.updateBlock(params.m.id, &attr, sizeof(perInstanceAttr), params.is, params.frameIndex);
	}

	error resourceManager::updateAnimations(instanceParams params)
	{
		return mJointRegistry.updateBlock(
			params.m.id,
			params.m.anims.jointMatrices->data(),
			params.m.anims.jointMatrices->size() * sizeof(glm::mat4),
			params.is,
			params.frameIndex
		);
	}

	void resourceManager::removeFromRender(instanceParams params)
	{
		// Execute all schedulded deletes.
		mPositionRegistry.deleteScheduledBlocks(params.frameIndex);
		mNormalRegistry.deleteScheduledBlocks(params.frameIndex);
		mTangentRegistry.deleteScheduledBlocks(params.frameIndex);
		mJointIndexRegistry.deleteScheduledBlocks(params.frameIndex);
		mWeightRegistry.deleteScheduledBlocks(params.frameIndex);
		mIndexRegistry.deleteScheduledBlocks(params.frameIndex);
		mPrimitiveRegistry.deleteScheduledBlocks(params.frameIndex);
		mMeshletRegistry.deleteScheduledBlocks(params.frameIndex);
		mPerMeshRegistry.deleteScheduledBlocks(params.frameIndex);
		mPerInstanceRegistry.deleteScheduledBlocks(params.frameIndex);
		mJointRegistry.deleteScheduledBlocks(params.frameIndex);

		mMaterialRegistry.deleteScheduledMaterials(params.frameIndex);

		if (!mOpaqueCommandBuffers.contains(params.m.mat.pixelShader->hash()))
			return;

		mPerInstanceRegistry.scheduleDeleteBlock(params.m.id, params.frameIndex);

		for (int i = 0; i < params.m.meshData->size(); i++)
		{
			auto& cmd = mOpaqueCommandBuffers[params.m.mat.pixelShader->hash()];

			const mesh& crntMesh = params.m.meshData->operator[](i);
			const perMeshAttributes crntMeshAttrs = params.m.perMeshData->operator[](i);

			// Remove instance.
			cmd.removeInstance(
				commandBuffer::removeInstanceParams{
					.instanceID = params.m.id,
					.meshID = crntMesh.meshHash,
					.frameIndex = params.frameIndex,
				}
				);

			mAccumilationCommandBuffer.removeInstance(
				commandBuffer::removeInstanceParams{
					.instanceID = params.m.id,
					.meshID = crntMesh.meshHash,
					.frameIndex = params.frameIndex,
				}
				);

			// Remove animation data.
			mJointRegistry.scheduleDeleteBlock(params.m.id, params.frameIndex);

			// Mesh isn't used.
			if (!cmd.meshIsUsed(crntMesh.meshHash, params.frameIndex) && !mAccumilationCommandBuffer.meshIsUsed(crntMesh.meshHash, params.frameIndex))
			{
				mPositionRegistry.scheduleDeleteBlock(crntMesh.meshHash, params.frameIndex);

				mNormalRegistry.scheduleDeleteBlock(crntMesh.meshHash, params.frameIndex);

				mTangentRegistry.scheduleDeleteBlock(crntMesh.meshHash, params.frameIndex);

				if (crntMeshAttrs.isSkinned)
				{
					mJointIndexRegistry.scheduleDeleteBlock(crntMesh.meshHash, params.frameIndex);
					mWeightRegistry.scheduleDeleteBlock(crntMesh.meshHash, params.frameIndex);
				}

				mIndexRegistry.scheduleDeleteBlock(crntMesh.meshHash, params.frameIndex);

				mPrimitiveRegistry.scheduleDeleteBlock(crntMesh.meshHash, params.frameIndex);

				mMeshletRegistry.scheduleDeleteBlock(crntMesh.meshHash, params.frameIndex);

				mMaterialRegistry.scheduleDeleteMaterials(params.m.mat.hash, params.frameIndex);

				mPerMeshRegistry.scheduleDeleteBlock(crntMesh.meshHash, params.frameIndex);
			}
		}
	}

	void resourceManager::bindDescriptorSets(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout layout)
	{
		std::array<VkDescriptorSet, 2> set{
			mBufferDescriptorSet.getDescriptorSet().first,
			mTextureDescriptorSet.getDescriptorSet().first,
		};

		vkCmdBindDescriptorSets(cmd, bindPoint, layout, 0, 2, set.data(), 0, nullptr);
	}

	void resourceManager::transitionColorAttachmentImage(
		VkCommandBuffer cmd,
		VkImageLayout current,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask
	) const
	{
		transitionImage(
			cmd,
			getColorAttachmentImage(false).img.image,
			getColorAttachmentImage(false).img.format,
			current,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask
		);

		if (mPreset.msaa > 1)
		{
			transitionImage(
				cmd,
				getColorAttachmentImage(true).img.image,
				getColorAttachmentImage(true).img.format,
				current,
				newLayout,
				srcStageMask,
				srcAccessMask,
				dstStageMask,
				dstAccessMask
			);
		}
	}

	void resourceManager::transitionDepthImage(
		VkCommandBuffer cmd,
		VkImageLayout current,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask
	) const
	{
		transitionImage(
			cmd,
			getDepthImage(false).img.image,
			getDepthImage(false).img.format,
			current,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask
		);

		if (mPreset.msaa > 1)
		{
			transitionImage(
				cmd,
				getDepthImage(true).img.image,
				getDepthImage(true).img.format,
				current,
				newLayout,
				srcStageMask,
				srcAccessMask,
				dstStageMask,
				dstAccessMask
			);
		}
	}
	void resourceManager::transitionAccumImage(
		VkCommandBuffer cmd,
		VkImageLayout current,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask
	) const
	{
		transitionImage(
			cmd,
			getAccumImage(false).img.image,
			getAccumImage(false).img.format,
			current,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask
		);

		if (mPreset.msaa > 1)
		{
			transitionImage(
				cmd,
				getAccumImage(true).img.image,
				getAccumImage(true).img.format,
				current,
				newLayout,
				srcStageMask,
				srcAccessMask,
				dstStageMask,
				dstAccessMask
			);
		}
	}

	void resourceManager::transitionRevealImage(
		VkCommandBuffer cmd,
		VkImageLayout current,
		VkImageLayout newLayout,
		VkPipelineStageFlags2 srcStageMask,
		VkAccessFlags2 srcAccessMask,
		VkPipelineStageFlags2 dstStageMask,
		VkAccessFlags2 dstAccessMask
	) const
	{
		transitionImage(
			cmd,
			getRevealImage(false).img.image,
			getRevealImage(false).img.format,
			current,
			newLayout,
			srcStageMask,
			srcAccessMask,
			dstStageMask,
			dstAccessMask
		);

		if (mPreset.msaa > 1)
		{
			transitionImage(
				cmd,
				getRevealImage(true).img.image,
				getRevealImage(true).img.format,
				current,
				newLayout,
				srcStageMask,
				srcAccessMask,
				dstStageMask,
				dstAccessMask
			);
		}
	}

	void resourceManager::transitionHzbChainImages(VkCommandBuffer cmd, VkImageLayout current, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask) const
	{
		for (auto& hzb : mHZBImages)
		{
			transitionImage(
				cmd,
				hzb.img.image,
				hzb.img.format,
				current,
				newLayout,
				srcStageMask,
				srcAccessMask,
				dstStageMask,
				dstAccessMask
			);
		}
	}

	vulkanImage resourceManager::getColorAttachmentImage(bool needResolve) const
	{
		return needResolve ? mColorAttachmentResolveImage : mColorAttachmentImage;
	}

	vulkanImage resourceManager::getDepthImage(bool needResolve) const
	{
		return needResolve ? mDepthResolveImage : mDepthImage;
	}

	vulkanImage resourceManager::getAccumImage(bool needResolve) const
	{
		return needResolve ? mAccumResolveImage : mAccumImage;
	}

	vulkanImage resourceManager::getRevealImage(bool needResolve) const
	{
		return needResolve ? mRevealResolveImage : mRevealImage;
	}

	std::vector<vulkanImage> resourceManager::getHZB() const
	{
		return mHZBImages;
	}

	error resourceManager::changeViewPort(resourceManager::buildParams params)
	{
		destroyViewPortDependantResources();

		error err = buildViewPortDependantResources(params);
		if (err)
			return err;

		updateWriteAfterViewPortChange();

		return {};
	}

	error resourceManager::addLine(addLineParams params)
	{
		error err = mLineBuffer.updateBuffer(params.is, &params.l, sizeof(line), mLineBuffer.getLoadedBytes());
		if (err && err.is(errCodeBufferOverFlow))
		{
			vulkanBuffer newBuf{};

			newBuf.init(params.device, params.allocator, { true, false });

			err = newBuf.build(params.is, mLineBuffer, mLineBuffer.getLoadedBytes() * 2, false);
			if (err)
				return err;

			mLineBuffer.destroy();

			mLineBuffer = newBuf;
		}
		if (err)
			return err;

		err = mLineBuffer.updateBuffer(params.is, &params.l, sizeof(line), mLineBuffer.getLoadedBytes());
		if (err)
			return err;

		return {};
	}

	error resourceManager::updatePerDrawBuffer(perDrawData data, submit& is, uint32_t frameIndex)
	{
		mUboPerDrawBuffer[frameIndex].markBytesAsDead(sizeof(perDrawData));

		return mUboPerDrawBuffer[frameIndex].updateBuffer(
			is,
			&data,
			sizeof(perDrawData),
			0
		);
	}

	error resourceManager::buildResources(buildParams params)
	{
		// Buffers.
		mPositionRegistry.init(params.device, params.allocator);
		mNormalRegistry.init(params.device, params.allocator);
		mTangentRegistry.init(params.device, params.allocator);
		mJointIndexRegistry.init(params.device, params.allocator);
		mWeightRegistry.init(params.device, params.allocator);
		mIndexRegistry.init(params.device, params.allocator);
		mPrimitiveRegistry.init(params.device, params.allocator);
		mMeshletRegistry.init(params.device, params.allocator);
		mPerMeshRegistry.init(params.device, params.allocator);
		// Updated each frame used as MAPPED.
		mPerInstanceRegistry.init(params.device, params.allocator, { true, false }, mCtx->config.inner.graphics.framesInFlight);
		mJointRegistry.init(params.device, params.allocator, { true, false }, mCtx->config.inner.graphics.framesInFlight);
		mAccumilationCommandBuffer.init(params.device, params.allocator, params.is, mCtx->config.inner.graphics.framesInFlight);
		mLineBuffer.init(params.device, params.allocator, { true, false });

		error err = mAccumilationCommandBuffer.build(params.is);
		if (err)
			return err;

		err = mLineBuffer.build(params.is, nullptr, sizeof(glm::vec3) * 2 * 5000, 0);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mPositionRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mNormalRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mTangentRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mJointIndexRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mWeightRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mIndexRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mPrimitiveRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mMeshletRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mPerInstanceRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mJointRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mPerMeshRegistry });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::cmdBuf, .cmdBuf = &mAccumilationCommandBuffer });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::vulkanBuf, .vulkanBuf = &mLineBuffer });

		// Init visability buffers.
		std::vector<uint32_t> visDispatch{ 0, 0, 1, 1 };

		mVisabilityBuffer.resize(mCtx->config.inner.graphics.framesInFlight);

		for (uint32_t i = 0; i < mCtx->config.inner.graphics.framesInFlight; i++)
		{
			mVisabilityBuffer[i].init(params.device, params.allocator);

			err = mVisabilityBuffer[i].build(params.is, visDispatch.data(), 2 << 24, sizeof(uint32_t) * 4, true);
			if (err)
				return err;

			mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::vulkanBuf, .vulkanBuf = &mVisabilityBuffer[i] });
		}

		mUboPerDrawBuffer.resize(mCtx->config.inner.graphics.framesInFlight);

		// Init per draw buffer.
		for (uint32_t i = 0; i < mCtx->config.inner.graphics.framesInFlight; i++)
		{
			mUboPerDrawBuffer[i].init(params.device, params.allocator, { true, false });

			error err = mUboPerDrawBuffer[i].buildAsUBO(params.is, nullptr, sizeof(perDrawData), 0);
			if (err)
				return err;

			mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::vulkanBuf, .vulkanBuf = &mUboPerDrawBuffer[i] });
		}

		// Textures.
		auto samp = descriptorSet::createSampler(params.device, float(mPreset.anisotropicFiltering));
		if (!samp)
			return samp.err();

		mSampler = samp.value();

		auto defaultMat = mCtx->mAmanager->loadDetaultMaterial();
		if (!defaultMat)
			return defaultMat.err();

		err = mMaterialRegistry.init(mSampler, defaultMat.value());
		if (err)
			return err;

		mClipMap.init(params.device, params.allocator);

		err = mClipMap.build(
			params.is,
			VkExtent3D{
				.width = mCtx->config.inner.graphics.clipMapResolution,
				.height = mCtx->config.inner.graphics.clipMapResolution,
				.depth = mCtx->config.inner.graphics.clipMapResolution,
			},
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			false,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			false,
			VK_IMAGE_TYPE_3D
			);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::vulkImg, .img = &mClipMap });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::sampler, .sampler = &mSampler });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::matReg, .matReg = &mMaterialRegistry });

		return buildViewPortDependantResources(params);
	}

	error resourceManager::buildViewPortDependantResources(buildParams params)
	{
		VkExtent3D colorAttachmentExtent = {
			params.width,
			params.height,
			1
		};

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageUsageFlags depthImageUsages{};
		depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		depthImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;

		mColorAttachmentImage.init(params.device, params.allocator);
		mColorAttachmentResolveImage.init(params.device, params.allocator);
		mDepthImage.init(params.device, params.allocator);
		mDepthResolveImage.init(params.device, params.allocator);
		mAccumImage.init(params.device, params.allocator);
		mAccumResolveImage.init(params.device, params.allocator);
		mRevealImage.init(params.device, params.allocator);
		mRevealResolveImage.init(params.device, params.allocator);

		error err = mColorAttachmentImage.build(params.is, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, false, sampleCounts(mPreset.msaa), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mColorAttachmentResolveImage.build(params.is, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mDepthImage.build(params.is, colorAttachmentExtent, VK_FORMAT_D32_SFLOAT, depthImageUsages, false, sampleCounts(mPreset.msaa), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mDepthResolveImage.build(params.is, colorAttachmentExtent, VK_FORMAT_D32_SFLOAT, depthImageUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		// Build images for OIT.
		const VkImageUsageFlags weightedUsages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		err = mAccumImage.build(params.is, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, sampleCounts(mPreset.msaa), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mAccumResolveImage.build(params.is, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mRevealImage.build(params.is, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, sampleCounts(mPreset.msaa), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mRevealResolveImage.build(params.is, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		// Build HZB.
		uint32_t mip0Width = (std::max)(1u, params.width >> 1);
		uint32_t mip0Height = (std::max)(1u, params.height >> 1);

		uint32_t hzbMipLevels = static_cast<uint32_t>(std::floor(std::log2((std::max)(mip0Width, mip0Height))));

		mHZBImages.clear();

		VkExtent3D mipExtent = { mip0Width, mip0Height, 1 };

		for (uint32_t l = 0; l < hzbMipLevels; l++)
		{
			const uint32_t mipWidth = (std::max)(1u, params.width >> (l + 1));
			const uint32_t mipHeight = (std::max)(1u, params.height >> (l + 1));

			vulkanImage currentDepth{};

			currentDepth.init(params.device, params.allocator);

			mipExtent.width = mipWidth;
			mipExtent.height = mipHeight;

			err = currentDepth.build(params.is, mipExtent, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL);
			if (err)
				return err;

			mHZBImages.push_back(std::move(currentDepth));
		}

		return {};
	}

	error resourceManager::buildDescriptors(resourceManager::buildParams params)
	{
		error err = mBufferDescriptorSet.init(
			params.device,
			params.physicalDevice,
			poolConstraints{
				.maxRWImageDescriptors = mDeviceLimits.maxRWImage,
				.maxSampledImageDescriptors = mDeviceLimits.maxSampledImage,
				.maxCombinedImageDescriptors = mDeviceLimits.maxCombinedImageSamplers,
				.maxBuffersDescriptors = mDeviceLimits.maxStorageBuffers,
				.maxUniformBuffersDescriptors = mDeviceLimits.maxUniformBuffers,
			}
			);
		if (err)
			return err;

		err = mTextureDescriptorSet.init(
			params.device,
			params.physicalDevice,
			poolConstraints{
				.maxRWImageDescriptors = mDeviceLimits.maxRWImage,
				.maxSampledImageDescriptors = mDeviceLimits.maxSampledImage,
				.maxCombinedImageDescriptors = mDeviceLimits.maxCombinedImageSamplers,
				.maxBuffersDescriptors = mDeviceLimits.maxStorageBuffers,
				.maxUniformBuffersDescriptors = mDeviceLimits.maxUniformBuffers,
			}
			);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::descSet, .descSet = &mBufferDescriptorSet });
		mDeletionQueue.addDestroyTask(destroyTask{ .type = handleType::descSet, .descSet = &mTextureDescriptorSet });

		// Buffers bindings.
		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.positionsBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.normalBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.tangentBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.jointIndexBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.weightBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perInstanceBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.cmdOpaqueBufferBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.cmdAccumilationBufferBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.indexBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.primitiveBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.meshletBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.jointsBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perMeshBinding, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.visabilityBuffer, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.lineBuffer, mDeviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perDrawBufferUboBinding, mDeviceLimits.maxUniformBuffers / mBindings.uniformBufferBindings, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			)
		);

		// Texture bindings.
		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.accumBinding, mDeviceLimits.maxCombinedImageSamplers / mBindings.combinedSampledImageBindings, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.revealBinding, mDeviceLimits.maxCombinedImageSamplers / mBindings.combinedSampledImageBindings, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.materialArrayBinding, mDeviceLimits.maxCombinedImageSamplers / mBindings.combinedSampledImageBindings, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.clipMapBinding, mDeviceLimits.maxSampledImage / mBindings.storageImageBindings, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
			)
		);

		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.orignalZBufferBinding, mDeviceLimits.maxSampledImage / mBindings.combinedSampledImageBindings, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
			)
		);

		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.hzbBinding, mDeviceLimits.maxRWImage / mBindings.storageImageBindings, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
			)
		);

		err = mBufferDescriptorSet.build(VK_SHADER_STAGE_ALL, mBindings.storageBufferBindings + mBindings.uniformBufferBindings);
		if (err)
			return err;

		err = mTextureDescriptorSet.build(VK_SHADER_STAGE_ALL, mBindings.storageImageBindings + mBindings.combinedSampledImageBindings);
		if (err)
			return err;

		// Set unchanged resource descriptors.
		std::vector<VkDescriptorBufferInfo> bufferInfo{};

		for (auto& b : mUboPerDrawBuffer)
			bufferInfo.push_back(VkDescriptorBufferInfo{ .buffer = b.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE });

		auto writeInfo = descriptorSet::getWriteInfo(mBindings.perDrawBufferUboBinding, bufferInfo, true);
		mBufferDescriptorSet.updateWrite(writeInfo);

		bufferInfo = {};

		for (auto& b : mVisabilityBuffer)
			bufferInfo.push_back(VkDescriptorBufferInfo{ .buffer = b.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE });

		writeInfo = descriptorSet::getWriteInfo(mBindings.visabilityBuffer, bufferInfo);
		mBufferDescriptorSet.updateWrite(writeInfo);

		std::vector<VkDescriptorImageInfo> clipMapInfo{ VkDescriptorImageInfo{} };
		clipMapInfo.front().imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		clipMapInfo.front().imageView = mClipMap.img.view;

		std::vector<VkWriteDescriptorSet> wSet = descriptorSet::getWriteInfo(mBindings.clipMapBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, clipMapInfo);

		mTextureDescriptorSet.updateWrite(wSet);

		updateWriteAfterViewPortChange();

		return {};
	}

	void resourceManager::updateWriteAfterViewPortChange()
	{
		std::vector<VkDescriptorImageInfo> originalZInfo{ VkDescriptorImageInfo{} };
		originalZInfo.front().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		originalZInfo.front().imageView = getDepthImage(mPreset.msaa > 1).img.view;

		std::vector<VkWriteDescriptorSet> wSet = descriptorSet::getWriteInfo(mBindings.orignalZBufferBinding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, originalZInfo);

		mTextureDescriptorSet.updateWrite(wSet);

		std::vector<VkDescriptorImageInfo> hzbInfo{};

		for (auto& h : mHZBImages)
		{
			VkDescriptorImageInfo imgInfo{
				.imageView = h.img.view,
				.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
			};

			hzbInfo.push_back(std::move(imgInfo));
		}

		wSet = descriptorSet::getWriteInfo(mBindings.hzbBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, hzbInfo);

		mTextureDescriptorSet.updateWrite(wSet);

		std::vector<VkDescriptorImageInfo> info{ VkDescriptorImageInfo{} };
		info.front().sampler = mSampler;
		info.front().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.front().imageView = getAccumImage(mPreset.msaa > 1).img.view;

		wSet = descriptorSet::getWriteInfo(mBindings.accumBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info);

		mTextureDescriptorSet.updateWrite(wSet);

		info.front().sampler = mSampler;
		info.front().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.front().imageView = getRevealImage(mPreset.msaa > 1).img.view;

		wSet = descriptorSet::getWriteInfo(mBindings.revealBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info);

		mTextureDescriptorSet.updateWrite(wSet);
	}

	void resourceManager::destroyViewPortDependantResources()
	{
		mAccumImage.destroy();
		mRevealImage.destroy();
		mAccumResolveImage.destroy();
		mRevealResolveImage.destroy();
		mColorAttachmentImage.destroy();
		mColorAttachmentResolveImage.destroy();
		mDepthImage.destroy();
		mDepthResolveImage.destroy();

		for (auto& v : mHZBImages)
			v.destroy();

		mHZBImages.clear();
	}

	uint32_t resourceManager::getMaxCmdBufferSize(uint32_t frameIndex) const
	{
		uint32_t opaque{};
		for (auto& [_, v] : mOpaqueCommandBuffers)
			opaque += v.getCommandBufferLoadedSize(frameIndex);

		return std::max(opaque, mAccumilationCommandBuffer.getCommandBufferLoadedSize(frameIndex));
	}
}