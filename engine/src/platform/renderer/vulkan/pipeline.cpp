#include <pch.h>
#include "pipeline.h"
#include "helper.h"

namespace engine
{
	classicGraphicPipeline::classicGraphicPipeline()
		:
		mDevice(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineLayout(VK_NULL_HANDLE),
		mInputAssembly(),
		mRasterizer(),
		mColorBlendAttachment(),
		mMultisampling(),
		mDepthStencil(),
		mRenderInfo(),
		mColorAttachmentformat(),
		mID(genUID())
	{
		mInputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

		mRasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

		mMultisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

		mPipelineLayout = {};

		mDepthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

		mRenderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

		mShaderStages.clear();
	}

	void classicGraphicPipeline::init(VkDevice device)
	{
		mDevice = device;
	}

	void classicGraphicPipeline::destroy()
	{
		vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
		vkDestroyPipeline(mDevice, mPipeline, nullptr);
	}

	std::pair<VkPipeline, VkPipelineLayout> classicGraphicPipeline::getPipeline()
	{
		return { mPipeline, mPipelineLayout };
	}

	engine::error classicGraphicPipeline::build(VkPushConstantRange* pushConstant, const std::vector<VkDescriptorSetLayout>& descriptorSets, bool meshShaderPipeline)
	{
		//build the pipeline layout that controls the inputs/outputs of the shader
		VkPipelineLayoutCreateInfo pipeline_layout_info = pipelineLayoutCreateInfo();

		if (descriptorSets.size() != 0)
		{
			pipeline_layout_info.setLayoutCount = uint32_t(descriptorSets.size());
			pipeline_layout_info.pSetLayouts = descriptorSets.data();
		}

		if (pushConstant)
		{
			pipeline_layout_info.pPushConstantRanges = pushConstant;
			pipeline_layout_info.pushConstantRangeCount = 1;
		}

		if (auto result = vkCreatePipelineLayout(mDevice, &pipeline_layout_info, nullptr, &mPipelineLayout); result != VK_SUCCESS)
			return vkResultToStr(result);

		// make viewport state from our stored viewport and scissor.
		// at the moment we wont support multiple viewports or scissors
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pNext = nullptr;

		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// setup dummy color blending. We arent using transparent objects yet
		// the blending is just "no blend", but we do write to the color attachment
		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.pNext = nullptr;

		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &mColorBlendAttachment;

		// completely clear VertexInputStateCreateInfo, as we have no need for it
		VkPipelineVertexInputStateCreateInfo _vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

		// build the actual pipeline
		// we now use all of the info structs we have been writing into into this one
		// to create the pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		// connect the renderInfo to the pNext extension mechanism
		pipelineInfo.pNext = &mRenderInfo;

		pipelineInfo.stageCount = (uint32_t)mShaderStages.size();
		pipelineInfo.pStages = mShaderStages.data();
		pipelineInfo.pVertexInputState = meshShaderPipeline ? nullptr : &_vertexInputInfo;
		pipelineInfo.pInputAssemblyState = meshShaderPipeline ? nullptr : &mInputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &mRasterizer;
		pipelineInfo.pMultisampleState = &mMultisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &mDepthStencil;
		pipelineInfo.layout = mPipelineLayout;

		// dynamic state.
		VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamicInfo.pDynamicStates = &state[0];
		dynamicInfo.dynamicStateCount = 2;

		pipelineInfo.pDynamicState = &dynamicInfo;

		// its easy to error out on create graphics pipeline, so we handle it a bit
		// better than the common VK_CHECK case
		if (auto result = vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline); result != VK_SUCCESS)
			return vkResultToStr(result);

		return {};
	}

	void classicGraphicPipeline::setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
	{
		mShaderStages.clear();

		mShaderStages.push_back(
			pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, "vsmain"));

