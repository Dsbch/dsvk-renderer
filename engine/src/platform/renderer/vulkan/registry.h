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
		void init(VkDevice device, VmaAllocator allocator, bool mapped = false);
		withError<bufferHandle> addBlock(uint32_t id, const void* data, size_t sizeInBytes, submit& is, size_t newSize = newBufferSize);
		withError<bufferHandle> findBlock(uint32_t id);
		error updateBlock(uint32_t id, const void* data, size_t sizeInBytes, submit& is);
		bool deleteBlock(uint32_t id);
		void destroy();
		void setUpdated();
		bool needDescriptorUpdate() const;

		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);
	private:
		bool mUseMappedBuffers;
		std::vector<bufferWithHandles> mBuffers;
		std::vector<VkDescriptorBufferInfo> mBuffersInfo;

		VkDevice mDevice;
		VmaAllocator mAllocator;

		bool mNeedUpdate;
	};

	// PipelineData and pipeLineRegistry structs manage pipeline creation and constructing of command buffer for task shader.
	struct pipelineData
	{
		enum class pipelineType
		{
			opaque,
			accumilation,
			composite,
		};

		struct taskShaderRender
		{
			VkPipeline pipeline;
			VkPipelineLayout layout;
			uint32_t cmdPipelineStartOffset;
			uint32_t cmdPipelineEndOffset;
		};

		error init(
			VkDevice device,
			std::shared_ptr<const shader> pixelShader,
			std::shared_ptr<const shader> meshShader,
			std::shared_ptr<const shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			const std::vector<VkFormat>& colorAttachmentFormats,
			VkSampleCountFlagBits sampleCount,
			pipelineType type = pipelineType::opaque
		);
		void destroy();

		classicGraphicPipeline pipeline;
		taskShaderRender mCmdMapping;
		std::map<entityHash, std::vector<meshletShaderCMD>> entityCmd;
		std::map<meshHash, uint32_t> instanceMeshCount;
		bool needUpdate;
	};

	struct pipelineRegistry
	{
	public:
		error init(VkDevice device, VmaAllocator allocator, submit& is);
		error initAccumilatePipeline(
			VkDevice device,
			std::shared_ptr<const shader> pixelShader,
			std::shared_ptr<const shader> meshShader,
			std::shared_ptr<const shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			const std::initializer_list<VkFormat>& colorAttachmentFormats,
			graphicsPreset preset
		);
		error initCompositePipeline(
			VkDevice device,
			std::shared_ptr<const shader> pixelShader,
			std::shared_ptr<const shader> meshShader,
			std::shared_ptr<const shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			const std::initializer_list<VkFormat>& colorAttachmentFormats,
			graphicsPreset preset
		);

		void destroy();

		error createPipeline(
			VkDevice device,
			std::shared_ptr<const shader> pixelShader,
			std::shared_ptr<const shader> meshShader,
			std::shared_ptr<const shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			const std::initializer_list<VkFormat>& colorAttachmentFormats,
			graphicsPreset preset
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
			bool isBlendGeometry;
		};

		error addInstance(const addInstanceParams& params);
		void removeInstance(uint32_t pixelShaderID, uint32_t instanceID, uint32_t meshID);

		std::vector<VkWriteDescriptorSet> getOpaqueCmdBufferWriteInfo(uint32_t binding);
		std::vector<VkWriteDescriptorSet> getAccumilationCmdBufferWriteInfo(uint32_t binding);

		bool instanceExists(uint32_t id) const;
		bool meshIsUsed(uint32_t id) const;
		const std::vector<pipelineData::taskShaderRender> getOpaquePipelines() const;
		const pipelineData::taskShaderRender getAccumilationPipeline() const;
		const classicGraphicPipeline getCompositePipeline() const;

		error updateOpaqueCmdBuffer(submit& is);
		error updateAccumilationCmdBuffer(submit& is);

		bool needOpaqueDescriptorUpdate() const;
		bool needAccumilationDescriptorUpdate() const;
		void setOpaqueUpdated();
		void setAccumilationUpdated();
	private:
		// Pipeline state.
		pipelineData mAccumilatePipeline;
		pipelineData mCompositePipeline;
		std::map<pixelShaderHash, pipelineData> mPipelines;

		error addOpaqueInstance(const addInstanceParams& params);
		error addBlendInstance(const addInstanceParams& params);
		void removeOpaqueInstance(uint32_t pixelShaderID, uint32_t instanceID, uint32_t meshID);
		void removeBlendInstance(uint32_t instanceID, uint32_t meshID);
		
		// Command buffers state.
		bool mNeedOpaqueDescriptorUpdate;
		bool mNeedAccumilationDescriptorUpdate;
		std::vector<VkDescriptorBufferInfo> mOpaqueBufferInfo;
		std::vector<VkDescriptorBufferInfo> mAccumilationsBufferInfo;

		uint32_t mCmdOpaqueBufferNewSize;
		uint32_t mCmdAccumilationBufferNewSize;
		vulkanBuffer mCmdOpaqueBuffer;
		vulkanBuffer mCmdAccumilationBuffer;

	};

	struct materialRegistry
	{
	public:
		error init(VkSampler sampler, materialTextures defaultMat);
		void destroy();
		void setUpdated();
		bool needDescriptorUpdate() const;

		std::vector<VkWriteDescriptorSet> getWriteInfo(uint32_t binding);

		withError<uint32_t> addMaterials(const materials& materials);
		withError<uint32_t> getMaterialsOffset(const materials& materials);
		void deleteMaterials(const materials& materials);
	private:
		materialTextures mDefaultMat;

		VmaVirtualBlock mVBlock;

		struct virtualTextureBlock
		{
			uint32_t offset;
			uint32_t size;
			VmaVirtualAllocation allocation;
		};
		std::map<textureHash, virtualTextureBlock> mUploadedMaterials;

		std::vector<VkDescriptorImageInfo> mImagesInfo;
		VkSampler mSampler;
		bool mNeedUpdate;
	};
}
