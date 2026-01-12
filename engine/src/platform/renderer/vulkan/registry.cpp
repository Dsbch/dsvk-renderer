#include <pch.h>

#include "registry.h"
#include "descriptorSet.h"
#include "texture.h"

namespace engine
{
	void bufferRegistry::init(VkDevice device, VmaAllocator allocator, submit is)
	{
		mDevice = device;
		mAllocator = allocator;
		mSubmit = is;
		mNeedUpdate = false;
	}

	withError<bufferHandle> bufferRegistry::addBlock(uint32_t id, const void* data, size_t sizeInBytes, size_t newSize)
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

				error err = mBuffers[i].buffer.updateBuffer(mSubmit, data, sizeInBytes, offset);
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

		error err = newBuffer.build(mSubmit, data, newSize, sizeInBytes);
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
		VkFormat colorAttachmentFormat
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

		pipeline.setMultisamplingNone();

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

	error pipelineRegistry::init(VkDevice device, VmaAllocator allocator, submit is)
	{
		mNeedDescriptorUpdate = true;
		mSubmit = is;

		mCmdBufferNewSize = 2 << 21;
		mCmdBuffer.init(device, allocator);

		error err = mCmdBuffer.build(mSubmit, nullptr, mCmdBufferNewSize, 0);
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
		VkFormat colorAttachmentFormat
	)
	{
		if (mPipelines.find(pixelShader->hash()) != mPipelines.end())
			return {};

		pipelineData pipeline{};

		error err = pipeline.init(device, pixelShader, meshShader, taskShader, descriptorSets, depthFormat, colorAttachmentFormat);
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

		if (pipeline.meshletShaderCMD.find(instanceID) != pipeline.meshletShaderCMD.end())
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

		pipeline.meshletShaderCMD[instanceID] = meshCMD;

		return {};
	}

