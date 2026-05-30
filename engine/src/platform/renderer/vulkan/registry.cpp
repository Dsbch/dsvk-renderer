#include <pch.h>

#include "registry.h"
#include "descriptorSet.h"
#include "texture.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	void bufferRegistry::init(VkDevice device, VmaAllocator allocator, bool mapped)
	{
		mDevice = device;
		mAllocator = allocator;
		mNeedUpdate = false;
		mUseMappedBuffers = mapped;
	}

	withError<bufferHandle> bufferRegistry::addBlock(uint32_t id, const void* data, size_t sizeInBytes, submit& is, size_t newSize)
	{
		for (uint32_t i = 0; i < mBuffers.size(); i++)
		{
			if (auto handle = mBuffers[i].bufferHandles.find(bufferHandle{ .id = id }); handle != mBuffers[i].bufferHandles.end())
				return *handle;
		}

		for (uint32_t i = 0; i < mBuffers.size(); i++)
		{
			VmaVirtualAllocationCreateInfo allocateInfo{
				.size = sizeInBytes,
			};
			VmaVirtualAllocation vAllocation{};

			VkDeviceSize offset = 0;
			if (vmaVirtualAllocate(mBuffers[i].vBlock, &allocateInfo, &vAllocation, &offset) == VK_SUCCESS)
			{
				auto handle = bufferHandle{
						.id = id,
						.offset = uint32_t(offset),
						.bufferIndex = i,
						.size = sizeInBytes,
						.vAllocation = vAllocation,
				};

				mBuffers[i].bufferHandles.insert(handle);

				error err = mBuffers[i].buffer.updateBuffer(is, data, sizeInBytes, offset);
				if (err)
					return err;

				return handle;
			}
		}

		vulkanBuffer newBuffer{};
		newBuffer.init(mDevice, mAllocator, mUseMappedBuffers);

		if (sizeInBytes > newSize)
		{
			newSize = sizeInBytes;
		}

		VmaVirtualBlockCreateInfo vBlockInfo{
			.size = newSize,
		};
		VmaVirtualBlock vBlock;

		if (vmaCreateVirtualBlock(&vBlockInfo, &vBlock) != VK_SUCCESS)
			return error{ "can't create virtual block" };

		VmaVirtualAllocationCreateInfo allocateInfo{
			.size = sizeInBytes,
		};
		VmaVirtualAllocation vAllocation{};

		VkDeviceSize offset = 0;
		if (vmaVirtualAllocate(vBlock, &allocateInfo, &vAllocation, &offset) != VK_SUCCESS)
			return error{ "can't allocate in virtual block" };

		error err = newBuffer.build(is, data, newSize, sizeInBytes);
		if (err)
			return err;

		auto handle = bufferHandle{
				.id = id,
				.offset = uint32_t(offset),
				.bufferIndex = uint32_t(mBuffers.size()),
				.size = sizeInBytes,
				.vAllocation = vAllocation,
		};

		mBuffers.push_back(
			bufferWithHandles{
				.buffer = newBuffer,
				.vBlock = vBlock,
			}
			);

		mBuffers.back().bufferHandles.insert(handle);

		mNeedUpdate = true;

		return handle;
	}

	withError<bufferHandle> bufferRegistry::findBlock(uint32_t id)
	{
		for (uint32_t i = 0; i < mBuffers.size(); i++)
		{
			if (auto handle = mBuffers[i].bufferHandles.find(bufferHandle{ .id = id }); handle != mBuffers[i].bufferHandles.end())
				return *handle;
		}

		return error{ "block not found" };
	}

	error bufferRegistry::updateBlock(uint32_t id, const void* data, size_t sizeInBytes, submit& is)
	{
		for (uint32_t i = 0; i < mBuffers.size(); i++)
		{
			if (auto handle = mBuffers[i].bufferHandles.find(bufferHandle{ .id = id }); handle != mBuffers[i].bufferHandles.end())
			{
				mBuffers[i].buffer.markBytesAsDead(sizeInBytes);

				error err = mBuffers[i].buffer.updateBuffer(is, data, sizeInBytes, handle->offset);
				if (err)
					return err;
			}
		}

		return {};
	}

	bool bufferRegistry::deleteBlock(uint32_t id)
	{
		for (uint32_t i = 0; i < mBuffers.size(); i++)
		{
			if (auto found = mBuffers[i].bufferHandles.find(bufferHandle{ .id = id }); found != mBuffers[i].bufferHandles.end())
			{
				vmaVirtualFree(mBuffers[i].vBlock, found->vAllocation);
				mBuffers[i].buffer.markBytesAsDead(found->size);
				mBuffers[i].bufferHandles.erase(bufferHandle{ .id = id });

				return true;
			}
		}

		return false;
	}

	void bufferRegistry::destroy()
	{
		for (auto& b : mBuffers)
			b.buffer.destroy();

		mBuffers.clear();
	}

	std::vector<VkWriteDescriptorSet> bufferRegistry::getWriteInfo(uint32_t binding)
	{
		mBuffersInfo.clear();

		for (auto& b : mBuffers)
		{
			mBuffersInfo.push_back(
				VkDescriptorBufferInfo{ .buffer = b.buffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE }
			);
		}

		return descriptorSet::getWriteInfo(binding, mBuffersInfo);
	}

	void bufferRegistry::setUpdated()
	{
		mNeedUpdate = false;
	}

	bool bufferRegistry::needDescriptorUpdate() const
	{
		return mNeedUpdate;
	}

	error pipelineData::init(
		VkDevice device,
		std::shared_ptr<const shader> pixelShader,
		std::shared_ptr<const shader> meshShader,
		std::shared_ptr<const shader> taskShader,
		const std::vector<VkDescriptorSetLayout>& descriptorSets,
		VkFormat depthFormat,
		const std::vector<VkFormat>& colorAttachmentFormats,
		VkSampleCountFlagBits sampleCount,
		pipelineType type
	)
	{
		needUpdate = false;

		VkPushConstantRange pc{};
		pc.offset = 0;
		pc.size = sizeof(pushConstants);
		pc.stageFlags = VK_SHADER_STAGE_ALL;

		// init pipeline.
		pipeline.init(device);

		//connecting the vertex and pixel shaders to the pipeline
		pipeline.setShaders(
			static_cast<vulkanShader*>(const_cast<shader*>(taskShader.get()))->mShaderModule,
			static_cast<vulkanShader*>(const_cast<shader*>(meshShader.get()))->mShaderModule,
			static_cast<vulkanShader*>(const_cast<shader*>(pixelShader.get()))->mShaderModule
		);

		pipeline.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pipeline.setPolygonMode(VK_POLYGON_MODE_FILL);

		// Back face culling is done in shaders.
		pipeline.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		pipeline.setMultisampling(sampleCount);

		if (type == pipelineType::opaque)
		{
			pipeline.disableBlending();
			pipeline.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
		}

		if (type == pipelineType::accumilation)
		{
			pipeline.enableBlendingOITAccumulation();
			pipeline.enableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
		}

		if (type == pipelineType::composite)
		{
			pipeline.enableBlendingOITComposite();
			pipeline.disableDepthtest();
		}

		//connect the image format we will draw into, from draw image
		pipeline.setColorAttachmentFormats(colorAttachmentFormats);
		pipeline.setDepthFormat(depthFormat);

		error err = pipeline.build(&pc, descriptorSets, true);
		if (err)
			return err;

		return {};
	}

	void pipelineData::destroy()
	{
		pipeline.destroy();
	}

	error pipelineRegistry::init(VkDevice device, VmaAllocator allocator, submit& is)
	{
		mNeedOpaqueDescriptorUpdate = true;
		mNeedAccumilationDescriptorUpdate = true;

		mCmdOpaqueBufferNewSize = 2 << 21;
		mCmdOpaqueBuffer.init(device, allocator);

		error err = mCmdOpaqueBuffer.build(is, nullptr, mCmdOpaqueBufferNewSize, 0);
		if (err)
			return err;

		mCmdAccumilationBufferNewSize = 2 << 21;

		mCmdAccumilationBuffer.init(device, allocator);

		err = mCmdAccumilationBuffer.build(is, nullptr, mCmdAccumilationBufferNewSize, 0);
		if (err)
			return err;

		mCompositePipeline = {};
		mAccumilatePipeline = {};

		return {};
	}

	error pipelineRegistry::initAccumilatePipeline(
		VkDevice device,
		std::shared_ptr<const shader> pixelShader,
		std::shared_ptr<const shader> meshShader,
		std::shared_ptr<const shader> taskShader,
		const std::vector<VkDescriptorSetLayout>& descriptorSets,
		VkFormat depthFormat,
		const std::initializer_list<VkFormat>& colorAttachmentFormats,
		graphicsPreset preset
	)
	{
		return mAccumilatePipeline.init(
			device,
			pixelShader,
			meshShader,
			taskShader,
			descriptorSets,
			depthFormat,
			colorAttachmentFormats,
			sampleCounts(preset.msaa),
			pipelineData::pipelineType::accumilation
		);
	}

	error pipelineRegistry::initCompositePipeline(
		VkDevice device,
		std::shared_ptr<const shader> pixelShader,
		std::shared_ptr<const shader> meshShader,
		std::shared_ptr<const shader> taskShader,
		const std::vector<VkDescriptorSetLayout>& descriptorSets,
		VkFormat depthFormat,
		const std::initializer_list<VkFormat>& colorAttachmentFormats,
		graphicsPreset preset
	)
	{
		return mCompositePipeline.init(
			device,
			pixelShader,
			meshShader,
			taskShader,
			descriptorSets,
			depthFormat,
			colorAttachmentFormats,
			sampleCounts(preset.msaa),
			pipelineData::pipelineType::composite
		);
	}

	void pipelineRegistry::destroy()
	{
		for (auto& [_, p] : mPipelines)
			p.destroy();

		mPipelines.clear();

		mCmdOpaqueBuffer.destroy();
		mCmdAccumilationBuffer.destroy();
	}

	error pipelineRegistry::createPipeline(
		VkDevice device,
		std::shared_ptr<const shader> pixelShader,
		std::shared_ptr<const shader> meshShader,
		std::shared_ptr<const shader> taskShader,
		const std::vector<VkDescriptorSetLayout>& descriptorSets,
		VkFormat depthFormat,
		const std::initializer_list<VkFormat>& colorAttachmentFormats,
		graphicsPreset preset
	)
	{
		if (mPipelines.find(pixelShader->hash()) != mPipelines.end())
			return {};

		pipelineData pipeline{};

		error err = pipeline.init(
			device,
			pixelShader,
			meshShader,
			taskShader,
			descriptorSets,
			depthFormat,
			colorAttachmentFormats,
			sampleCounts(preset.msaa),
			pipelineData::pipelineType::opaque
		);
		if (err)
			return err;

		mPipelines.insert({ pixelShader->hash(), pipeline });

		return {};
	}

	error pipelineRegistry::addInstance(const pipelineRegistry::addInstanceParams& params)
	{
		// Always add every geometry to opaque pipeline.
		error err = addOpaqueInstance(params);
		if (err)
			return err;

		// Also add to transperent pass geometry that have blend materials.
		if (params.isBlendGeometry)
		{
			err = addBlendInstance(params);
			if (err)
				return err;
		}

		return {};
	}

	void pipelineRegistry::removeInstance(uint32_t pixelShaderID, uint32_t instanceID, uint32_t meshID)
	{
		removeOpaqueInstance(pixelShaderID, instanceID, meshID);
		removeBlendInstance(instanceID, meshID);
	}

	std::vector<VkWriteDescriptorSet> pipelineRegistry::getOpaqueCmdBufferWriteInfo(uint32_t binding)
	{
		mOpaqueBufferInfo = { VkDescriptorBufferInfo{.buffer = mCmdOpaqueBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE } };

		return descriptorSet::getWriteInfo(binding, mOpaqueBufferInfo);
	}

	std::vector<VkWriteDescriptorSet> pipelineRegistry::getAccumilationCmdBufferWriteInfo(uint32_t binding)
	{
		mAccumilationsBufferInfo = { VkDescriptorBufferInfo{.buffer = mCmdAccumilationBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE } };

		return descriptorSet::getWriteInfo(binding, mAccumilationsBufferInfo);
	}

	bool pipelineRegistry::instanceExists(uint32_t id) const
	{
		for (auto& [_, p] : mPipelines)
		{
			if (p.entityCmd.find(id) != p.entityCmd.end())
				return true;
		}

		return false;
	}

	bool pipelineRegistry::meshIsUsed(uint32_t id) const
	{
		for (auto& [_, p] : mPipelines)
		{
			if (auto found = p.instanceMeshCount.find(id); found != p.instanceMeshCount.end() && found->second != 0)
				return true;
		}

		return false;
	}

	const std::vector<pipelineData::taskShaderRender> pipelineRegistry::getOpaquePipelines() const
	{
		std::vector<pipelineData::taskShaderRender> pipelineMappings;
		for (auto& [_, v] : mPipelines)
			pipelineMappings.push_back(v.mCmdMapping);

		return pipelineMappings;
	}

	const classicGraphicPipeline pipelineRegistry::getCompositePipeline() const
	{
		return mCompositePipeline.pipeline;
	}

	const pipelineData::taskShaderRender pipelineRegistry::getAccumilationPipeline() const
	{
		return mAccumilatePipeline.mCmdMapping;
	}

	error pipelineRegistry::updateOpaqueCmdBuffer(submit& is)
	{
		bool needBufferUpdate = false;
		for (auto& [_, p] : mPipelines)
			needBufferUpdate |= p.needUpdate;

		if (!needBufferUpdate)
			return {};

		uint32_t startOffset = 0;
		uint32_t endOffset = 0;
		std::vector<meshletShaderCMD> cmd;
		for (auto& [k, p] : mPipelines)
		{
			for (auto& [_, v] : p.entityCmd)
			{
				cmd.insert(cmd.end(), v.begin(), v.end());
				endOffset += uint32_t(v.size());
			}

			p.mCmdMapping = pipelineData::taskShaderRender{
				.pipeline = p.pipeline.getPipeline().first,
				.layout = p.pipeline.getPipeline().second,
				.cmdPipelineStartOffset = startOffset,
				.cmdPipelineEndOffset = endOffset,
			};

			startOffset = endOffset;
		}

		mCmdOpaqueBuffer.markBytesAsDead(mCmdOpaqueBuffer.getLoadedBytes());

		error err = mCmdOpaqueBuffer.updateBuffer(is, cmd.data(), cmd.size() * sizeof(meshletShaderCMD), 0);
		if (err && err.is(errCodeBufferOverFlow))
		{
			mCmdOpaqueBufferNewSize = uint32_t(float(mCmdOpaqueBufferNewSize) * 1.5f);

			if (cmd.size() > mCmdOpaqueBufferNewSize)
				mCmdOpaqueBufferNewSize = uint32_t(cmd.size());

			mCmdOpaqueBuffer.destroy();

			err = mCmdOpaqueBuffer.build(is, cmd.data(), mCmdOpaqueBufferNewSize, cmd.size() * sizeof(meshletShaderCMD));
			if (err)
				return err;

			mNeedOpaqueDescriptorUpdate = true;
		}
		if (err)
			return err;

		for (auto& [_, p] : mPipelines)
			p.needUpdate = false;

		return {};
	}

	error pipelineRegistry::updateAccumilationCmdBuffer(submit& is)
	{
		if (!mAccumilatePipeline.needUpdate)
			return {};

		uint32_t endOffset = 0;
		std::vector<meshletShaderCMD> cmd;
		for (auto& [_, v] : mAccumilatePipeline.entityCmd)
		{
			cmd.insert(cmd.end(), v.begin(), v.end());
			endOffset += uint32_t(v.size());
		}

		mAccumilatePipeline.mCmdMapping = pipelineData::taskShaderRender{
			.pipeline = mAccumilatePipeline.pipeline.getPipeline().first,
			.layout = mAccumilatePipeline.pipeline.getPipeline().second,
			.cmdPipelineStartOffset = 0,
			.cmdPipelineEndOffset = endOffset,
		};

		mCmdAccumilationBuffer.markBytesAsDead(mCmdAccumilationBuffer.getLoadedBytes());

		error err = mCmdAccumilationBuffer.updateBuffer(is, cmd.data(), cmd.size() * sizeof(meshletShaderCMD), 0);
		if (err && err.is(errCodeBufferOverFlow))
		{
			mCmdAccumilationBufferNewSize = uint32_t(float(mCmdAccumilationBufferNewSize) * 1.5f);

			if (cmd.size() > mCmdAccumilationBufferNewSize)
				mCmdAccumilationBufferNewSize = uint32_t(cmd.size());

			mCmdAccumilationBuffer.destroy();

			err = mCmdAccumilationBuffer.build(is, cmd.data(), mCmdAccumilationBufferNewSize, cmd.size() * sizeof(meshletShaderCMD));
			if (err)
				return err;

			mNeedAccumilationDescriptorUpdate = true;
		}
		if (err)
			return err;

		mAccumilatePipeline.needUpdate = false;

		return {};
	}

	error materialRegistry::init(VkSampler sampler, materialTextures defaultMat)
	{
		mDefaultMat = defaultMat;
		mNeedUpdate = false;
		mSampler = sampler;

		VmaVirtualBlockCreateInfo vBlockInfo{
			.size = VkDeviceSize(2 << 30),
		};

		if (vmaCreateVirtualBlock(&vBlockInfo, &mVBlock) != VK_SUCCESS)
			return error{ "can't create virtual block" };

		return {};
	}

	void materialRegistry::destroy()
	{
		mDefaultMat.albedo.reset();
		mDefaultMat.normal.reset();
		mDefaultMat.metallicRoughness.reset();
	}

	bool pipelineRegistry::needOpaqueDescriptorUpdate() const
	{
		return mNeedOpaqueDescriptorUpdate;
	}

	bool pipelineRegistry::needAccumilationDescriptorUpdate() const
	{
		return mNeedAccumilationDescriptorUpdate;
	}

	void pipelineRegistry::setOpaqueUpdated()
	{
		mNeedOpaqueDescriptorUpdate = false;
	}

	void pipelineRegistry::setAccumilationUpdated()
	{
		mNeedAccumilationDescriptorUpdate = false;
	}

	error pipelineRegistry::addOpaqueInstance(const addInstanceParams& params)
	{
		if (mPipelines.find(params.pixelShaderID) == mPipelines.end())
			return { "pipeline doesn't exist" };

		pipelineData& pipeline = mPipelines[params.pixelShaderID];

		if (pipeline.entityCmd.find(params.instanceID) != pipeline.entityCmd.end())
			return {};

		pipeline.needUpdate = true;

		for (auto& m : params.meshesData)
		{
			pipeline.instanceMeshCount[m.meshID]++;

			uint32_t baseOffset = m.meshletHandle.offset / uint32_t(sizeof(meshlet));

			for (uint32_t i = 0; i < m.meshlets.second; i++)
			{
				pipeline.entityCmd[params.instanceID].push_back(
					meshletShaderCMD{
						.instanceIndex = params.perInstanceHandle.bufferIndex,
						.instanceOffset = params.perInstanceHandle.offset / uint32_t(sizeof(perInstanceAttr)),
						.meshletIndex = m.meshletHandle.bufferIndex,
						.meshletOffset1 = baseOffset + i,
						.meshletOffset2 = i < m.meshlets.third - m.meshlets.second ? baseOffset + i + m.meshlets.second : std::numeric_limits<uint32_t>::max(),
						.meshletOffset3 = i < m.meshlets.fourth - m.meshlets.third ? baseOffset + i + m.meshlets.third : std::numeric_limits<uint32_t>::max(),
						.meshletOffset4 = i < m.meshlets.data.size() - m.meshlets.fourth ? baseOffset + i + m.meshlets.fourth : std::numeric_limits<uint32_t>::max()
					}
				);
			}
		}

		return {};
	}

	error pipelineRegistry::addBlendInstance(const addInstanceParams& params)
	{
		if (mAccumilatePipeline.entityCmd.find(params.instanceID) != mAccumilatePipeline.entityCmd.end())
			return {};

		mAccumilatePipeline.needUpdate = true;

		for (auto& m : params.meshesData)
		{
			mAccumilatePipeline.instanceMeshCount[m.meshID]++;

			uint32_t baseOffset = m.meshletHandle.offset / uint32_t(sizeof(meshlet));

			for (uint32_t i = 0; i < m.meshlets.second; i++)
			{
				mAccumilatePipeline.entityCmd[params.instanceID].push_back(
					meshletShaderCMD{
						.instanceIndex = params.perInstanceHandle.bufferIndex,
						.instanceOffset = params.perInstanceHandle.offset / uint32_t(sizeof(perInstanceAttr)),
						.meshletIndex = m.meshletHandle.bufferIndex,
						.meshletOffset1 = baseOffset + i,
						.meshletOffset2 = i < m.meshlets.third - m.meshlets.second ? baseOffset + i + m.meshlets.second : std::numeric_limits<uint32_t>::max(),
						.meshletOffset3 = i < m.meshlets.fourth - m.meshlets.third ? baseOffset + i + m.meshlets.third : std::numeric_limits<uint32_t>::max(),
						.meshletOffset4 = i < m.meshlets.data.size() - m.meshlets.fourth ? baseOffset + i + m.meshlets.fourth : std::numeric_limits<uint32_t>::max()
					}
				);
			}
		}

		return {};
	}

	void pipelineRegistry::removeOpaqueInstance(uint32_t pixelShaderID, uint32_t instanceID, uint32_t meshID)
	{
		if (mPipelines.find(pixelShaderID) == mPipelines.end())
			return;

		pipelineData& pipeline = mPipelines[pixelShaderID];

		if (pipeline.entityCmd.find(instanceID) == pipeline.entityCmd.end())
			return;

		pipeline.entityCmd.erase(instanceID);

		if (auto found = pipeline.instanceMeshCount.find(meshID); found != pipeline.instanceMeshCount.end() && found->second != 0)
			found->second--;

		pipeline.needUpdate = true;
	}

	void pipelineRegistry::removeBlendInstance(uint32_t instanceID, uint32_t meshID)
	{
		if (mAccumilatePipeline.entityCmd.find(instanceID) == mAccumilatePipeline.entityCmd.end())
			return;

		mAccumilatePipeline.entityCmd.erase(instanceID);

		if (auto found = mAccumilatePipeline.instanceMeshCount.find(meshID); found != mAccumilatePipeline.instanceMeshCount.end() && found->second != 0)
			found->second--;

		mAccumilatePipeline.needUpdate = true;
	}

	withError<uint32_t> materialRegistry::addMaterials(const materials& materials)
	{
		if (auto found = mUploadedMaterials.find(materials.hash); found != mUploadedMaterials.end())
		{
			return found->second.offset;
		}
		else
		{
			VkDeviceSize offset;
			VmaVirtualAllocation allocation;

			VmaVirtualAllocationCreateInfo allocateInfo{
				.size = materials.textures.size() * 3,
			};

			if (VkResult res = vmaVirtualAllocate(mVBlock, &allocateInfo, &allocation, &offset); res != VK_SUCCESS)
				return error{ "materialRegistry::addMaterials: {}", vkResultToStr(res) };

			mUploadedMaterials[materials.hash] = materialRegistry::virtualTextureBlock{
				.offset = uint32_t(offset),
				.size = uint32_t(allocateInfo.size),
				.allocation = allocation,
			};

			mNeedUpdate = true;
			if (offset + materials.textures.size() * 3 > mImagesInfo.size())
			{
				mImagesInfo.resize(offset + materials.textures.size() * 3);
			}

			VkDescriptorImageInfo info{};
			info.sampler = mSampler;
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			uint32_t uploadOffset = uint32_t(offset);
			for (auto& t : materials.textures)
			{
				info.imageView = static_cast<const vulkanTexture*>(t.albedo.get())->mImage.img.view;
				mImagesInfo[uploadOffset] = info;
				uploadOffset++;

				info.imageView = static_cast<const vulkanTexture*>(t.normal.get())->mImage.img.view;
				mImagesInfo[uploadOffset] = info;
				uploadOffset++;

				info.imageView = static_cast<const vulkanTexture*>(t.metallicRoughness.get())->mImage.img.view;
				mImagesInfo[uploadOffset] = info;
				uploadOffset++;
			}

			return uint32_t(offset);
		}
	}

	withError<uint32_t> materialRegistry::getMaterialsOffset(const materials& materials)
	{
		if (auto found = mUploadedMaterials.find(materials.hash); found != mUploadedMaterials.end())
		{
			return found->second.offset;
		}

		return error{ "Materils not found" };
	}

	void materialRegistry::deleteMaterials(const materials& materials)
	{
		if (auto found = mUploadedMaterials.find(materials.hash); found != mUploadedMaterials.end())
		{
			// Still need to update descripts, because we can get error if textures get deleted later.
			mNeedUpdate = true;

			vmaVirtualFree(mVBlock, found->second.allocation);

			// Set freed materials to default, because they can get deleted and descriptor will be invalidated.
			for (uint32_t i = found->second.offset; i < found->second.offset + found->second.size; i += 3)
			{
				mImagesInfo[i].imageView = static_cast<const vulkanTexture*>(mDefaultMat.albedo.get())->mImage.img.view;
				mImagesInfo[i + 1].imageView = static_cast<const vulkanTexture*>(mDefaultMat.normal.get())->mImage.img.view;
				mImagesInfo[i + 2].imageView = static_cast<const vulkanTexture*>(mDefaultMat.metallicRoughness.get())->mImage.img.view;
			}

			mUploadedMaterials.erase(materials.hash);
		}
	}

	void materialRegistry::setUpdated()
	{
		mNeedUpdate = false;
	}

	bool materialRegistry::needDescriptorUpdate() const
	{
		return mNeedUpdate;
	}

	std::vector<VkWriteDescriptorSet> materialRegistry::getWriteInfo(uint32_t binding)
	{
		return descriptorSet::getWriteInfo(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mImagesInfo);
	}
}
