#include <pch.h>

#include "registry.h"
#include "descriptorSet.h"
#include "texture.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	void bufferRegistry::init(VkDevice device, VmaAllocator allocator)
	{
		mDevice = device;
		mAllocator = allocator;
		mNeedUpdate = false;
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
		newBuffer.init(mDevice, mAllocator);

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
		std::shared_ptr<shader> pixelShader,
		std::shared_ptr<shader> meshShader,
		std::shared_ptr<shader> taskShader,
		const std::vector<VkDescriptorSetLayout>& descriptorSets,
		VkFormat depthFormat,
		VkFormat colorAttachmentFormat,
		VkSampleCountFlagBits sampleCount
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
			static_cast<vulkanShader*>(taskShader.get())->mShaderModule,
			static_cast<vulkanShader*>(meshShader.get())->mShaderModule,
			static_cast<vulkanShader*>(pixelShader.get())->mShaderModule
		);

		pipeline.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pipeline.setPolygonMode(VK_POLYGON_MODE_FILL);

		// Back face culling is done in shaders.
		pipeline.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		pipeline.setMultisampling(sampleCount);

		pipeline.disableBlending();

		pipeline.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

		//connect the image format we will draw into, from draw image
		pipeline.setColorAttachmentFormat(colorAttachmentFormat);
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
		mNeedDescriptorUpdate = true;

		mCmdBufferNewSize = 2 << 21;
		mCmdBuffer.init(device, allocator);

		error err = mCmdBuffer.build(is, nullptr, mCmdBufferNewSize, 0);
		if (err)
			return err;

		return {};
	}

	void pipelineRegistry::destroy()
	{
		for (auto& [_, p] : mPipelines)
			p.destroy();

		mPipelines.clear();

		mCmdBuffer.destroy();
	}

	error pipelineRegistry::createPipeline(
		VkDevice device,
		std::shared_ptr<shader> pixelShader,
		std::shared_ptr<shader> meshShader,
		std::shared_ptr<shader> taskShader,
		const std::vector<VkDescriptorSetLayout>& descriptorSets,
		VkFormat depthFormat,
		VkFormat colorAttachmentFormat,
		graphicsPreset preset
	)
	{
		if (mPipelines.find(pixelShader->hash()) != mPipelines.end())
			return {};

		pipelineData pipeline{};

		error err = pipeline.init(device, pixelShader, meshShader, taskShader, descriptorSets, depthFormat, colorAttachmentFormat, sampleCounts(preset.msaa));
		if (err)
			return err;

		mPipelines.insert({ pixelShader->hash(), pipeline });

		return {};
	}

	error pipelineRegistry::addInstance(uint32_t pixelShaderID, uint32_t instanceID, uint32_t meshID, bufferHandle meshletHandle, bufferHandle perInstanceHandle, const dataWithLodLevels<meshlet>& mesh)
	{
		if (mPipelines.find(pixelShaderID) == mPipelines.end())
			return { "pipeline doesn't exist" };

		pipelineData& pipeline = mPipelines[pixelShaderID];

		if (pipeline.entityCmd.find(instanceID) != pipeline.entityCmd.end())
			return {};

		pipeline.needUpdate = true;

		pipeline.instanceMeshCount[meshID]++;

		std::vector<meshletShaderCMD> meshCMD;

		uint32_t baseOffset = meshletHandle.offset / uint32_t(sizeof(meshlet));

		for (uint32_t i = 0; i < mesh.second; i++)
		{
			meshCMD.push_back(
				meshletShaderCMD{
					.instanceIndex = perInstanceHandle.bufferIndex,
					.instanceOffset = perInstanceHandle.offset / uint32_t(sizeof(perInstanceAttr)),
					.meshletIndex = meshletHandle.bufferIndex,
					.meshletOffset1 = baseOffset + i,
					.meshletOffset2 = i < mesh.third - mesh.second ? baseOffset + i + mesh.second : std::numeric_limits<uint32_t>::max(),
					.meshletOffset3 = i < mesh.fourth - mesh.third ? baseOffset + i + mesh.third : std::numeric_limits<uint32_t>::max(),
					.meshletOffset4 = i < mesh.data->size() - mesh.fourth ? baseOffset + i + mesh.fourth : std::numeric_limits<uint32_t>::max()
				}
			);
		}

		pipeline.entityCmd[instanceID] = meshCMD;

		return {};
	}

	void pipelineRegistry::removeInstance(uint32_t pixelShaderID, uint32_t instanceID, uint32_t meshID)
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

	std::vector<VkWriteDescriptorSet> pipelineRegistry::getWriteInfo(uint32_t binding)
	{
		mBufferInfo = { VkDescriptorBufferInfo{.buffer = mCmdBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE } };

		return descriptorSet::getWriteInfo(binding, mBufferInfo);
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
			if (p.instanceMeshCount.find(id) != p.instanceMeshCount.end())
				return true;
		}

		return false;
	}

	const std::map<pixelShaderHash, pipelineRegistry::taskShaderRender> pipelineRegistry::getPipelines() const
	{
		return mCmdMappings;
	}

	error pipelineRegistry::updateCommandBuffer(submit& is)
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

			mCmdMappings[k] = taskShaderRender{
				.pipeline = p.pipeline.getPipeline().first,
				.layout = p.pipeline.getPipeline().second,
				.cmdPipelineStartOffset = startOffset,
				.cmdPipelineEndOffset = endOffset,
			};

			startOffset += endOffset;
		}

		mCmdBuffer.markBytesAsDead(mCmdBuffer.getLoadedBytes());

		error err = mCmdBuffer.updateBuffer(is, cmd.data(), cmd.size() * sizeof(meshletShaderCMD), 0);
		if (err.err() == "buffer overflow")
		{
			mCmdBufferNewSize = uint32_t(float(mCmdBufferNewSize) * 1.5f);

			if (cmd.size() > mCmdBufferNewSize)
				mCmdBufferNewSize = uint32_t(cmd.size());

			mCmdBuffer.destroy();

			err = mCmdBuffer.build(is, cmd.data(), mCmdBufferNewSize, cmd.size() * sizeof(meshletShaderCMD));
			if (err)
				return err;

			mNeedDescriptorUpdate = true;
		}
		if (err)
			return err;

		for (auto& [_, p] : mPipelines)
			p.needUpdate = false;

		return {};
	}

	error materialRegistry::init(VkSampler sampler)
	{
		mNeedUpdate = false;
		mSampler = sampler;

		VmaVirtualBlockCreateInfo vBlockInfo{
			.size = 2 << 15,
		};

		if (vmaCreateVirtualBlock(&vBlockInfo, &mVBlock) != VK_SUCCESS)
			return error{ "can't create virtual block" };

		return {};
	}

	bool pipelineRegistry::needDescriptorUpdate() const
	{
		return mNeedDescriptorUpdate;
	}

	void pipelineRegistry::setUpdated()
	{
		mNeedDescriptorUpdate = false;
	}

	// Forms texture positions of each type in one descriptor set.
	// Returns only a startIndex for each texture type, because order is preserved.
	// If you uploaded 20 albedo textures and returned startIndex is 0, then albedo will be from 
	// 0 to 19 with preserved order.
	withError<materialRegistry::materialsOffsets> materialRegistry::addMaterials(const materials& materials)
	{
		materialRegistry::materialsOffsets result{};

		VkDescriptorImageInfo info{};
		info.sampler = mSampler;
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if (auto found = mUploadedTextures.find(materials.albedoHash); found != mUploadedTextures.end() && found->second.count != 0)
		{
			result.albedoStart = found->second.offset;
			found->second.count++;
		}
		else
		{
			VkDeviceSize offset;
			VmaVirtualAllocation allocation;

			VmaVirtualAllocationCreateInfo allocateInfo{
				.size = materials.textures.size(),
			};

			if (VkResult res = vmaVirtualAllocate(mVBlock, &allocateInfo, &allocation, &offset); res != VK_SUCCESS)
				return error{"materialRegistry::addMaterials: {}", vkResultToStr(res)};

			mUploadedTextures[materials.albedoHash] = materialRegistry::virtualTextureBlock{
				.count = 1,
				.offset = uint32_t(offset),
				.allocation = allocation,
			};

			result.albedoStart = uint32_t(offset);
		
			mNeedUpdate = true;
			if (offset + materials.textures.size() > mImagesInfo.size())
			{
				mImagesInfo.resize(mImagesInfo.size() + materials.textures.size());
			}

			for (auto& t : materials.textures)
			{
				info.imageView = static_cast<const vulkanTexture*>(t.albedo.get())->mImage.image.view;
				mImagesInfo[offset] = info;
				offset++;
			}
		}

		if (auto found = mUploadedTextures.find(materials.normalHash); found != mUploadedTextures.end() && found->second.count != 0)
		{
			result.normalStart = found->second.offset;
			found->second.count++;
		}
		else
		{
			VkDeviceSize offset;
			VmaVirtualAllocation allocation;

			VmaVirtualAllocationCreateInfo allocateInfo{
				.size = materials.textures.size(),
			};

			if (VkResult res = vmaVirtualAllocate(mVBlock, &allocateInfo, &allocation, &offset); res != VK_SUCCESS)
				return error{ "materialRegistry::addMaterials: {}", vkResultToStr(res) };

			mUploadedTextures[materials.normalHash] = materialRegistry::virtualTextureBlock{
				.count = 1,
				.offset = uint32_t(offset),
				.allocation = allocation,
			};

			result.normalStart = uint32_t(offset);

			mNeedUpdate = true;
			if (offset + materials.textures.size() > mImagesInfo.size())
			{
				mImagesInfo.resize(mImagesInfo.size() + materials.textures.size());
			}

			for (auto& t : materials.textures)
			{
				info.imageView = static_cast<const vulkanTexture*>(t.normal.get())->mImage.image.view;
				mImagesInfo[offset] = info;
				offset++;
			}
		}

		if (auto found = mUploadedTextures.find(materials.metallicRoughnessHash); found != mUploadedTextures.end() && found->second.count != 0)
		{
			result.metallicRoughnessStart = found->second.offset;
			found->second.count++;
		}
		else
		{
			VkDeviceSize offset;
			VmaVirtualAllocation allocation;

			VmaVirtualAllocationCreateInfo allocateInfo{
				.size = materials.textures.size(),
			};

			if (VkResult res = vmaVirtualAllocate(mVBlock, &allocateInfo, &allocation, &offset); res != VK_SUCCESS)
				return error{ "materialRegistry::addMaterials: {}", vkResultToStr(res) };

			mUploadedTextures[materials.metallicRoughnessHash] = materialRegistry::virtualTextureBlock{
				.count = 1,
				.offset = uint32_t(offset),
				.allocation = allocation,
			};

			result.metallicRoughnessStart = uint32_t(offset);

			mNeedUpdate = true;
			if (offset + materials.textures.size() > mImagesInfo.size())
			{
				mImagesInfo.resize(mImagesInfo.size() + materials.textures.size());
			}

			for (auto& t : materials.textures)
			{
				info.imageView = static_cast<const vulkanTexture*>(t.metallicRoughness.get())->mImage.image.view;
				mImagesInfo[offset] = info;
				offset++;
			}
		}

		return result;
	}

	void materialRegistry::deleteMaterials(const materials& materials)
	{
		if (auto found = mUploadedTextures.find(materials.albedoHash); found != mUploadedTextures.end())
		{
			found->second.count--;

			if (found->second.count == 0)
			{
				vmaVirtualFree(mVBlock, found->second.allocation);
			}
		}

		if (auto found = mUploadedTextures.find(materials.normalHash); found != mUploadedTextures.end())
		{
			found->second.count--;

			if (found->second.count == 0)
			{
				vmaVirtualFree(mVBlock, found->second.allocation);
			}
		}

		if (auto found = mUploadedTextures.find(materials.metallicRoughnessHash); found != mUploadedTextures.end())
		{
			found->second.count--;

			if (found->second.count == 0)
			{
				vmaVirtualFree(mVBlock, found->second.allocation);
			}
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
