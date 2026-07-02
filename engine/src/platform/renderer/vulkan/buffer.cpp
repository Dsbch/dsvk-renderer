#include <pch.h>

#include "buffer.h"
#include "helper.h"

namespace engine
{
	void vulkanBuffer::init(VkDevice device, VmaAllocator allocator, bool mapped)
	{
		mMapped = mapped;
		mDevice = device;
		mAllocator = allocator;
	}

	error vulkanBuffer::build(submit& is, const void* data, size_t sizeInBytes, size_t validBytes)
	{
		if (mBuffer.buffer != VK_NULL_HANDLE)
			return error{ "buffer already created" };

		mLoadedBytes = validBytes;
		mByteSize = sizeInBytes;

		auto createBufRes = createBuffer(
			mAllocator,
			mDevice,
			sizeInBytes,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			mMapped
		);
		if (!createBufRes)
			return createBufRes.err();

		mBuffer = createBufRes.value();

		if (data && mLoadedBytes != 0)
		{
			if (mMapped)
			{
				VkResult res = vmaCopyMemoryToAllocation(mAllocator, data, mBuffer.allocation, 0, mLoadedBytes);
				if (res != VK_SUCCESS)
					return { vkResultToStr(res) };
			}
			else
			{
				auto stagingBuffer = createBuffer(mAllocator, mDevice, mLoadedBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, true);
				if (!stagingBuffer)
					return stagingBuffer.err();

				VkResult res = vmaCopyMemoryToAllocation(mAllocator, data, stagingBuffer.value().allocation, 0, mLoadedBytes);
				if (res != VK_SUCCESS)
					return { vkResultToStr(res) };

				error err = is.queue(
					[&](VkCommandBuffer cmd)
					{
						VkBufferCopy copy{};
						copy.dstOffset = 0;
						copy.srcOffset = 0;
						copy.size = mLoadedBytes;

						vkCmdCopyBuffer(cmd, stagingBuffer.value().buffer, mBuffer.buffer, 1, &copy);
					},
					[allocator = mAllocator, buffer = stagingBuffer.value()]()
					{
						destroyBuffer(allocator, buffer);
					}
				);
				if (err)
					return err;
			}
		}

		return {};
	}

	error vulkanBuffer::build(submit& is, vulkanBuffer& buf, size_t sizeInBytes)
	{
		if (mBuffer.buffer != VK_NULL_HANDLE)
			return error{ "buffer already created" };

		mLoadedBytes = buf.getLoadedBytes();
		mByteSize = sizeInBytes;

		auto createBufRes = createBuffer(
			mAllocator,
			mDevice,
			sizeInBytes,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			mMapped
		);
		if (!createBufRes)
			return createBufRes.err();

		mBuffer = createBufRes.value();

		if (buf.getBuffer().buffer != VK_NULL_HANDLE && mLoadedBytes != 0)
		{
			if (mMapped && buf.mMapped)
			{
				VkResult res = vmaCopyMemoryToAllocation(mAllocator, buf.mBuffer.info.pMappedData, mBuffer.allocation, 0, mLoadedBytes);
				if (res != VK_SUCCESS)
					return { vkResultToStr(res) };
			}
			else
			{
				error err = is.queue(
					[&](VkCommandBuffer cmd)
					{
						VkBufferCopy copy{};
						copy.dstOffset = 0;
						copy.srcOffset = 0;
						copy.size = mLoadedBytes;

						vkCmdCopyBuffer(cmd, buf.getBuffer().buffer, mBuffer.buffer, 1, &copy);
					}
				);
				if (err)
					return err;
			}
		}

		return {};
	}


