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

		void init(std::shared_ptr<context> ctx, VkDevice device, VmaAllocator allocator, submit& is, uint32_t framesInFlight);
		error build(submit& is);
		void destroy();

		struct meshes
		{
			UID meshID;
			bufferHandle meshHandle;
			bufferHandle meshletHandle;
			const dataWithLodLevels<meshletData>& meshlets;
		};

		struct addInstanceParams
		{
			UID instanceID;
			bufferHandle perInstanceHandle;
			std::vector<meshes>& meshesData;
			bool isBlendGeometry;
			uint32_t frameIndex;
		};

		struct removeInstanceParams
		{
			UID instanceID;
			std::vector<meshes>& meshesData;
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

		bool meshIsUsed(UID id, uint32_t frameIndex) const;
		uint32_t getMaxJobCount(uint32_t frameIndex) const;
		bool instanceExists(UID id, uint32_t frameIndex) const;
		std::vector<VkDescriptorBufferInfo> getBufferInfo();
		bool needDescriptorUpdate() const;
		void setUpdated();
		vulkanBuffer getCmdBuffer(uint32_t frameIndex) const;
		uint32_t getCommandBufferLoadedSize(uint32_t frameIndex) const;
	private:
		std::shared_ptr<context> mCtx;

		uint32_t mFramesInFlight;

		bool mNeedDescriptorUpdate;

		uint32_t mCmdBufferSize;
		vulkanBuffer::mapFlags mBufferMapFlags;
		std::vector<vulkanBuffer> mCmdBuffer;
		
		std::vector<std::set<UID>> mEntitiesToDelete;
		std::vector<std::map<UID, std::vector<meshletShaderCMD>>> mEntitiesToAdd;
		std::vector<std::map<UID, std::pair<size_t, size_t>>> mUploadedEntities;
		std::vector<std::map<UID, uint32_t>> mMeshCount;
		std::vector<std::map<UID, uint32_t>> mJobCount;
		std::vector<VkDescriptorBufferInfo> mCmdBufferInfo;
	};
}