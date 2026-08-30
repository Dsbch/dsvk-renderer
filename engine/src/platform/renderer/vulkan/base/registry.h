#pragma once
#include <pch.h>

#include "platform/renderer/vertex.h"
#include "platform/renderer/renderer.h"

#include "buffer.h"
#include "submit.h"
#include "image.h"
#include "shader.h"

namespace engine
{
	static const uint32_t newBufferSize = 2 << 27;

	struct bufferHandle
	{
		uint32_t id;
		uint32_t offset;
		uint32_t bufferIndex;
		size_t size;
		VmaVirtualAllocation vAllocation;

		bool operator<(const bufferHandle& other) const
		{
			return id < other.id;
		}
	};

	struct bufferWithHandles
	{
		std::vector<vulkanBuffer> buffer;
		VmaVirtualBlock vBlock;
		std::set<bufferHandle> bufferHandles;
	};

	struct bufferRegistry
	{
	public:
		bufferRegistry() = default;
		bufferRegistry(const bufferRegistry&) = delete;

		void init(VkDevice device, VmaAllocator allocator, vulkanBuffer::mapFlags flags = {false, false}, uint32_t buffersPerBlock = 1);
		withError<bufferHandle> addBlock(uint32_t id, const void* data, size_t sizeInBytes, submit& is, size_t newSize = newBufferSize);
		withError<bufferHandle> findBlock(uint32_t id);
		error updateBlock(uint32_t id, const void* data, size_t sizeInBytes, submit& is, uint32_t frameIndex = 0);
		bool scheduleDeleteBlock(uint32_t id, uint32_t frameIndex);
		void deleteScheduledBlocks(uint32_t frameIndex);
		
		void destroy();
		void setUpdated();
		bool needDescriptorUpdate() const;

		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);
		std::vector<VkDescriptorBufferInfo> getBufferInfo();
	private:
		bool deleteBlock(uint32_t id);

		uint32_t mBuffersPerBlock;
		vulkanBuffer::mapFlags mBufferMapFlags;
		std::vector<bufferWithHandles> mBuffers;
		std::vector<VkDescriptorBufferInfo> mBuffersInfo;

		std::map<uint32_t, std::set<uint32_t>> mBlockScheduledToDelete;

		VkDevice mDevice;
		VmaAllocator mAllocator;

		bool mNeedUpdate;
	};

	struct materialRegistry
	{
	public:
		materialRegistry() = default;
		materialRegistry(const materialRegistry&) = delete;

		error init(VkSampler sampler, materialTextures defaultMat);
		void destroy();
		void setUpdated();
		bool needDescriptorUpdate() const;

		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);
		std::vector<VkDescriptorImageInfo> getImagesInfo();

		withError<uint32_t> addMaterials(const materials& materials);
		withError<uint32_t> getMaterialsOffset(const materials& materials);
		void scheduleDeleteMaterials(uint32 hash, uint32_t frameIndex);
		void deleteScheduledMaterials(uint32_t frameIndex);
	private:
		void deleteMaterials(uint32 hash);

		std::map<uint32_t, std::set<uint32_t>> mMaterialsScheduledToDelete;

		materialTextures mDefaultMat;

		VmaVirtualBlock mVBlock;

		struct virtualTextureBlock
		{
			uint32_t offset;
			uint32_t size;
			VmaVirtualAllocation allocation;
		};
		std::map<textureHash, virtualTextureBlock> mUploadedMaterials;

		std::vector<VkDescriptorImageInfo> mImagesInfo;
		VkSampler mSampler;
		bool mNeedUpdate;
	};
}
