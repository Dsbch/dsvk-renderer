#include <pch.h>
#include "vulkanBuffer.h"

namespace vktest
{
	engine::error vulkanBuffer::checkError()
	{
		return mErr;
	}

	void vulkanBuffer::init(VkDevice device, VmaAllocator allocator)
	{
		mDevice = device;
		mAllocator = allocator;
	}

	engine::error vulkanBuffer::build(immediateSubmit is, void* data, size_t sizeInBytes, size_t len)
	{
		auto createBufRes = createBuffer(mAllocator, mDevice, sizeInBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		if (!createBufRes)
			return createBufRes.err();

		mBuffer = createBufRes.value();

		auto stagingBuffer = createBuffer(mAllocator, mDevice, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
		if (!stagingBuffer)
			return stagingBuffer.err();

		void* mappedData = nullptr;
		vmaMapMemory(mAllocator, stagingBuffer.value().allocation, &mappedData);

		std::memcpy(mappedData, data, sizeInBytes);

		is.submit(
			[&](VkCommandBuffer cmd)
			{
				VkBufferCopy copy {};
				copy.dstOffset = 0;
				copy.srcOffset = 0;
				copy.size = sizeInBytes;

				vkCmdCopyBuffer(cmd, stagingBuffer.value().buffer, mBuffer.buffer, 1, &copy);
			}
		);

		vmaUnmapMemory(mAllocator, stagingBuffer.value().allocation);

		destroyBuffer(mAllocator, stagingBuffer.value());

		return {};
	}

	void vulkanBuffer::destroy()
	{
		destroyBuffer(mAllocator, mBuffer);
	}

	allocatedBuffer vulkanBuffer::getBuffer()
	{
		return mBuffer;
	}

	engine::withError<allocatedBuffer> vulkanBuffer::createBuffer(VmaAllocator allocator, VkDevice device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
	{
		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.size = allocSize;

		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = memoryUsage;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		allocatedBuffer newBuffer;

		auto result = vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info);
		if (result != VK_SUCCESS)
			return { vkResultToStr(result) };

		VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newBuffer.buffer };
		newBuffer.bufferAddress = vkGetBufferDeviceAddress(device, &deviceAdressInfo);

		return newBuffer;
	}

	void vulkanBuffer::destroyBuffer(VmaAllocator allocator, allocatedBuffer buf)
	{
		vmaDestroyBuffer(allocator, buf.buffer, buf.allocation);
	}
}