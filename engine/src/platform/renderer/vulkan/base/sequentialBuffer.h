#pragma once

#include <pch.h>

#include <vma/vk_mem_alloc.h>

#include "registry.h"
#include "submit.h"
#include "platform/renderer/vertex.h"

namespace engine
{
	struct sequentialBuffer
	{
	public:
		sequentialBuffer() = default;

		sequentialBuffer(const sequentialBuffer&) = delete;

		void init(std::shared_ptr<context> ctx, VkDevice device, VmaAllocator allocator, uint32_t framesInFlight = 1, uint32_t bufferSize = 2 << 24);
		error build(submit& is);
		void destroy();

		error addBlock(UID id, const void* data, size_t sizeInBytes, submit& is, uint32_t frameIndex = 0);
		error updateBlock(UID id, const void* data, size_t sizeInBytes, submit& is, uint32_t frameIndex = 0);
		error deleteBlock(UID id, submit& is, uint32_t frameIndex = 0);

		std::vector<VkDescriptorBufferInfo> getBufferInfo();
		bool needDescriptorUpdate() const;
		void setUpdated();
		uint32_t getBufferLoadedSize(uint32_t frameIndex) const;
	private:
		std::shared_ptr<context> mCtx;
		VkDevice mDevice;
		VmaAllocator mAllocator;

		uint32_t mFramesInFlight;
		uint32_t mBufferSize;

		bool mNeedDescriptorUpdate;

		vulkanBuffer::mapFlags mBufferMapFlags;
		std::vector<vulkanBuffer> mBuffers;
		
		std::vector<std::map<UID, std::pair<size_t, size_t>>> mUploadedBlocks;
		std::vector<VkDescriptorBufferInfo> mBufferInfos;
	};
}