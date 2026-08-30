#include <pch.h>
#include "commandBuffer.h"

namespace engine
{
	void commandBuffer::init(VkDevice device, VmaAllocator allocator, submit& is, uint32_t framesInFlight)
	{
		mNeedDescriptorUpdate = true;

		mFramesInFlight = framesInFlight;

		mCmdBuffer.resize(framesInFlight);
		mEntitiesToDelete.resize(framesInFlight);
		mEntitiesToAdd.resize(framesInFlight);
		mUploadedEntities.resize(framesInFlight);
		mMeshCount.resize(framesInFlight);

		mCmdBufferSize = 2 << 24;

		// Buffer is mapped and CPU readback if turned off to faster access.
		mBufferMapFlags = { true, false };

		for (uint32_t i = 0; i < mFramesInFlight; i++)
		{
			mCmdBuffer[i].init(device, allocator, mBufferMapFlags);
		}
	}

	error commandBuffer::build(submit& is)
	{
		for (uint32_t i = 0; i < mFramesInFlight; i++)
		{
			error err = mCmdBuffer[i].build(is, nullptr, mCmdBufferSize, 0);
			if (err)
				return err;
		}

		return {};
	}

	void commandBuffer::destroy()
	{
		for (auto& b : mCmdBuffer)
			b.destroy();

		mCmdBuffer.clear();
	}

	error commandBuffer::addInstance(const addInstanceParams& params)
	{
		if (!mEntitiesToAdd[params.frameIndex].contains(params.instanceID) && !mUploadedEntities[params.frameIndex].contains(params.instanceID))
		{
			for (auto& m : params.meshesData)
			{
				mMeshCount[params.frameIndex][m.meshID]++;

				uint32_t baseOffset = m.meshletHandle.offset / uint32_t(sizeof(meshlet));

				for (uint32_t i = 0; i < m.meshlets.second; i++)
				{
					mEntitiesToAdd[params.frameIndex][params.instanceID].push_back(
						meshletShaderCMD{
							.instanceIndex = params.perInstanceHandle.bufferIndex,
							.instanceOffset = params.perInstanceHandle.offset / uint32_t(sizeof(perInstanceAttr)),
							.meshletIndex = m.meshletHandle.bufferIndex,
							.meshletOffset1 = baseOffset + i,
							.meshletOffset2 = i < m.meshlets.third - m.meshlets.second ? baseOffset + i + m.meshlets.second : std::numeric_limits<uint32_t>::max(),
							.meshletOffset3 = i < m.meshlets.fourth - m.meshlets.third ? baseOffset + i + m.meshlets.third : std::numeric_limits<uint32_t>::max(),
							.meshletOffset4 = i < m.meshlets.data.size() - m.meshlets.fourth ? baseOffset + i + m.meshlets.fourth : std::numeric_limits<uint32_t>::max(),
							.meshIndex = m.meshHandle.bufferIndex,
							.meshOffset = m.meshHandle.offset / uint32_t(sizeof(perMeshAttributes)),
							.visabilityBit = NOT_VISIBLE_FLAG_BIT,
							.selectedLod = 1,
						}
						);
				}
			}
		}

		return {};
	}

	void commandBuffer::removeInstance(const removeInstanceParams& params)
	{
		if (mEntitiesToDelete[params.frameIndex].contains(params.instanceID))
			return;

		if (!mEntitiesToAdd[params.frameIndex].contains(params.instanceID) && !mUploadedEntities[params.frameIndex].contains(params.instanceID))
			return;

		mEntitiesToDelete[params.frameIndex].insert(params.instanceID);

		if (auto found = mMeshCount[params.frameIndex].find(params.meshID); found != mMeshCount[params.frameIndex].end() && found->second != 0)
			found->second--;
	}

