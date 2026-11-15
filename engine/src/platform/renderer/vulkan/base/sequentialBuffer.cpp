#include <pch.h>
#include "sequentialBuffer.h"

#include "descriptorSet.h"

namespace engine
{
	void sequentialBuffer::init(std::shared_ptr<context> ctx, VkDevice device, VmaAllocator allocator, uint32_t framesInFlight, uint32_t bufferSize)
	{
		mNeedDescriptorUpdate = true;

		mCtx = ctx;
		mDevice = device;
		mAllocator = allocator;

		mFramesInFlight = framesInFlight;

		mBufferSize = bufferSize;

		// Buffer is mapped and CPU readback if turned off to faster access.
		mBufferMapFlags = { true, false };

		mBuffers.resize(framesInFlight);
		mUploadedBlocks.resize(framesInFlight);

		for (uint32_t i = 0; i < mFramesInFlight; i++)
			mBuffers[i].init(device, allocator, mBufferMapFlags);
	}

	error sequentialBuffer::build(submit& is)
	{
		for (uint32_t i = 0; i < mFramesInFlight; i++)
		{
			error err = mBuffers[i].build(is, nullptr, mBufferSize, 0);
			if (err)
				return err;
		}

		return {};
	}

	void sequentialBuffer::destroy()
	{
		for (auto& b : mBuffers)
			b.destroy();

		mBuffers.clear();
	}

	error sequentialBuffer::addBlock(UID id, const void* data, size_t sizeInBytes, submit& is, uint32_t frameIndex)
	{
		if (mUploadedBlocks[frameIndex].count(id) != 0)
			return {};

		size_t sizeBeforeUpload = mBuffers[frameIndex].getLoadedBytes();

		error err = mBuffers[frameIndex].updateBuffer(is, data, sizeInBytes, mBuffers[frameIndex].getLoadedBytes());
		if (err && err.is(errCodeBufferOverFlow))
		{
			mNeedDescriptorUpdate = true;

			mBufferSize = uint32_t(float(mBufferSize) * 1.5f);
			uint32_t minSize = uint32_t(sizeInBytes + mBuffers[frameIndex].getLoadedBytes());

			if (mBufferSize < minSize)
				mBufferSize = minSize;

			vulkanBuffer newBuf{};

			newBuf.init(mDevice, mAllocator, mBufferMapFlags);
			err = newBuf.build(is, mBuffers[frameIndex], mBufferSize, true);
			if (err)
				return err;

			err = newBuf.updateBuffer(is, data, sizeInBytes, mBuffers[frameIndex].getLoadedBytes());
			if (err)
				return err;

			mBuffers[frameIndex] = std::move(newBuf);
		}
		else if (err)
			return err;

		mUploadedBlocks[frameIndex][id] = { sizeBeforeUpload, mBuffers[frameIndex].getLoadedBytes() };

		return {};
	}

	error sequentialBuffer::deleteBlock(UID id, submit& is, uint32_t frameIndex)
	{
		auto uploadedEnity = mUploadedBlocks[frameIndex].find(id);

		if (uploadedEnity != mUploadedBlocks[frameIndex].end())
		{
			if (mBuffers[frameIndex].getLoadedBytes() != uploadedEnity->second.second)
			{
				error err = mBuffers[frameIndex].shiftData(is, uploadedEnity->second.first, uploadedEnity->second.second);
				if (err)
					return err;
			}
			else
				mBuffers[frameIndex].markBytesAsDead(uploadedEnity->second.second - uploadedEnity->second.first);

			size_t deletedSize = uploadedEnity->second.second - uploadedEnity->second.first;

			for (auto& [_, v] : mUploadedBlocks[frameIndex])
			{
				if (v.first >= uploadedEnity->second.second)
				{
					v.first -= deletedSize;
					v.second -= deletedSize;
				}
			}

			mUploadedBlocks[frameIndex].erase(id);
		}

		return {};
	}

	error sequentialBuffer::updateBlock(UID id, const void* data, size_t sizeInBytes, submit& is, uint32_t frameIndex)
	{
		auto uploadedEnity = mUploadedBlocks[frameIndex].find(id);

		if (uploadedEnity != mUploadedBlocks[frameIndex].end())
		{
			mBuffers[frameIndex].markBytesAsDead(sizeInBytes);

			return mBuffers[frameIndex].updateBuffer(is, data, sizeInBytes, uploadedEnity->second.first);
		}

		return {};
	}

	std::vector<VkDescriptorBufferInfo> sequentialBuffer::getBufferInfo()
	{
		mBufferInfos.clear();

		for (auto& b : mBuffers)
			mBufferInfos.push_back(VkDescriptorBufferInfo{ .buffer = b.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE });

		return mBufferInfos;
	}

	bool sequentialBuffer::needDescriptorUpdate() const
	{
		return mNeedDescriptorUpdate;
	}

	void sequentialBuffer::setUpdated()
	{
		mNeedDescriptorUpdate = false;
	}

	uint32_t sequentialBuffer::getBufferLoadedSize(uint32_t frameIndex) const
	{
		return uint32_t(mBuffers[frameIndex].getLoadedBytes());
	}
}