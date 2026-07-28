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

	struct graphicsPipeline
	{
	public:
		enum class pipelineType
		{
			opaque,
			accumilation,
			composite,
		};

		void init(VkDevice device, pipelineType type);
		error build(
			std::shared_ptr<const shader> pixelShader,
			std::shared_ptr<const shader> meshShader,
			std::shared_ptr<const shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			const std::vector<VkFormat>& colorAttachmentFormats,
			VkSampleCountFlagBits sampleCount
		);
		error buildLinePipeline(
			std::shared_ptr<const shader> pixelShader,
			std::shared_ptr<const shader> vertexShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			const std::vector<VkFormat>& colorAttachmentFormats,
			VkSampleCountFlagBits sampleCount
		);
		void destroy();
		std::pair<VkPipeline, VkPipelineLayout> getPipeline() const;
	private:
		error create(VkPushConstantRange* pushConstant, const std::vector<VkDescriptorSetLayout>& descriptorSets, bool meshShaderPipeline = false);

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

		VkDevice mDevice;
		pipelineType mType;
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
		error build(VkPushConstantRange* pushConstant, const std::vector<VkDescriptorSetLayout>& descriptorSets);
		void destroy();

		void setShader(VkShaderModule computeShader);

		std::pair<VkPipeline, VkPipelineLayout> getPipeline();
	private:
		VkDevice mDevice;
		VkPipeline mPipeline;
		VkPipelineLayout mPipelineLayout;
		VkPipelineShaderStageCreateInfo mComputeShaderStage;
	};
}