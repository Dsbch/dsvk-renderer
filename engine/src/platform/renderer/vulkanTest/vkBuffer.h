#pragma once

#include <pch.h>

#include "vkHelper.h"
#include <vma/vk_mem_alloc.h>

#include "immediateSubmit.h"

namespace vktest
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
			mAllocator(VK_NULL_HANDLE)
		{
		}

		void init(VkDevice device, VmaAllocator allocator);
		engine::error build(immediateSubmit is, void* data, size_t sizeInBytes, size_t len);
		void destroy();

		allocatedBuffer getBuffer();

		static engine::withError<allocatedBuffer> createBuffer(VmaAllocator allocator, VkDevice device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, bool useMemmoryMap = false);
		static void destroyBuffer(VmaAllocator allocator, allocatedBuffer buf);
	private:
		VkDevice mDevice;
		VmaAllocator mAllocator;
		allocatedBuffer mBuffer;
	};
}