	void pipelineRegistry::removeInstance(uint32_t pixelShaderID, uint32_t instanceID, uint32_t meshID)
	{
		if (mPipelines.find(pixelShaderID) == mPipelines.end())
			return;

		pipelineData& pipeline = mPipelines[pixelShaderID];

		if (pipeline.meshletShaderCMD.find(instanceID) == pipeline.meshletShaderCMD.end())
			return;

		pipeline.meshletShaderCMD.erase(instanceID);

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
			if (p.meshletShaderCMD.find(id) != p.meshletShaderCMD.end())
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

	std::vector<pipelineRegistry::taskShaderRender> pipelineRegistry::getPipelines()
	{
		std::vector<pipelineRegistry::taskShaderRender> result;

		for (auto& [_, p] : mPipelines)
		{
			uint32_t cmdLength = 0;

			for (const auto [_, v] : p.meshletShaderCMD)
			{
				cmdLength += uint32_t(v.size());
			}

			result.push_back(
				pipelineRegistry::taskShaderRender{
					.pipeline = p.pipeline.getPipeline().first,
					.layout = p.pipeline.getPipeline().second,
					.commandBufferLength = cmdLength,
				}
				);
		}

		return result;
	}

	error pipelineRegistry::updateCommandBuffer()
	{
		bool needBufferUpdate = false;
		for (auto& [_, p] : mPipelines)
			needBufferUpdate |= p.needUpdate;

		if (!needBufferUpdate)
			return {};

		std::vector<meshletShaderCMD> cmd;
		for (auto& [_, p] : mPipelines)
		{
			for (auto& [_, v] : p.meshletShaderCMD)
			{
				cmd.insert(cmd.end(), v.begin(), v.end());
			}
		}

		mCmdBuffer.markBytesAsDead(mCmdBuffer.getLoadedBytes());

		error err = mCmdBuffer.updateBuffer(mSubmit, cmd.data(), cmd.size() * sizeof(meshletShaderCMD), 0);
		if (err.err() == "buffer overflow")
		{
			mCmdBufferNewSize = uint32_t(float(mCmdBufferNewSize) * 1.5f);

			if (cmd.size() > mCmdBufferNewSize)
				mCmdBufferNewSize = uint32_t(cmd.size());

			mCmdBuffer.destroy();

			err = mCmdBuffer.build(mSubmit, cmd.data(), cmd.size() * sizeof(meshletShaderCMD), cmd.size() * sizeof(meshletShaderCMD));
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

	void materialRegistry::init(VkSampler sampler)
	{
		mNeedUpdate = false;
		mSampler = sampler;
	}

	bool pipelineRegistry::needDescriptorUpdate() const
	{
		return mNeedDescriptorUpdate;
	}

	void pipelineRegistry::setUpdated()
	{
		mNeedDescriptorUpdate = false;
	}

	materialRegistry::materialOffsets materialRegistry::addMaterial(const materialTextures& textures)
	{
		materialRegistry::materialOffsets result{};

		VkDescriptorImageInfo info{};
		info.sampler = mSampler;
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if (auto foundAlbedo = mOccupiedIndices.find(textures.albedoAtlas->hash()); foundAlbedo != mOccupiedIndices.end())
		{
			result.albedo = foundAlbedo->second;
		}
		else
		{
			mNeedUpdate = true;
			info.imageView = static_cast<const vulkanTexture*>(textures.albedoAtlas.get())->mImage.image.view;

			if (mFreeIndices.size() != 0)
			{
				uint32_t freeIndex = *mFreeIndices.begin();
				mFreeIndices.pop_front();
				mOccupiedIndices[textures.albedoAtlas->hash()] = freeIndex;

				result.albedo = freeIndex;

				mImagesInfo[freeIndex] = info;
			}
			else
			{
				result.albedo = uint32_t(mImagesInfo.size());
				mOccupiedIndices[textures.albedoAtlas->hash()] = result.albedo;
				mImagesInfo.push_back(info);
			}
		}

		if (auto foundNormal = mOccupiedIndices.find(textures.normalAtlas->hash()); foundNormal != mOccupiedIndices.end())
		{
			result.normal = foundNormal->second;
		}
		else
		{
			mNeedUpdate = true;
			info.imageView = static_cast<const vulkanTexture*>(textures.normalAtlas.get())->mImage.image.view;

			if (mFreeIndices.size() != 0)
			{
				uint32_t freeIndex = *mFreeIndices.begin();
				mFreeIndices.pop_front();
				mOccupiedIndices[textures.normalAtlas->hash()] = freeIndex;

				result.normal = freeIndex;

				mImagesInfo[freeIndex] = info;
			}
			else
			{
				result.normal = uint32_t(mImagesInfo.size());
				mOccupiedIndices[textures.normalAtlas->hash()] = result.normal;
				mImagesInfo.push_back(info);
			}
		}

		if (auto foundMetalicRoughnes = mOccupiedIndices.find(textures.metalicRoughnesAtlas->hash()); foundMetalicRoughnes != mOccupiedIndices.end())
		{
			result.metalicRoughnes = foundMetalicRoughnes->second;
		}
		else
		{
			mNeedUpdate = true;
			result.metalicRoughnes = uint32_t(mImagesInfo.size());

			if (mFreeIndices.size() != 0)
			{
				uint32_t freeIndex = *mFreeIndices.begin();
				mFreeIndices.pop_front();
				mOccupiedIndices[textures.metalicRoughnesAtlas->hash()] = freeIndex;

				result.metalicRoughnes = freeIndex;

				mImagesInfo[freeIndex] = info;
			}
			else
			{
				result.metalicRoughnes = uint32_t(mImagesInfo.size());
				mOccupiedIndices[textures.metalicRoughnesAtlas->hash()] = result.metalicRoughnes;
				mImagesInfo.push_back(info);
			}
		}

		mTextureCount[textures.albedoAtlas->hash()]++;
		mTextureCount[textures.normalAtlas->hash()]++;
		mTextureCount[textures.metalicRoughnesAtlas->hash()]++;

		return result;
	}

	void materialRegistry::deleteMaterial(const materialTextures& textures)
	{
		if (auto foundAlbedo = mOccupiedIndices.find(textures.albedoAtlas->hash()); foundAlbedo != mOccupiedIndices.end())
		{
			mTextureCount[foundAlbedo->first]--;

			if (mTextureCount[foundAlbedo->first] == 0)
			{
				mFreeIndices.push_back(foundAlbedo->second);
				mOccupiedIndices.erase(foundAlbedo->first);
			}
		}

		if (auto foundNormal = mOccupiedIndices.find(textures.normalAtlas->hash()); foundNormal != mOccupiedIndices.end())
		{
			mTextureCount[foundNormal->first]--;

			if (mTextureCount[foundNormal->first] == 0)
			{
				mFreeIndices.push_back(foundNormal->second);
				mOccupiedIndices.erase(foundNormal->first);
			}
		}


		if (auto foundMetalicRoughnes = mOccupiedIndices.find(textures.metalicRoughnesAtlas->hash()); foundMetalicRoughnes != mOccupiedIndices.end())
		{
			mTextureCount[foundMetalicRoughnes->first]--;

			if (mTextureCount[foundMetalicRoughnes->first] == 0)
			{
				mFreeIndices.push_back(foundMetalicRoughnes->second);
				mOccupiedIndices.erase(foundMetalicRoughnes->first);
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
