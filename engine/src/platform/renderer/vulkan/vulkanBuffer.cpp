#include <pch.h>

#include "vulkanBuffer.h"
#include "helper.h"

namespace engine
{
	void vulkanBuffer::init(VkDevice device, VmaAllocator allocator)
	{
		mDevice = device;
		mAllocator = allocator;
	}

	error vulkanBuffer::build(immediateSubmit is, const void* data, size_t sizeInBytes, size_t validBytes)
	{
		if (mBuffer.buffer != VK_NULL_HANDLE)
			return error{ "buffer already created" };

		mLoadedBytes = validBytes;
		mByteSize = sizeInBytes;

		auto createBufRes = createBuffer(mAllocator, mDevice, sizeInBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		if (!createBufRes)
			return createBufRes.err();

		mBuffer = createBufRes.value();

		if (data && mLoadedBytes != 0)
		{
			auto stagingBuffer = createBuffer(mAllocator, mDevice, mLoadedBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_ONLY, true);
			if (!stagingBuffer)
				return stagingBuffer.err();

			std::memcpy(stagingBuffer.value().info.pMappedData, data, mLoadedBytes);

			auto err = is.submit(
				[&](VkCommandBuffer cmd)
				{
					VkBufferCopy copy{};
					copy.dstOffset = 0;
					copy.srcOffset = 0;
					copy.size = mLoadedBytes;

					vkCmdCopyBuffer(cmd, stagingBuffer.value().buffer, mBuffer.buffer, 1, &copy);
				}
			);
			if (err)
				return err;

			destroyBuffer(mAllocator, stagingBuffer.value());
		}

		return {};
	}

	error vulkanBuffer::updateBuffer(immediateSubmit is, const void* data, size_t sizeInBytes, size_t offset)
	{
		if (sizeInBytes + mLoadedBytes >= mByteSize)
			return error{ "buffer overflow" };

		auto stagingBuffer = createBuffer(mAllocator, mDevice, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_ONLY, true);
		if (!stagingBuffer)
			return stagingBuffer.err();

		std::memcpy(stagingBuffer.value().info.pMappedData, data, sizeInBytes);

		auto err = is.submit(
			[&](VkCommandBuffer cmd)
			{
				VkBufferCopy copy{};
				copy.dstOffset = offset;
				copy.srcOffset = 0;
				copy.size = sizeInBytes;

				vkCmdCopyBuffer(cmd, stagingBuffer.value().buffer, mBuffer.buffer, 1, &copy);
			}
		);
		if (err)
			return err;

		destroyBuffer(mAllocator, stagingBuffer.value());

		mLoadedBytes += sizeInBytes;

		return {};
	}

	void vulkanBuffer::destroy()
	{
		destroyBuffer(mAllocator, mBuffer);

		mBuffer.buffer = VK_NULL_HANDLE;
		mBuffer.allocation = VK_NULL_HANDLE;
		mBuffer.info = VmaAllocationInfo{};

		mByteSize = 0;
		mLoadedBytes = 0;
	}

	allocatedBuffer vulkanBuffer::getBuffer()
	{
		return mBuffer;
	}

	withError<allocatedBuffer> vulkanBuffer::createBuffer(VmaAllocator allocator, VkDevice device, size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, bool useMemmoryMap)
	{
		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.size = allocSize;

		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = memoryUsage;

		if (useMemmoryMap)
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

	size_t vulkanBuffer::getSize() const
	{
		return mByteSize;
	}

	size_t vulkanBuffer::getLoadedBytes() const
	{
		return mLoadedBytes;
	}
}