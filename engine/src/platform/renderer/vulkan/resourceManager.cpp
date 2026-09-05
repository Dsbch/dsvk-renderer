#include <pch.h>
#include "resourceManager.h"

namespace engine
{
	void resourceManager::init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanCtx)
	{
		mCtx = ctx;
		mVulkanCtx = vulkanCtx;
	}

	error resourceManager::build(uint32_t width, uint32_t height)
	{
		error err = buildResources(width, height);
		if (err)
			return err;

		err = buildDescriptors();
		if (err)
			return err;

		return {};
	}

	void resourceManager::destroy()
	{
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

	VkSampler resourceManager::getSampler() const
	{
		return mSampler;
	}

	error resourceManager::updateDescriptors(uint32_t frameIndex)
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
					.device = mVulkanCtx->device,
					.allocator = mVulkanCtx->allocator,
					.is = mVulkanCtx->iSubmit,
					.frameIndex = frameIndex,
				}
				);
			if (err)
				return err;
		}

		error err = mAccumilationCommandBuffer.updateCommandBuffer(
			commandBuffer::updateCommandBufferParams{
				.device = mVulkanCtx->device,
				.allocator = mVulkanCtx->allocator,
				.is = mVulkanCtx->iSubmit,
				.frameIndex = frameIndex,
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
		if (uint32_t max = getMaxCmdBufferSize(frameIndex) * sizeof(uint32_t) / sizeof(meshletShaderCMD); max > (mVisabilityBuffer[frameIndex].getSize() - 4 * sizeof(uint32_t)))
		{
			mVisabilityBuffer[frameIndex].destroy();

			std::vector<uint32_t> visDispatch{ 0, 0, 1, 1 };

			error err = mVisabilityBuffer[frameIndex].build(mVulkanCtx->iSubmit, visDispatch.data(), max + 4 * sizeof(uint32_t), sizeof(uint32_t) * 4, true);
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

	error resourceManager::addToRender(const model& m, uint32_t frameIndex)
	{
		if (!mOpaqueCommandBuffers.contains(m.mat.pixelShader->hash()))
		{
			commandBuffer cmd{};

			cmd.init(mVulkanCtx->device, mVulkanCtx->allocator, mVulkanCtx->iSubmit, mCtx->config.inner.graphics.framesInFlight);
			error err = cmd.build(mVulkanCtx->iSubmit);
			if (err)
				return err;

			mOpaqueCommandBuffers[m.mat.pixelShader->hash()] = cmd;

			mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::cmdBuf, .cmdBuf = &mOpaqueCommandBuffers[m.mat.pixelShader->hash()] });
		}

		if (mOpaqueCommandBuffers[m.mat.pixelShader->hash()].instanceExists(m.id, frameIndex))
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
			auto jointHandle = mJointRegistry.addBlock(
				m.id,
				m.anims.jointMatrices->data(),
				m.anims.jointMatrices->size() * sizeof(glm::mat4),
				mVulkanCtx->iSubmit
			);
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
			mVulkanCtx->iSubmit
		);
		if (!perInstanceHandle)
			return perInstanceHandle.err();

		commandBuffer::addInstanceParams addParams{
			.instanceID = m.id,
			.perInstanceHandle = perInstanceHandle.value(),
			.meshesData = {},
			.isBlendGeometry = m.mat.hasBlendMaterials(),
			.frameIndex = frameIndex,
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
				mVulkanCtx->iSubmit
			);
			if (!handle)
				return handle.err();

			handle = mNormalRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.normal.data(),
				crntMesh.normal.size() * sizeof(glm::vec4),
				mVulkanCtx->iSubmit
			);
			if (!handle)
				return handle.err();

			handle = mTangentRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.tangent.data(),
				crntMesh.tangent.size() * sizeof(glm::vec4),
				mVulkanCtx->iSubmit
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
					mVulkanCtx->iSubmit
				);
				if (!handle)
					return handle.err();

				handle = mWeightRegistry.addBlock(
					crntMesh.meshHash,
					crntMesh.weights.data(),
					crntMesh.weights.size() * sizeof(glm::vec4),
					mVulkanCtx->iSubmit
				);
				if (!handle)
					return handle.err();

				weightHandle = handle.value();
			}

			auto perMeshHandle = mPerMeshRegistry.addBlock(
				crntMesh.meshHash,
				&crntMeshAttrs,
				sizeof(perMeshAttributes),
				mVulkanCtx->iSubmit
			);
			if (!perMeshHandle)
				return perMeshHandle.err();

			std::vector<meshlet> meshlets = crntMesh.meshlets.data;

			auto indexHandle = mIndexRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.indices.data.data(),
				crntMesh.indices.data.size() * sizeof(uint32_t),
				mVulkanCtx->iSubmit
			);
			if (!handle)
				return handle.err();

			auto primitivesHandle = mPrimitiveRegistry.addBlock(
				crntMesh.meshHash,
				crntMesh.primitives.data.data(),
				crntMesh.primitives.data.size() * sizeof(uint32_t),
				mVulkanCtx->iSubmit
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
				mVulkanCtx->iSubmit
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
		error err = mOpaqueCommandBuffers[m.mat.pixelShader->hash()].addInstance(addParams);
		if (err)
			return err;


		return {};
	}

	error resourceManager::updateInstance(const model& m, uint32_t frameIndex)
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

		return mPerInstanceRegistry.updateBlock(m.id, &attr, sizeof(perInstanceAttr), mVulkanCtx->iSubmit, frameIndex);
	}

	error resourceManager::updateAnimations(const model& m, uint32_t frameIndex)
	{
		return mJointRegistry.updateBlock(
			m.id,
			m.anims.jointMatrices->data(),
			m.anims.jointMatrices->size() * sizeof(glm::mat4),
			mVulkanCtx->iSubmit,
			frameIndex
		);
	}

	void resourceManager::removeFromRender(const model& m, uint32_t frameIndex)
	{
		// Execute all schedulded deletes.
		mPositionRegistry.deleteScheduledBlocks(frameIndex);
		mNormalRegistry.deleteScheduledBlocks(frameIndex);
		mTangentRegistry.deleteScheduledBlocks(frameIndex);
		mJointIndexRegistry.deleteScheduledBlocks(frameIndex);
		mWeightRegistry.deleteScheduledBlocks(frameIndex);
		mIndexRegistry.deleteScheduledBlocks(frameIndex);
		mPrimitiveRegistry.deleteScheduledBlocks(frameIndex);
		mMeshletRegistry.deleteScheduledBlocks(frameIndex);
		mPerMeshRegistry.deleteScheduledBlocks(frameIndex);
		mPerInstanceRegistry.deleteScheduledBlocks(frameIndex);
		mJointRegistry.deleteScheduledBlocks(frameIndex);

		mMaterialRegistry.deleteScheduledMaterials(frameIndex);

		if (!mOpaqueCommandBuffers.contains(m.mat.pixelShader->hash()))
			return;

		mPerInstanceRegistry.scheduleDeleteBlock(m.id, frameIndex);

		for (int i = 0; i < m.meshData->size(); i++)
		{
			auto& cmd = mOpaqueCommandBuffers[m.mat.pixelShader->hash()];

			const mesh& crntMesh = m.meshData->operator[](i);
			const perMeshAttributes crntMeshAttrs = m.perMeshData->operator[](i);

			// Remove instance.
			cmd.removeInstance(
				commandBuffer::removeInstanceParams{
					.instanceID = m.id,
					.meshID = crntMesh.meshHash,
					.frameIndex = frameIndex,
				}
				);

			mAccumilationCommandBuffer.removeInstance(
				commandBuffer::removeInstanceParams{
					.instanceID = m.id,
					.meshID = crntMesh.meshHash,
					.frameIndex = frameIndex,
				}
				);

			// Remove animation data.
			mJointRegistry.scheduleDeleteBlock(m.id, frameIndex);

			// Mesh isn't used.
			if (!cmd.meshIsUsed(crntMesh.meshHash, frameIndex) && !mAccumilationCommandBuffer.meshIsUsed(crntMesh.meshHash, frameIndex))
			{
				mPositionRegistry.scheduleDeleteBlock(crntMesh.meshHash, frameIndex);

				mNormalRegistry.scheduleDeleteBlock(crntMesh.meshHash, frameIndex);

				mTangentRegistry.scheduleDeleteBlock(crntMesh.meshHash, frameIndex);

				if (crntMeshAttrs.isSkinned)
				{
					mJointIndexRegistry.scheduleDeleteBlock(crntMesh.meshHash, frameIndex);
					mWeightRegistry.scheduleDeleteBlock(crntMesh.meshHash, frameIndex);
				}

				mIndexRegistry.scheduleDeleteBlock(crntMesh.meshHash, frameIndex);

				mPrimitiveRegistry.scheduleDeleteBlock(crntMesh.meshHash, frameIndex);

				mMeshletRegistry.scheduleDeleteBlock(crntMesh.meshHash, frameIndex);

				mMaterialRegistry.scheduleDeleteMaterials(m.mat.hash, frameIndex);

				mPerMeshRegistry.scheduleDeleteBlock(crntMesh.meshHash, frameIndex);
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

		if (mVulkanCtx->preset.msaa > 1)
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

		if (mVulkanCtx->preset.msaa > 1)
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

		if (mVulkanCtx->preset.msaa > 1)
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

		if (mVulkanCtx->preset.msaa > 1)
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

	std::span<vulkanImage> resourceManager::getHZB()
	{
		return mHZBImages;
	}

	const commandBuffer* resourceManager::getOpaqueCmdBuffer(uint32_t pixelShaderID) const
	{
		auto it = mOpaqueCommandBuffers.find(pixelShaderID);
		return it == mOpaqueCommandBuffers.end() ? nullptr : &it->second;
	}

	const commandBuffer& resourceManager::getAccumilationCmdBuffer() const
	{
		return mAccumilationCommandBuffer;
	}

	const vulkanBuffer& resourceManager::getVisabilityBuffer(uint32_t frameIndex) const
	{
		return mVisabilityBuffer[frameIndex];
	}

	const vulkanBuffer& resourceManager::getLinebuffer() const
	{
		return mLineBuffer;
	}

	error resourceManager::changeViewPort(uint32_t width, uint32_t height)
	{
		destroyViewPortDependantResources();

		error err = buildViewPortDependantResources(width, height);
		if (err)
			return err;

		updateWriteAfterViewPortChange();

		return {};
	}

	error resourceManager::addLine(line l)
	{
		error err = mLineBuffer.updateBuffer(mVulkanCtx->iSubmit, &l, sizeof(line), mLineBuffer.getLoadedBytes());
		if (err && err.is(errCodeBufferOverFlow))
		{
			vulkanBuffer newBuf{};

			newBuf.init(mVulkanCtx->device, mVulkanCtx->allocator, { true, false });

			err = newBuf.build(mVulkanCtx->iSubmit, mLineBuffer, mLineBuffer.getLoadedBytes() * 2, false);
			if (err)
				return err;

			mLineBuffer.destroy();

			mLineBuffer = newBuf;
		}
		if (err)
			return err;

		err = mLineBuffer.updateBuffer(mVulkanCtx->iSubmit, &l, sizeof(line), mLineBuffer.getLoadedBytes());
		if (err)
			return err;

		return {};
	}

	error resourceManager::updatePerDrawBuffer(perDrawData data, uint32_t frameIndex)
	{
		mUboPerDrawBuffer[frameIndex].markBytesAsDead(sizeof(perDrawData));

		return mUboPerDrawBuffer[frameIndex].updateBuffer(
			mVulkanCtx->iSubmit,
			&data,
			sizeof(perDrawData),
			0
		);
	}

	error resourceManager::buildResources(uint32_t width, uint32_t height)
	{
		// Buffers.
		mPositionRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mNormalRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mTangentRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mJointIndexRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mWeightRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mIndexRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mPrimitiveRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mMeshletRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mPerMeshRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator);
		// Updated each frame used as MAPPED.
		mPerInstanceRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator, { true, false }, mCtx->config.inner.graphics.framesInFlight);
		mJointRegistry.init(mVulkanCtx->device, mVulkanCtx->allocator, { true, false }, mCtx->config.inner.graphics.framesInFlight);
		mAccumilationCommandBuffer.init(mVulkanCtx->device, mVulkanCtx->allocator, mVulkanCtx->iSubmit, mCtx->config.inner.graphics.framesInFlight);
		mLineBuffer.init(mVulkanCtx->device, mVulkanCtx->allocator, { true, false });

		error err = mAccumilationCommandBuffer.build(mVulkanCtx->iSubmit);
		if (err)
			return err;

		err = mLineBuffer.build(mVulkanCtx->iSubmit, nullptr, sizeof(glm::vec3) * 2 * 5000, 0);
		if (err)
			return err;

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mPositionRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mNormalRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mTangentRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mJointIndexRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mWeightRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mIndexRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mPrimitiveRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mMeshletRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mPerInstanceRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mJointRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::buffRegistry, .buffRegistry = &mPerMeshRegistry });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::cmdBuf, .cmdBuf = &mAccumilationCommandBuffer });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::vulkanBuf, .vulkanBuf = &mLineBuffer });

		// Init visability buffers.
		std::vector<uint32_t> visDispatch{ 0, 0, 1, 1 };

		mVisabilityBuffer.resize(mCtx->config.inner.graphics.framesInFlight);

		for (uint32_t i = 0; i < mCtx->config.inner.graphics.framesInFlight; i++)
		{
			mVisabilityBuffer[i].init(mVulkanCtx->device, mVulkanCtx->allocator);

			err = mVisabilityBuffer[i].build(mVulkanCtx->iSubmit, visDispatch.data(), 2 << 24, sizeof(uint32_t) * 4, true);
			if (err)
				return err;

			mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::vulkanBuf, .vulkanBuf = &mVisabilityBuffer[i] });
		}

		mUboPerDrawBuffer.resize(mCtx->config.inner.graphics.framesInFlight);

		// Init per draw buffer.
		for (uint32_t i = 0; i < mCtx->config.inner.graphics.framesInFlight; i++)
		{
			mUboPerDrawBuffer[i].init(mVulkanCtx->device, mVulkanCtx->allocator, { true, false });

			error err = mUboPerDrawBuffer[i].buildAsUBO(mVulkanCtx->iSubmit, nullptr, sizeof(perDrawData), 0);
			if (err)
				return err;

			mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::vulkanBuf, .vulkanBuf = &mUboPerDrawBuffer[i] });
		}

		// Textures.
		auto samp = descriptorSet::createSampler(mVulkanCtx->device, float(mVulkanCtx->preset.anisotropicFiltering));
		if (!samp)
			return samp.err();

		mSampler = samp.value();

		auto defaultMat = mCtx->mAmanager->loadDetaultMaterial();
		if (!defaultMat)
			return defaultMat.err();

		err = mMaterialRegistry.init(mSampler, defaultMat.value());
		if (err)
			return err;

		mClipMap.init(mVulkanCtx->device, mVulkanCtx->allocator);

		err = mClipMap.build(
			mVulkanCtx->iSubmit,
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

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::vulkImg, .img = &mClipMap });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::sampler, .sampler = &mSampler });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::matReg, .matReg = &mMaterialRegistry });

		return buildViewPortDependantResources(width, height);
	}

	error resourceManager::buildViewPortDependantResources(uint32_t width, uint32_t height)
	{
		VkExtent3D colorAttachmentExtent = {
			width,
			height,
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

		mColorAttachmentImage.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mColorAttachmentResolveImage.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mDepthImage.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mDepthResolveImage.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mAccumImage.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mAccumResolveImage.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mRevealImage.init(mVulkanCtx->device, mVulkanCtx->allocator);
		mRevealResolveImage.init(mVulkanCtx->device, mVulkanCtx->allocator);

		error err = mColorAttachmentImage.build(mVulkanCtx->iSubmit, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, false, sampleCounts(mVulkanCtx->preset.msaa), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mColorAttachmentResolveImage.build(mVulkanCtx->iSubmit, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mDepthImage.build(mVulkanCtx->iSubmit, colorAttachmentExtent, VK_FORMAT_D32_SFLOAT, depthImageUsages, false, sampleCounts(mVulkanCtx->preset.msaa), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mDepthResolveImage.build(mVulkanCtx->iSubmit, colorAttachmentExtent, VK_FORMAT_D32_SFLOAT, depthImageUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		// Build images for OIT.
		const VkImageUsageFlags weightedUsages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		err = mAccumImage.build(mVulkanCtx->iSubmit, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, sampleCounts(mVulkanCtx->preset.msaa), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mAccumResolveImage.build(mVulkanCtx->iSubmit, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mRevealImage.build(mVulkanCtx->iSubmit, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, sampleCounts(mVulkanCtx->preset.msaa), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		err = mRevealResolveImage.build(mVulkanCtx->iSubmit, colorAttachmentExtent, VK_FORMAT_R16G16B16A16_SFLOAT, weightedUsages, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		if (err)
			return err;

		// Build HZB.
		uint32_t mip0Width = (std::max)(1u, width >> 1);
		uint32_t mip0Height = (std::max)(1u, height >> 1);

		uint32_t hzbMipLevels = static_cast<uint32_t>(std::floor(std::log2((std::max)(mip0Width, mip0Height))));

		mHZBImages.resize(hzbMipLevels);

		VkExtent3D mipExtent = { mip0Width, mip0Height, 1 };

		for (uint32_t l = 0; l < hzbMipLevels; l++)
		{
			const uint32_t mipWidth = (std::max)(1u, width >> (l + 1));
			const uint32_t mipHeight = (std::max)(1u, height >> (l + 1));

			vulkanImage currentDepth{};

			currentDepth.init(mVulkanCtx->device, mVulkanCtx->allocator);

			mipExtent.width = mipWidth;
			mipExtent.height = mipHeight;

			err = currentDepth.build(mVulkanCtx->iSubmit, mipExtent, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, false, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL);
			if (err)
				return err;

			mHZBImages[l] = std::move(currentDepth);
		}

		return {};
	}

	error resourceManager::buildDescriptors()
	{
		error err = mBufferDescriptorSet.init(
			mVulkanCtx->device,
			mVulkanCtx->physicalDevice,
			poolConstraints{
				.maxRWImageDescriptors = mVulkanCtx->deviceLimits.maxRWImage,
				.maxSampledImageDescriptors = mVulkanCtx->deviceLimits.maxSampledImage,
				.maxCombinedImageDescriptors = mVulkanCtx->deviceLimits.maxCombinedImageSamplers,
				.maxBuffersDescriptors = mVulkanCtx->deviceLimits.maxStorageBuffers,
				.maxUniformBuffersDescriptors = mVulkanCtx->deviceLimits.maxUniformBuffers,
			}
			);
		if (err)
			return err;

		err = mTextureDescriptorSet.init(
			mVulkanCtx->device,
			mVulkanCtx->physicalDevice,
			poolConstraints{
				.maxRWImageDescriptors = mVulkanCtx->deviceLimits.maxRWImage,
				.maxSampledImageDescriptors = mVulkanCtx->deviceLimits.maxSampledImage,
				.maxCombinedImageDescriptors = mVulkanCtx->deviceLimits.maxCombinedImageSamplers,
				.maxBuffersDescriptors = mVulkanCtx->deviceLimits.maxStorageBuffers,
				.maxUniformBuffersDescriptors = mVulkanCtx->deviceLimits.maxUniformBuffers,
			}
			);
		if (err)
			return err;

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::descSet, .descSet = &mBufferDescriptorSet });
		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::descSet, .descSet = &mTextureDescriptorSet });

		// Buffers bindings.
		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.positionsBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.normalBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.tangentBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.jointIndexBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.weightBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perInstanceBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.cmdOpaqueBufferBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.cmdAccumilationBufferBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.indexBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.primitiveBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.meshletBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.jointsBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perMeshBinding, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.visabilityBuffer, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.lineBuffer, mVulkanCtx->deviceLimits.maxStorageBuffers / mBindings.storageBufferBindings, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mBufferDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perDrawBufferUboBinding, mVulkanCtx->deviceLimits.maxUniformBuffers / mBindings.uniformBufferBindings, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			)
		);

		// Texture bindings.
		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.accumBinding, mVulkanCtx->deviceLimits.maxCombinedImageSamplers / mBindings.combinedSampledImageBindings, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.revealBinding, mVulkanCtx->deviceLimits.maxCombinedImageSamplers / mBindings.combinedSampledImageBindings, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.materialArrayBinding, mVulkanCtx->deviceLimits.maxCombinedImageSamplers / mBindings.combinedSampledImageBindings, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.clipMapBinding, mVulkanCtx->deviceLimits.maxSampledImage / mBindings.storageImageBindings, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
			)
		);

		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.orignalZBufferBinding, mVulkanCtx->deviceLimits.maxSampledImage / mBindings.combinedSampledImageBindings, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
			)
		);

		mTextureDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.hzbBinding, mVulkanCtx->deviceLimits.maxRWImage / mBindings.storageImageBindings, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
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
		originalZInfo.front().imageView = getDepthImage(mVulkanCtx->preset.msaa > 1).img.view;

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
		info.front().imageView = getAccumImage(mVulkanCtx->preset.msaa > 1).img.view;

		wSet = descriptorSet::getWriteInfo(mBindings.accumBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info);

		mTextureDescriptorSet.updateWrite(wSet);

		info.front().sampler = mSampler;
		info.front().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.front().imageView = getRevealImage(mVulkanCtx->preset.msaa > 1).img.view;

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
	}

	uint32_t resourceManager::getMaxCmdBufferSize(uint32_t frameIndex) const
	{
		uint32_t opaque{};
		for (auto& [_, v] : mOpaqueCommandBuffers)
			opaque += v.getCommandBufferLoadedSize(frameIndex);

		return std::max(opaque, mAccumilationCommandBuffer.getCommandBufferLoadedSize(frameIndex));
	}
}