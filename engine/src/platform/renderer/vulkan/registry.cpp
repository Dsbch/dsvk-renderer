#include <pch.h>

#include "registry.h"
#include "vulkanDescriptorSet.h"

namespace engine
{
	void bufferRegistry::init(VkDevice device, VmaAllocator allocator, immediateSubmit immSubmit)
	{
		mDevice = device;
		mAllocator = allocator;
		mImmSubmit = immSubmit;
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
						.vAllocation = vAllocation,
				};

				mBuffers[i].bufferHandles.insert(handle);

				auto err = mBuffers[i].buffer.updateBuffer(mImmSubmit, data, sizeInBytes, offset);
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

		auto err = newBuffer.build(mImmSubmit, data, newSize, sizeInBytes);
		if (err)
			return err;

		auto handle = bufferHandle{
				.id = id,
				.offset = uint32_t(offset),
				.bufferIndex = uint32_t(mBuffers.size()),
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
				mNeedUpdate = true;
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

	bool bufferRegistry::needDecriptorUpdate() const
	{
		return mNeedUpdate;
	}

	void textureRegistry::init(VkSampler sampler)
	{
		mNeedUpdate = false;
		mSampler = sampler;
	}

	uint32_t textureRegistry::addTexture(uint32_t id, const vulkanImage& texture)
	{
		if (auto offset = mUploadedTextures.find(id); offset != mUploadedTextures.end())
			return offset->second;

		mTextures.push_back(texture);
		mUploadedTextures[id] = uint32_t(mTextures.size());
		mNeedUpdate = true;

		return uint32_t(mTextures.size());
	}

	void textureRegistry::deleteTexture(uint32_t offset)
	{
		mTextures.erase(mTextures.begin() + offset);
		mNeedUpdate = true;
	}

	// here destroy does nothing, because ECS will destroy all textures.
	// it's a by product of not storing textures in CPU RAM.
	void textureRegistry::destroy()
	{
	}

	void textureRegistry::setUpdated()
	{
		mNeedUpdate = false;
	}

	bool textureRegistry::needDecriptorUpdate() const
	{
		return mNeedUpdate;
	}

	std::vector<VkWriteDescriptorSet> textureRegistry::getWriteInfo(uint32_t binding)
	{
		mImagesInfo.clear();

		for (auto& i : mTextures)
		{
			VkDescriptorImageInfo info{};
			info.sampler = mSampler;
			info.imageView = i.image.view;
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			mImagesInfo.push_back(info);
		}

		return descriptorSet::getWriteInfo(binding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mImagesInfo);
	}
}