		mShaderStages.push_back(
			pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, "psmain"));
	}

	void classicGraphicPipeline::setShaders(VkShaderModule taskShader, VkShaderModule meshShader, VkShaderModule fragmentShader)
	{
		mShaderStages.clear();

		if (taskShader != VK_NULL_HANDLE)
			mShaderStages.push_back(pipelineShaderStageCreateInfo(VK_SHADER_STAGE_TASK_BIT_EXT, taskShader, "asmain"));

		mShaderStages.push_back(pipelineShaderStageCreateInfo(VK_SHADER_STAGE_MESH_BIT_EXT, meshShader, "msmain"));

		mShaderStages.push_back(pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, "psmain"));
	}

	void classicGraphicPipeline::setInputTopology(VkPrimitiveTopology topology)
	{
		mInputAssembly.topology = topology;
		mInputAssembly.primitiveRestartEnable = VK_FALSE;
	}

	void classicGraphicPipeline::setPolygonMode(VkPolygonMode mode)
	{
		mRasterizer.polygonMode = mode;
		mRasterizer.lineWidth = 1.f;
	}

	void classicGraphicPipeline::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
	{
		mRasterizer.cullMode = cullMode;
		mRasterizer.frontFace = frontFace;
	}

	void classicGraphicPipeline::setMultisampling(VkSampleCountFlagBits sampleCount)
	{
		mMultisampling.sampleShadingEnable = VK_FALSE;
		if (sampleCount != VK_SAMPLE_COUNT_1_BIT)
			mMultisampling.sampleShadingEnable = VK_TRUE;

		// multisampling defaulted to no multisampling (1 sample per pixel)
		mMultisampling.rasterizationSamples = sampleCount;
		mMultisampling.minSampleShading = 1.0f;
		mMultisampling.pSampleMask = nullptr;
		// no alpha to coverage either
		mMultisampling.alphaToCoverageEnable = VK_FALSE;
		mMultisampling.alphaToOneEnable = VK_FALSE;
	}

	void classicGraphicPipeline::disableBlending()
	{
		// default write mask
		mColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		// no blending
		mColorBlendAttachment.blendEnable = VK_FALSE;
	}

	void classicGraphicPipeline::enableBlendingAdditive()
	{
		mColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		mColorBlendAttachment.blendEnable = VK_TRUE;
		mColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		mColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		mColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		mColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		mColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		mColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void classicGraphicPipeline::enableBlendingAlphablend()
	{
		mColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		mColorBlendAttachment.blendEnable = VK_TRUE;
		mColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		mColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		mColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		mColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		mColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		mColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	void classicGraphicPipeline::setColorAttachmentFormat(VkFormat format)
	{
		mColorAttachmentformat = format;
		// connect the format to the renderInfo  structure
		mRenderInfo.colorAttachmentCount = 1;
		mRenderInfo.pColorAttachmentFormats = &mColorAttachmentformat;
	}

	void classicGraphicPipeline::setDepthFormat(VkFormat format)
	{
		mRenderInfo.depthAttachmentFormat = format;
	}

	void classicGraphicPipeline::disableDepthtest()
	{
		mDepthStencil.depthTestEnable = VK_FALSE;
		mDepthStencil.depthWriteEnable = VK_FALSE;
		mDepthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
		mDepthStencil.depthBoundsTestEnable = VK_FALSE;
		mDepthStencil.stencilTestEnable = VK_FALSE;
		mDepthStencil.front = {};
		mDepthStencil.back = {};
		mDepthStencil.minDepthBounds = 0.f;
		mDepthStencil.maxDepthBounds = 1.f;
	}

	void classicGraphicPipeline::enableDepthtest(bool depthWriteEnable, VkCompareOp op)
	{
		mDepthStencil.depthTestEnable = VK_TRUE;
		mDepthStencil.depthWriteEnable = depthWriteEnable;
		mDepthStencil.depthCompareOp = op;
		mDepthStencil.depthBoundsTestEnable = VK_FALSE;
		mDepthStencil.stencilTestEnable = VK_FALSE;
		mDepthStencil.front = {};
		mDepthStencil.back = {};
		mDepthStencil.minDepthBounds = 0.f;
		mDepthStencil.maxDepthBounds = 1.f;
	}

	computePipeline::computePipeline()
		:
		mDevice(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineLayout(VK_NULL_HANDLE),
		mComputeShaderStage()
	{
	}

	void computePipeline::init(VkDevice device)
	{
		mDevice = device;
	}

	void computePipeline::destroy()
	{
		vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
		vkDestroyPipeline(mDevice, mPipeline, nullptr);
	}

	engine::error computePipeline::checkError()
	{
		return mErr;
	}

	void computePipeline::setShader(VkShaderModule computeShader)
	{
		mComputeShaderStage = pipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, computeShader, "main");
	}

	std::pair<VkPipeline, VkPipelineLayout> computePipeline::getPipeline()
	{
		return { mPipeline, mPipelineLayout };
	}

	engine::error computePipeline::build(VkPushConstantRange* pushConstant, const std::vector<VkDescriptorSetLayout>& descriptorSets)
	{
		VkPipelineLayoutCreateInfo pipeline_layout_info = pipelineLayoutCreateInfo();

		if (descriptorSets.size() != 0)
		{
			pipeline_layout_info.setLayoutCount = uint32_t(descriptorSets.size());
			pipeline_layout_info.pSetLayouts = descriptorSets.data();
		}

		if (pushConstant)
		{
			pipeline_layout_info.pPushConstantRanges = pushConstant;
			pipeline_layout_info.pushConstantRangeCount = 1;
		}

		if (auto result = vkCreatePipelineLayout(mDevice, &pipeline_layout_info, nullptr, &mPipelineLayout); result != VK_SUCCESS)
			return vkResultToStr(result);

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.pNext = nullptr;
		computePipelineCreateInfo.layout = mPipelineLayout;
		computePipelineCreateInfo.stage = mComputeShaderStage;

		if (auto result = vkCreateComputePipelines(mDevice, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &mPipeline); result != VK_SUCCESS)
			return vkResultToStr(result);

		return {};
	}
}
