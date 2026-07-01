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
		vulkanBuffer buffer;
		VmaVirtualBlock vBlock;
		std::set<bufferHandle> bufferHandles;
	};

	struct bufferRegistry
	{
	public:
		void init(VkDevice device, VmaAllocator allocator, bool mapped = false);
		withError<bufferHandle> addBlock(uint32_t id, const void* data, size_t sizeInBytes, submit& is, size_t newSize = newBufferSize);
		withError<bufferHandle> findBlock(uint32_t id);
		error updateBlock(uint32_t id, const void* data, size_t sizeInBytes, submit& is);
		bool deleteBlock(uint32_t id);
		void destroy();
		void setUpdated();
		bool needDescriptorUpdate() const;

		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);
		std::vector<VkDescriptorBufferInfo> getBufferInfo();
	private:
		bool mUseMappedBuffers;
		std::vector<bufferWithHandles> mBuffers;
		std::vector<VkDescriptorBufferInfo> mBuffersInfo;

		VkDevice mDevice;
		VmaAllocator mAllocator;

		bool mNeedUpdate;
	};

	struct materialRegistry
	{
	public:
		error init(VkSampler sampler, materialTextures defaultMat);
		void destroy();
		void setUpdated();
		bool needDescriptorUpdate() const;

		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);
		std::vector<VkDescriptorImageInfo> getImagesInfo();

		withError<uint32_t> addMaterials(const materials& materials);
		withError<uint32_t> getMaterialsOffset(const materials& materials);
		void deleteMaterials(const materials& materials);
	private:
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
