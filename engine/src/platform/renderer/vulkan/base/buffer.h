#pragma once

#include <pch.h>

#include <vk_mem_alloc.h>
#include "submit.h"

namespace engine
{
	struct allocatedBuffer
	{
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo info;
		VkDeviceAddress bufferAddress;
	};

	struct vulkanBuffer
	{
	public:
		vulkanBuffer()
			:
			mDevice(VK_NULL_HANDLE),
			mBuffer(
				{
					.buffer = VK_NULL_HANDLE,
					.allocation = VK_NULL_HANDLE,
					.info = {},
				}
				),
			mAllocator(VK_NULL_HANDLE),
			mLoadedBytes(0),
			mByteSize(0)
		{
		}

		struct mapFlags
		{
			bool mapped;
			bool cpuReadBack;
		};

		void init(VkDevice device, VmaAllocator allocator, mapFlags flags = {false, false});
		error build(submit& is, const void* data, size_t sizeInBytes, size_t validBytes, bool dispatchBuffer = false);
		error build(submit& is, vulkanBuffer& buf, size_t sizeInBytes, bool destroyBuffer = false);
		error buildAsUBO(submit& is, const void* data, size_t sizeInBytes, size_t validBytes);
		error updateBuffer(submit& is, const void* data, size_t sizeInBytes, size_t offset);
		error shiftData(submit& is, size_t dstOffset, size_t srcOffset);
		void destroy();

		allocatedBuffer getBuffer() const;

		void markBytesAsDead(size_t size);
		size_t getSize() const;
		size_t getLoadedBytes() const;

		bool needDescriptorUpdate() const;
		void setUpdated();

		static withError<allocatedBuffer> createBuffer(VmaAllocator allocator, VkDevice device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, mapFlags flags = {false, false});
		static void destroyBuffer(VmaAllocator allocator, allocatedBuffer buf);
	private:
		VkDevice mDevice;
		VmaAllocator mAllocator;
		allocatedBuffer mBuffer;
		mapFlags mMapFlags;

		size_t mLoadedBytes;
		size_t mByteSize;
		bool mNeedDescriptorUpdate;
	};
}
