#pragma once

#include <pch.h>
#include "VkBootstrap.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

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

		std::pair<VkPipeline, VkPipelineLayout> getPipeline();

		void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
		void setShaders(VkShaderModule taskShader, VkShaderModule meshShader, VkShaderModule fragmentShader);
		void setInputTopology(VkPrimitiveTopology topology);
		void setPolygonMode(VkPolygonMode mode);
		void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
		void setMultisamplingNone();
		void disableBlending();
		void enableBlendingAdditive();
		void enableBlendingAlphablend();

		void setColorAttachmentFormat(VkFormat format);
		void setDepthFormat(VkFormat format);
		void disableDepthtest();
		void enableDepthtest(bool depthWriteEnable, VkCompareOp op);
	private:
		VkDevice mDevice;
		VkPipeline mPipeline;
		VkPipelineLayout mPipelineLayout;

		std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
		VkPipelineInputAssemblyStateCreateInfo mInputAssembly;
		VkPipelineRasterizationStateCreateInfo mRasterizer;
		VkPipelineColorBlendAttachmentState mColorBlendAttachment;
		VkPipelineMultisampleStateCreateInfo mMultisampling;
		VkPipelineDepthStencilStateCreateInfo mDepthStencil;
		VkPipelineRenderingCreateInfo mRenderInfo;
		VkFormat mColorAttachmentformat;
	};

	struct computePipeline
	{
	public:
		computePipeline();

		void init(VkDevice device);
		engine::error build(VkPushConstantRange* pushConstant, const std::vector<VkDescriptorSetLayout>& descriptorSets);
		void destroy();
		engine::error checkError();

		void setShader(VkShaderModule computeShader);

		std::pair<VkPipeline, VkPipelineLayout> getPipeline();
	private:
		engine::error mErr;

		VkDevice mDevice;
		VkPipeline mPipeline;
		VkPipelineLayout mPipelineLayout;
		VkPipelineShaderStageCreateInfo mComputeShaderStage;
	};
}