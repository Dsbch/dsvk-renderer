#pragma once

#include <pch.h>
#include "VkBootstrap.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "vkHelper.h"

namespace vktest
{
	class classicGraphicPipeline
	{
	public:
		classicGraphicPipeline();

		void destroy();

		engine::error checkError();

		void setDevice(VkDevice device);
		engine::error buildPipeline(VkPushConstantRange* pushConstant, const std::vector<VkDescriptorSetLayout>& descriptorSets);
		std::pair<VkPipeline, VkPipelineLayout> getPipeline();

		void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
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
		engine::error mErr;

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

		void clear();
	};

	class computePipeline
	{
	public:
		computePipeline();

		void destroy();
		engine::error checkError();

		void setDevice(VkDevice device);
		void setShader(VkShaderModule computeShader);
		engine::error buildPipeline(VkPushConstantRange* pushConstant, const std::vector<VkDescriptorSetLayout>& descriptorSets);
		
		std::pair<VkPipeline, VkPipelineLayout> getPipeline();
	private:
		engine::error mErr;

		VkDevice mDevice;
		VkPipeline mPipeline;
		VkPipelineLayout mPipelineLayout;
		VkPipelineShaderStageCreateInfo mComputeShaderStage;
	};
}