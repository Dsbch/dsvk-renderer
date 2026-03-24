#pragma once
#include <pch.h>

#include "platform/renderer/vertex.h"
#include "platform/renderer/renderer.h"

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
		void init(VkDevice device, VmaAllocator allocator);
		withError<bufferHandle> addBlock(uint32_t id, const void* data, size_t sizeInBytes, submit& is, size_t newSize = newBufferSize);
		withError<bufferHandle> findBlock(uint32_t id);
		error updateBlock(uint32_t id, const void* data, size_t sizeInBytes, submit& is);
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
			const std::vector<VkFormat>& colorAttachmentFormats,
			VkSampleCountFlagBits sampleCount,
			bool accumilatePipeline = false,
			bool compositePipeline = false
		);
		void destroy();

		classicGraphicPipeline pipeline;
		// Command buffer to render enity.
		std::map<entityHash, std::vector<meshletShaderCMD>> entityCmd;
		std::map<meshHash, uint32_t> instanceMeshCount;
		bool needUpdate;
	};

	struct pipelineRegistry
	{
	public:
		error init(VkDevice device, VmaAllocator allocator, submit& is);
		void destroy();

		error createPipeline(
			VkDevice device,
			std::shared_ptr<shader> pixelShader,
			std::shared_ptr<shader> meshShader,
			std::shared_ptr<shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			const std::initializer_list<VkFormat>& colorAttachmentFormats,
			graphicsPreset preset,
			bool accumilatePipeline = false,
			bool compositePipeline = false
		);

		struct meshes
		{
			uint32_t meshID;
			bufferHandle meshletHandle;
			const dataWithLodLevels<meshlet>& meshlets;
		};

		struct addInstanceParams
		{
			uint32_t pixelShaderID;
			uint32_t instanceID;
			bufferHandle perInstanceHandle;
			std::vector<meshes> meshesData;
		};

		error addInstance(const addInstanceParams& params);
		void removeInstance(uint32_t pixelShaderID, uint32_t instanceID, uint32_t meshesID);

		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);

		bool instanceExists(uint32_t id) const;
		bool meshIsUsed(uint32_t id) const;
		struct taskShaderRender
		{
			VkPipeline pipeline;
			VkPipelineLayout layout;
			uint32_t cmdPipelineStartOffset;
			uint32_t cmdPipelineEndOffset;
		};
		const std::map<pixelShaderHash, taskShaderRender> getOpaquePipelines() const;
		const std::pair<taskShaderRender, pipelineData> getBlendPipelines() const;

		error updateCommandBuffer(submit& is);

		bool needDescriptorUpdate() const;
		void setUpdated();
	private:
		bool mNeedDescriptorUpdate;
		std::vector<VkDescriptorBufferInfo> mBufferInfo;

		uint32_t mCmdBufferNewSize;
		vulkanBuffer mCmdBuffer;

		std::map<pixelShaderHash, taskShaderRender> mCmdMappings;
		std::map<pixelShaderHash, pipelineData> mPipelines;
		pipelineData mAccumilatePipeline;
		pipelineData mCompositePipeline;
	};

	struct materialRegistry
	{
	public:
		error init(VkSampler sampler);
		void setUpdated();
		bool needDescriptorUpdate() const;

		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);

		withError<uint32_t> addMaterials(const materials& materials);
		withError<uint32_t> getMaterialsOffset(const materials& materials);
		void deleteMaterials(const materials& materials);
	private:
		VmaVirtualBlock mVBlock;

		struct virtualTextureBlock
		{
			uint32_t offset;
			VmaVirtualAllocation allocation;
		};
		std::map<textureHash, virtualTextureBlock> mUploadedMaterials;

		std::vector<VkDescriptorImageInfo> mImagesInfo;
		VkSampler mSampler;
		bool mNeedUpdate;
	};
}