	error vulkanBuffer::buildAsUBO(submit& is, const void* data, size_t sizeInBytes, size_t validBytes)
	{
		if (mBuffer.buffer != VK_NULL_HANDLE)
			return error{ "buffer already created" };

		mLoadedBytes = validBytes;
		mByteSize = sizeInBytes;

		auto createBufRes = createBuffer(
			mAllocator,
			mDevice,
			sizeInBytes,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
			mMapped
		);
		if (!createBufRes)
			return createBufRes.err();

		mBuffer = createBufRes.value();

		if (data && mLoadedBytes != 0)
		{
			if (mMapped)
			{
				VkResult res = vmaCopyMemoryToAllocation(mAllocator, data, mBuffer.allocation, 0, mLoadedBytes);
				if (res != VK_SUCCESS)
					return { vkResultToStr(res) };
			}
			else
			{
				auto stagingBuffer = createBuffer(mAllocator, mDevice, mLoadedBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, true);
				if (!stagingBuffer)
					return stagingBuffer.err();

				VkResult res = vmaCopyMemoryToAllocation(mAllocator, data, stagingBuffer.value().allocation, 0, mLoadedBytes);
				if (res != VK_SUCCESS)
					return { vkResultToStr(res) };

				error err = is.queue(
					[&](VkCommandBuffer cmd)
					{
						VkBufferCopy copy{};
						copy.dstOffset = 0;
						copy.srcOffset = 0;
						copy.size = mLoadedBytes;

						vkCmdCopyBuffer(cmd, stagingBuffer.value().buffer, mBuffer.buffer, 1, &copy);
					},
					[allocator = mAllocator, buffer = stagingBuffer.value()]()
					{
						destroyBuffer(allocator, buffer);
					}
				);
				if (err)
					return err;
			}
		}

		return {};
	}

	error vulkanBuffer::updateBuffer(submit& is, const void* data, size_t sizeInBytes, size_t offset)
	{
		if (sizeInBytes == 0)
			return {};

		if (sizeInBytes + mLoadedBytes > mByteSize)
			return error{ errCodeBufferOverFlow, "buffer overflow" };

		if (mMapped)
		{
			VkResult res = vmaCopyMemoryToAllocation(mAllocator, data, mBuffer.allocation, offset, sizeInBytes);
			if (res != VK_SUCCESS)
				return { vkResultToStr(res) };
		}
		else
		{
			auto stagingBuffer = createBuffer(mAllocator, mDevice, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, true);
			if (!stagingBuffer)
				return stagingBuffer.err();

			VkResult res = vmaCopyMemoryToAllocation(mAllocator, data, stagingBuffer.value().allocation, 0, sizeInBytes);
			if (res != VK_SUCCESS)
				return { vkResultToStr(res) };

			error err = is.queue(
				[&](VkCommandBuffer cmd)
				{
					VkBufferCopy copy{};
					copy.dstOffset = offset;
					copy.srcOffset = 0;
					copy.size = sizeInBytes;

					vkCmdCopyBuffer(cmd, stagingBuffer.value().buffer, mBuffer.buffer, 1, &copy);
				},
				[=]()
				{
					destroyBuffer(mAllocator, stagingBuffer.value());
				}
			);
			if (err)
				return err;
		}

		mLoadedBytes += sizeInBytes;

		return {};
	}

	error vulkanBuffer::shiftData(submit& is, size_t dstOffset, size_t srcOffset)
	{
		if (mMapped)
		{
			VkResult res = vmaCopyMemoryToAllocation(mAllocator, (uint8_t*)mBuffer.info.pMappedData + srcOffset, mBuffer.allocation, dstOffset, mLoadedBytes - srcOffset);
			if (res != VK_SUCCESS)
				return { vkResultToStr(res) };
		}
		else
		{
			auto stagingBuffer = createBuffer(mAllocator, mDevice, mLoadedBytes - srcOffset, VK_BUFFER_USAGE_TRANSFER_DST_BIT| VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, false);
			if (!stagingBuffer)
				return stagingBuffer.err();

			error err = is.queue(
				[&](VkCommandBuffer cmd)
				{
					VkBufferCopy copy{};
					copy.dstOffset = 0;
					copy.srcOffset = srcOffset;
					copy.size = mLoadedBytes - srcOffset;

					vkCmdCopyBuffer(cmd, mBuffer.buffer, stagingBuffer.value().buffer, 1, &copy);

					pipelineBufferBarier(cmd, stagingBuffer.value().buffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, uint32_t(copy.size), 0);

					copy.dstOffset = dstOffset;
					copy.srcOffset = 0;
				
					vkCmdCopyBuffer(cmd, stagingBuffer.value().buffer, mBuffer.buffer, 1, &copy);
				},
				[=]()
				{
					destroyBuffer(mAllocator, stagingBuffer.value());
				}
			);
			if (err)
				return err;
		}

		mLoadedBytes -= (srcOffset - dstOffset);

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
			vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

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
		if (buf.buffer == VK_NULL_HANDLE)
			return;

		vmaDestroyBuffer(allocator, buf.buffer, buf.allocation);
	}

	void vulkanBuffer::markBytesAsDead(size_t size)
	{
		if (mLoadedBytes < size)
			mLoadedBytes = 0;
		else
			mLoadedBytes -= size;
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