#pragma once

#include <pch.h>

#include <vma/vk_mem_alloc.h>

#include "registry.h"
#include "submit.h"
#include "platform/renderer/vertex.h"

namespace engine
{
	// Command buffer holds two buffers under the hood.
	// First is actual command buffer.
	// Second is visability buffer.
	struct commandBuffer
	{
	public:
		commandBuffer() = default;

		commandBuffer(const commandBuffer&) = delete;

		void init(VkDevice device, VmaAllocator allocator, submit& is, uint32_t framesInFlight);
		error build(submit& is);
		void destroy();

		struct meshes
		{
			uint32_t meshID;
			bufferHandle meshHandle;
			bufferHandle meshletHandle;
			const dataWithLodLevels<meshletData>& meshlets;
		};

		struct addInstanceParams
		{
			uint32_t instanceID;
			bufferHandle perInstanceHandle;
			std::vector<meshes> meshesData;
			bool isBlendGeometry;
			uint32_t frameIndex;
		};

		struct removeInstanceParams
		{
			uint32_t instanceID;
			uint32_t meshID;
			uint32_t frameIndex;
		};

		struct updateCommandBufferParams
		{
			VkDevice device;
			VmaAllocator allocator;
			submit& is;
			uint32_t frameIndex;
		};

		error addInstance(const addInstanceParams& params);
		void removeInstance(const removeInstanceParams& params);
		error updateCommandBuffer(const updateCommandBufferParams& params);

		bool meshIsUsed(uint32_t id, uint32_t frameIndex) const;
		bool instanceExists(uint32_t id, uint32_t frameIndex) const;
		std::pair<std::vector<VkDescriptorBufferInfo>, std::vector<VkDescriptorBufferInfo>> getBufferInfo();
		bool needDescriptorUpdate() const;
		void setUpdated();
		vulkanBuffer getCmdBuffer(uint32_t frameIndex) const;
		vulkanBuffer getVisabilityBuffer(uint32_t frameIndex) const;
		uint32_t getCommandBufferLoadedSize(uint32_t frameIndex) const;
	private:
		uint32_t mFramesInFlight;

		bool mNeedDescriptorUpdate;

		uint32_t mCmdBufferSize;
		vulkanBuffer::mapFlags mBufferMapFlags;
		std::vector<vulkanBuffer> mCmdBuffer;
		std::vector<vulkanBuffer> mVisabilityBuffer;
		
		std::vector<std::set<uint32_t>> mEntitiesToDelete;
		std::vector<std::map<uint32_t, std::vector<meshletShaderCMD>>> mEntitiesToAdd;
		std::vector<std::map<uint32_t, std::pair<size_t, size_t>>> mUploadedEntities;
		std::vector<std::map<uint32_t, uint32_t>> mMeshCount;
		std::vector<VkDescriptorBufferInfo> mCmdBufferInfo;
		std::vector<VkDescriptorBufferInfo> mVisabilityBufferInfo;
	};
}