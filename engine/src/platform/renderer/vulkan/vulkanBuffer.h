#pragma once

#include <pch.h>

#include <vk_mem_alloc.h>

#include "VulkanImmediateSubmit.h"

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

		void init(VkDevice device, VmaAllocator allocator);
		engine::error build(immediateSubmit is, const void* data, size_t sizeInBytes, size_t validBytes);
		engine::error updateBuffer(immediateSubmit is, const void* data, size_t sizeInBytes, size_t offset);
		void destroy();

		allocatedBuffer getBuffer();

		void markBytesAsDead(size_t size);
		size_t getSize() const;
		size_t getLoadedBytes() const;

		static engine::withError<allocatedBuffer> createBuffer(VmaAllocator allocator, VkDevice device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, bool useMemmoryMap = false);
		static void destroyBuffer(VmaAllocator allocator, allocatedBuffer buf);
	private:
		VkDevice mDevice;
		VmaAllocator mAllocator;
		allocatedBuffer mBuffer;

		size_t mLoadedBytes;
		size_t mByteSize;
	};
}
