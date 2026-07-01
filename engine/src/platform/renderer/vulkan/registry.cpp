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
		getBufferInfo();

		return descriptorSet::getWriteInfo(binding, mBuffersInfo);
	}

	std::vector<VkDescriptorBufferInfo> bufferRegistry::getBufferInfo()
	{
		mBuffersInfo.clear();

		for (auto& b : mBuffers)
		{
			mBuffersInfo.push_back(
				VkDescriptorBufferInfo{ .buffer = b.buffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE }
			);
		}

		return mBuffersInfo;
	}

	void bufferRegistry::setUpdated()
	{
		mNeedUpdate = false;
	}

	bool bufferRegistry::needDescriptorUpdate() const
	{
		return mNeedUpdate;
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

	std::vector<VkDescriptorImageInfo> materialRegistry::getImagesInfo()
	{
		return mImagesInfo;
	}
}