	error commandBuffer::updateCommandBuffer(const updateCommandBufferParams& params)
	{
		std::vector<entityHash> toRemove;

		for (auto& instanceID : mEntitiesToDelete[params.frameIndex])
		{
			if (mEntitiesToAdd[params.frameIndex].contains(instanceID))
				toRemove.push_back(instanceID);
		}

		for (auto& id : toRemove)
		{
			mEntitiesToAdd[params.frameIndex].erase(id);
			mEntitiesToDelete[params.frameIndex].erase(id);
		}

		std::vector<meshletShaderCMD> cmdToAdd{};

		for (auto& [k, v] : mEntitiesToAdd[params.frameIndex])
		{
			if (!mUploadedEntities[params.frameIndex].contains(k))
			{
				size_t size = v.size() * sizeof(meshletShaderCMD);
				size_t offset = mCmdBuffer[params.frameIndex].getLoadedBytes() + cmdToAdd.size() * sizeof(meshletShaderCMD);

				for (auto& c : v)
					cmdToAdd.push_back(c);

				mUploadedEntities[params.frameIndex][k] = { offset, offset + size };
			}
		}

		error err = mCmdBuffer[params.frameIndex].updateBuffer(params.is, cmdToAdd.data(), cmdToAdd.size() * sizeof(meshletShaderCMD), mCmdBuffer[params.frameIndex].getLoadedBytes());
		if (err && err.is(errCodeBufferOverFlow))
		{
			mNeedDescriptorUpdate = true;

			mCmdBufferSize = uint32_t(float(mCmdBufferSize) * 1.5f);
			uint32_t minSize = uint32_t(cmdToAdd.size() * sizeof(meshletShaderCMD) + mCmdBuffer[params.frameIndex].getLoadedBytes());

			if (mCmdBufferSize < minSize)
				mCmdBufferSize = minSize;

			vulkanBuffer newBuf{};

			newBuf.init(params.device, params.allocator, mBufferMapFlags);
			err = newBuf.build(params.is, mCmdBuffer[params.frameIndex], mCmdBufferSize, true);
			if (err)
				return err;

			err = newBuf.updateBuffer(params.is, cmdToAdd.data(), cmdToAdd.size() * sizeof(meshletShaderCMD), mCmdBuffer[params.frameIndex].getLoadedBytes());
			if (err)
				return err;

			mCmdBuffer[params.frameIndex] = std::move(newBuf);
		}

		mEntitiesToAdd[params.frameIndex].clear();

		for (auto& k : mEntitiesToDelete[params.frameIndex])
		{
			auto uploadedEnity = mUploadedEntities[params.frameIndex].find(k);

			if (uploadedEnity != mUploadedEntities[params.frameIndex].end())
			{
				if (mCmdBuffer[params.frameIndex].getLoadedBytes() != uploadedEnity->second.second)
				{
					error err = mCmdBuffer[params.frameIndex].shiftData(params.is, uploadedEnity->second.first, uploadedEnity->second.second);
					if (err)
						return err;
				}
				else
					mCmdBuffer[params.frameIndex].markBytesAsDead(uploadedEnity->second.second - uploadedEnity->second.first);

				size_t deletedSize = uploadedEnity->second.second - uploadedEnity->second.first;

				for (auto& [_, v] : mUploadedEntities[params.frameIndex])
				{
					if (v.first >= uploadedEnity->second.second)
					{
						v.first -= deletedSize;
						v.second -= deletedSize;
					}
				}

				mUploadedEntities[params.frameIndex].erase(k);
			}
		}

		mEntitiesToDelete[params.frameIndex].clear();

		return {};
	}

	bool commandBuffer::meshIsUsed(uint32_t id, uint32_t frameIndex) const
	{
		if (auto found = mMeshCount[frameIndex].find(id); found != mMeshCount[frameIndex].end() && found->second != 0)
			return true;

		return false;
	}

	bool commandBuffer::instanceExists(uint32_t id, uint32_t frameIndex) const
	{
		if (mUploadedEntities[frameIndex].find(id) != mUploadedEntities[frameIndex].end())
			return true;

		return false;
	}

	std::vector<VkDescriptorBufferInfo> commandBuffer::getBufferInfo()
	{
		mBufferInfo.clear();

		for (auto& b : mCmdBuffer)
			mBufferInfo.push_back(VkDescriptorBufferInfo{ .buffer = b.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE });

		return mBufferInfo;
	}

	bool commandBuffer::needDescriptorUpdate() const
	{
		return mNeedDescriptorUpdate;
	}

	void commandBuffer::setUpdated()
	{
		mNeedDescriptorUpdate = false;
	}

	vulkanBuffer commandBuffer::getBuffer(uint32_t frameIndex) const
	{
		return mCmdBuffer[frameIndex];
	}

	uint32_t commandBuffer::getCommandBufferLoadedSize(uint32_t frameIndex) const
	{
		return uint32_t(mCmdBuffer[frameIndex].getLoadedBytes());
	}
}