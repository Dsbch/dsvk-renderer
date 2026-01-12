#pragma once
#include <pch.h>

#include "platform/renderer/vertex.h"

#include "buffer.h"
#include "submit.h"
#include "pipeline.h"
#include "image.h"
#include "shader.h"

namespace engine
{
	static const uint32_t newBufferSize = 2 << 27;

	struct bufferHandle
	{
		uint32_t id;
		uint32_t offset;
		uint32_t bufferIndex;
		size_t size;
		VmaVirtualAllocation vAllocation;

		bool operator<(const bufferHandle& other) const 
		{
			return id < other.id;
		}
	};

	struct bufferWithHandles
	{
		vulkanBuffer buffer;
		VmaVirtualBlock vBlock;
		std::set<bufferHandle> bufferHandles;
	};

	struct bufferRegistry
	{
	public:
		void init(VkDevice device, VmaAllocator allocator, submit is);
		withError<bufferHandle> addBlock(uint32_t id, const void* data, size_t sizeInBytes, size_t newSize = newBufferSize);
		bool deleteBlock(uint32_t id);
		void destroy();
		void setUpdated();
		bool needDescriptorUpdate() const;

		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);
	private:
		std::vector<bufferWithHandles> mBuffers;
		std::vector<VkDescriptorBufferInfo> mBuffersInfo;

		VkDevice mDevice;
		VmaAllocator mAllocator;
		submit mSubmit;

		bool mNeedUpdate;
	};

	// PipelineData and pipeLineRegistry structs manage pipeline creation and constrcting command buffer for task shader.
	struct pipelineData
	{
	public:
		error init(
			VkDevice device,
			std::shared_ptr<shader> pixelShader,
			std::shared_ptr<shader> meshShader,
			std::shared_ptr<shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			VkFormat colorAttachmentFormat
		);
		void destroy();
		
		classicGraphicPipeline pipeline;
		std::map<uint32_t, std::vector<meshletShaderCMD>> meshletShaderCMD;
		std::map<uint32_t, uint32_t> instanceMeshCount;
		bool needUpdate;
	};

	struct pipelineRegistry
	{
	public:
		error init(VkDevice device, VmaAllocator allocator, submit is);
		void destroy();

		error createPipeline(
			VkDevice device,
			std::shared_ptr<shader> pixelShader,
			std::shared_ptr<shader> meshShader,
			std::shared_ptr<shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			VkFormat colorAttachmentFormat
		);
		error addInstance(uint32_t pixelShaderID, uint32_t instanceID, uint32_t meshID, bufferHandle meshletHandle, bufferHandle perInstanceHandle, const dataWithLodLevels<meshlet>& mesh);
		void removeInstance(uint32_t pixelShaderID, uint32_t instanceID, uint32_t meshID);
		
		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);

		bool instanceExists(uint32_t id) const;
		bool meshIsUsed(uint32_t id) const;
		struct taskShaderRender
		{
			VkPipeline pipeline;
			VkPipelineLayout layout;
			uint32_t commandBufferLength;
		};
		std::vector<taskShaderRender> getPipelines();

		error updateCommandBuffer();

		bool needDescriptorUpdate() const;
		void setUpdated();
	private:
		submit mSubmit;

		bool mNeedDescriptorUpdate;
		std::vector<VkDescriptorBufferInfo> mBufferInfo;

		uint32_t mCmdBufferNewSize;
		vulkanBuffer mCmdBuffer;
		std::map<uint32_t, pipelineData> mPipelines;
	};
	
	struct materialRegistry
	{
	public:
		struct materialOffsets
		{
			uint32_t albedo;
			uint32_t normal;
			uint32_t metalicRoughnes;
		};

		void init(VkSampler sampler);
		void setUpdated();
		bool needDescriptorUpdate() const;

		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);

		materialOffsets addMaterial(const materialTextures& textures);
		void deleteMaterial(const materialTextures& textures);
	private:
		std::map<uint32_t, uint32_t> mOccupiedIndices;
		std::list<uint32_t> mFreeIndices;
		std::map<uint32_t, uint32_t> mTextureCount;
		std::vector<VkDescriptorImageInfo> mImagesInfo;
		VkSampler mSampler;
		bool mNeedUpdate;
	};
}
