#pragma once

#include <pch.h>
#include "VkBootstrap.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "vkHelper.h"

namespace vktest
{
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