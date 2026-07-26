#pragma once

#include <pch.h>
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "registry.h"
#include "descriptorSet.h"

namespace engine
{
	inline VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
		VkShaderModule shaderModule,
		const char* entry)
	{
		VkPipelineShaderStageCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		info.pNext = nullptr;

		// shader stage
		info.stage = stage;
		// module containing the code for this shader stage
		info.module = shaderModule;
		// the entry point of the shader
		info.pName = entry;
		return info;
	}

	inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo()
	{
		VkPipelineLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.pNext = nullptr;

		// empty defaults
		info.flags = 0;
		info.setLayoutCount = 0;
		info.pSetLayouts = nullptr;
		info.pushConstantRangeCount = 0;
		info.pPushConstantRanges = nullptr;
		return info;
	}

	struct classicGraphicPipeline
	{
	public:
		classicGraphicPipeline();

		void init(VkDevice device);
		engine::error build(VkPushConstantRange* pushConstant, const std::vector<VkDescriptorSetLayout>& descriptorSets, bool meshShaderPipeline = false);
		void destroy();

		std::pair<VkPipeline, VkPipelineLayout> getPipeline() const;

		void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
		void setShaders(VkShaderModule taskShader, VkShaderModule meshShader, VkShaderModule fragmentShader);
		void setInputTopology(VkPrimitiveTopology topology);
		void setPolygonMode(VkPolygonMode mode);
		void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
		void setMultisampling(VkSampleCountFlagBits sampleCount);
		void disableBlending();

		void setColorAttachmentFormats(const std::vector<VkFormat>& formats);
		void setDepthFormat(VkFormat format);
		void disableDepthtest();
		void enableDepthtest(bool depthWriteEnable, VkCompareOp op);
		void enableBlendingOITAccumulation();
		void enableBlendingOITComposite();
	private:
		VkDevice mDevice;
		VkPipeline mPipeline;
		VkPipelineLayout mPipelineLayout;

		std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
		VkPipelineInputAssemblyStateCreateInfo mInputAssembly;
		VkPipelineRasterizationStateCreateInfo mRasterizer;
		std::vector<VkPipelineColorBlendAttachmentState> mColorBlendAttachments;
		VkPipelineMultisampleStateCreateInfo mMultisampling;
		VkPipelineDepthStencilStateCreateInfo mDepthStencil;
		VkPipelineRenderingCreateInfo mRenderInfo;
		std::vector<VkFormat> mColorAttachmentformats;

		uint32_t mID;
	};

	struct computePipeline
	{
	public:
		computePipeline();

		void init(VkDevice device);
		engine::error build(VkPushConstantRange* pushConstant, const std::vector<VkDescriptorSetLayout>& descriptorSets);
		void destroy();

		void setShader(VkShaderModule computeShader);

		std::pair<VkPipeline, VkPipelineLayout> getPipeline();
	private:
		VkDevice mDevice;
		VkPipeline mPipeline;
		VkPipelineLayout mPipelineLayout;
		VkPipelineShaderStageCreateInfo mComputeShaderStage;
	};

	struct pipelineData
	{
	public:
		enum class pipelineType
		{
			opaque,
			accumilation,
			composite,
		};

		error init(
			VkDevice device,
			VmaAllocator allocator,
			submit& is,
			std::shared_ptr<const shader> pixelShader,
			std::shared_ptr<const shader> meshShader,
			std::shared_ptr<const shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			const std::vector<VkFormat>& colorAttachmentFormats,
			VkSampleCountFlagBits sampleCount,
			uint32_t framesInFlight,
			pipelineType type = pipelineType::opaque
		);
		void destroy();

		struct meshes
		{
			uint32_t meshID;
			bufferHandle meshHandle;
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
			uint32_t frameIndex;
		};

		struct removeInstanceParams
		{
			uint32_t pixelShaderID;
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

		struct pipelineRenderData
		{
			VkPipeline pipeline;
			VkPipelineLayout pipelineLayout;
			uint32_t cmdBufferCount;
		};

		pipelineRenderData getPipelineRenderData(uint32_t frameIndex) const;
		bool meshIsUsed(uint32_t id, uint32_t frameIndex) const;
		bool instanceExists(uint32_t id, uint32_t frameIndex) const;
		std::vector<VkDescriptorBufferInfo> getBufferInfo();
		bool needDescriptorUpdate() const;
		void setUpdated();
		vulkanBuffer getBuffer(uint32_t frameIndex) const;
		uint32_t getCommandBufferLoadedSize(uint32_t frameIndex) const;
	private:
		classicGraphicPipeline mPipeline;

		bool mNeedDescriptorUpdate;

		std::vector<std::set<entityHash>> mEntitiesToDelete;
		std::vector<std::map<entityHash, std::vector<meshletShaderCMD>>> mEntitiesToAdd;
		std::vector<std::map<entityHash, std::pair<size_t, size_t>>> mUploadedEntities;
		std::vector<std::map<meshHash, uint32_t>> mMeshCount;

		vulkanBuffer::mapFlags mBufferMapFlags;
		uint32_t mCmdBufferSize;
		std::vector<vulkanBuffer> mCmdBuffer;
		std::vector<VkDescriptorBufferInfo> mBufferInfo;
	};
}