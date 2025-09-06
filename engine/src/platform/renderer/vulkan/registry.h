#include <pch.h>

#include "platform/renderer/vertex.h"

#include "vulkanBuffer.h"
#include "vulkanImmediateSubmit.h"
#include "vulkanPipeline.h"
#include "vulkanImage.h"
#include "vulkanShader.h"

namespace engine
{
	static const uint32_t newBufferSize = 2 << 28;

	struct bufferHandle
	{
		uint32_t id;
		uint32_t offset;
		uint32_t bufferIndex;

		bool operator<(const bufferHandle& other) const {
			return other.id < id;
		}
	};

	struct bufferWithHandles
	{
		vulkanBuffer buffer;
		std::set<bufferHandle> bufferHandles;
	};

	struct bufferRegistry
	{
	public:
		void init(VkDevice device, VmaAllocator allocator, immediateSubmit immSubmit);
		withError<bufferHandle> addBlock(uint32_t id, void* data, size_t sizeInBytes);
		void deleteBlock(const bufferHandle& handle);
		void destroy();
		bool needDecriptorUpdate() const;

		VkDescriptorSetLayoutBinding getLayoutBinding(uint32_t binding, uint32_t maxDescriptorCount) const;
		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);
	private:
		std::vector<bufferWithHandles> mBuffers;

		VkDevice mDevice;
		VmaAllocator mAllocator;
		immediateSubmit mImmSubmit;

		bool mNeedUpdate;
	};

	struct textureRegistry
	{
	public:
		void init(VkSampler sampler);
		uint32_t addTexture(uint32_t id, const vulkanImage& texture);
		void deleteTexture(uint32_t offset);
		void destroy();
		bool needDecriptorUpdate() const;

		VkDescriptorSetLayoutBinding getLayoutBinding(uint32_t binding, uint32_t maxDescriptorCount) const;
		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);
	private:
		std::vector<vulkanImage> mTextures;
		std::map<uint32_t, uint32_t> mUploadedTextures;
		VkSampler mSampler;
		bool mNeedUpdate;
	};
}
