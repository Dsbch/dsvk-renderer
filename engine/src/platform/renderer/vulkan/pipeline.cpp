#include <pch.h>
#include "pipeline.h"
#include "shader.h"
#include "submit.h"
#include "helper.h"

#include <vma/vk_mem_alloc.h>

namespace engine
{
	classicGraphicPipeline::classicGraphicPipeline()
		:
		mDevice(VK_NULL_HANDLE),
		mPipeline(VK_NULL_HANDLE),
		mPipelineLayout(VK_NULL_HANDLE),
		mInputAssembly(),
		mRasterizer(),
		mColorBlendAttachments(),
		mMultisampling(),
		mDepthStencil(),
		mRenderInfo(),
		mColorAttachmentformats(),
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

	std::pair<VkPipeline, VkPipelineLayout> classicGraphicPipeline::getPipeline() const
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
		colorBlending.attachmentCount = uint32_t(mColorBlendAttachments.size());
		colorBlending.pAttachments = mColorBlendAttachments.data();

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
		VkPipelineColorBlendAttachmentState blendingState{};

		// default write mask
		blendingState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		// no blending
		blendingState.blendEnable = VK_FALSE;

		mColorBlendAttachments = { blendingState };
	}

	void classicGraphicPipeline::enableBlendingOITAccumulation()
	{
		VkPipelineColorBlendAttachmentState accumBlend{};
		accumBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		accumBlend.blendEnable = VK_TRUE;
		accumBlend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		accumBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		accumBlend.colorBlendOp = VK_BLEND_OP_ADD;
		accumBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		accumBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		accumBlend.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendAttachmentState revealBlend{};
		revealBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
		revealBlend.blendEnable = VK_TRUE;
		revealBlend.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		revealBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		revealBlend.colorBlendOp = VK_BLEND_OP_ADD;
		revealBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		revealBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		revealBlend.alphaBlendOp = VK_BLEND_OP_ADD;

		mColorBlendAttachments = { accumBlend, revealBlend };
	}

	void classicGraphicPipeline::enableBlendingOITComposite()
	{
		VkPipelineColorBlendAttachmentState blendingState{};

		blendingState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendingState.blendEnable = VK_TRUE;
		blendingState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendingState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendingState.colorBlendOp = VK_BLEND_OP_ADD;
		blendingState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendingState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendingState.alphaBlendOp = VK_BLEND_OP_ADD;

		mColorBlendAttachments = { blendingState };
	}

	void classicGraphicPipeline::setColorAttachmentFormats(const std::vector<VkFormat>& formats)
	{
		mColorAttachmentformats = formats;
		// connect the format to the renderInfo  structure
		mRenderInfo.colorAttachmentCount = uint32_t(mColorAttachmentformats.size());
		mRenderInfo.pColorAttachmentFormats = mColorAttachmentformats.data();
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
	{}

	void computePipeline::init(VkDevice device)
	{
		mDevice = device;
	}

	void computePipeline::destroy()
	{
		vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
		vkDestroyPipeline(mDevice, mPipeline, nullptr);
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

	error pipelineData::init(
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
		pipelineType type
	)
	{
		mNeedDescriptorUpdate = true;

		// Buffer is mapped and we need CPU readback.
		mBufferMapFlags = { true, false };

		VkPushConstantRange pc{};
		pc.offset = 0;
		pc.size = sizeof(pushConstants);
		pc.stageFlags = VK_SHADER_STAGE_ALL;

		// init pipeline.
		mPipeline.init(device);

		//connecting the vertex and pixel shaders to the pipeline
		mPipeline.setShaders(
			static_cast<vulkanShader*>(const_cast<shader*>(taskShader.get()))->mShaderModule,
			static_cast<vulkanShader*>(const_cast<shader*>(meshShader.get()))->mShaderModule,
			static_cast<vulkanShader*>(const_cast<shader*>(pixelShader.get()))->mShaderModule
		);

		mPipeline.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		mPipeline.setPolygonMode(VK_POLYGON_MODE_FILL);

		// Back face culling is done in shaders.
		mPipeline.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		mPipeline.setMultisampling(sampleCount);

		if (type == pipelineType::opaque)
		{
			mPipeline.disableBlending();
			mPipeline.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
		}

		if (type == pipelineType::accumilation)
		{
			mPipeline.enableBlendingOITAccumulation();
			mPipeline.enableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
		}

		if (type == pipelineType::composite)
		{
			mPipeline.enableBlendingOITComposite();
			mPipeline.disableDepthtest();
		}

		//connect the image format we will draw into, from draw image
		mPipeline.setColorAttachmentFormats(colorAttachmentFormats);
		mPipeline.setDepthFormat(depthFormat);

		error err = mPipeline.build(&pc, descriptorSets, true);
		if (err)
			return err;

		mEntitiesToDelete.resize(framesInFlight);
		mEntitiesToAdd.resize(framesInFlight);
		mUploadedEntities.resize(framesInFlight);
		mMeshCount.resize(framesInFlight);

		mCmdBufferSize = 2 << 24;
		mCmdBuffer.resize(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			mCmdBuffer[i].init(device, allocator, mBufferMapFlags);

			err = mCmdBuffer[i].build(is, nullptr, mCmdBufferSize, 0);
			if (err)
				return err;
		}

		return {};
	}

	void pipelineData::destroy()
	{
		mPipeline.destroy();

		for (auto& b : mCmdBuffer)
			b.destroy();
	
		mCmdBuffer.clear();
	}

	error pipelineData::addInstance(const pipelineData::addInstanceParams& params)
	{
		if (!mEntitiesToAdd[params.frameIndex].contains(params.instanceID) && !mUploadedEntities[params.frameIndex].contains(params.instanceID))
		{
			for (auto& m : params.meshesData)
			{
				mMeshCount[params.frameIndex][m.meshID]++;

				uint32_t baseOffset = m.meshletHandle.offset / uint32_t(sizeof(meshlet));

				for (uint32_t i = 0; i < m.meshlets.second; i++)
				{
					mEntitiesToAdd[params.frameIndex][params.instanceID].push_back(
						meshletShaderCMD{
							.instanceIndex = params.perInstanceHandle.bufferIndex,
							.instanceOffset = params.perInstanceHandle.offset / uint32_t(sizeof(perInstanceAttr)),
							.meshletIndex = m.meshletHandle.bufferIndex,
							.meshletOffset1 = baseOffset + i,
							.meshletOffset2 = i < m.meshlets.third - m.meshlets.second ? baseOffset + i + m.meshlets.second : std::numeric_limits<uint32_t>::max(),
							.meshletOffset3 = i < m.meshlets.fourth - m.meshlets.third ? baseOffset + i + m.meshlets.third : std::numeric_limits<uint32_t>::max(),
							.meshletOffset4 = i < m.meshlets.data.size() - m.meshlets.fourth ? baseOffset + i + m.meshlets.fourth : std::numeric_limits<uint32_t>::max(),
							.meshIndex = m.meshHandle.bufferIndex,
							.meshOffset = m.meshHandle.offset / uint32_t(sizeof(perMeshAttributes)),
							.visabilityBit = NOT_VISIBLE_FLAG_BIT,
							.selectedLod = 1,
						}
					);
				}
			}
		}

		return {};
	}

	void pipelineData::removeInstance(const pipelineData::removeInstanceParams& params)
	{
		if (mEntitiesToDelete[params.frameIndex].contains(params.instanceID))
			return;

		if (!mEntitiesToAdd[params.frameIndex].contains(params.instanceID) && !mUploadedEntities[params.frameIndex].contains(params.instanceID))
			return;

		mEntitiesToDelete[params.frameIndex].insert(params.instanceID);

		if (auto found = mMeshCount[params.frameIndex].find(params.meshID); found != mMeshCount[params.frameIndex].end() && found->second != 0)
			found->second--;
	}

	error pipelineData::updateCommandBuffer(const updateCommandBufferParams& params)
	{
		std::vector<entityHash> toRemove;

		for (auto& instanceID : mEntitiesToDelete[params.frameIndex])
		{
			if (mEntitiesToAdd[params.frameIndex].contains(instanceID))
				toRemove.push_back(instanceID);
		}

		for (auto& id : toRemove)
		{
			mEntitiesToAdd[params.frameIndex].erase(id);
			mEntitiesToDelete[params.frameIndex].erase(id);
		}

		for (auto& [k, v] : mEntitiesToAdd[params.frameIndex])
		{
			if (!mUploadedEntities[params.frameIndex].contains(k))
			{
				size_t size = v.size() * sizeof(meshletShaderCMD);
				size_t offset = mCmdBuffer[params.frameIndex].getLoadedBytes();

				error err = mCmdBuffer[params.frameIndex].updateBuffer(params.is, v.data(), size, offset);
				if (err && err.is(errCodeBufferOverFlow))
				{
					mNeedDescriptorUpdate = true;

					mCmdBufferSize = uint32_t(float(mCmdBufferSize) * 1.5f);
					uint32_t minSize = uint32_t(v.size() * sizeof(meshletShaderCMD) + mCmdBuffer[params.frameIndex].getLoadedBytes());

					if (mCmdBufferSize < minSize)
						mCmdBufferSize = minSize;

					vulkanBuffer newBuf{};

					newBuf.init(params.device, params.allocator, mBufferMapFlags);
					err = newBuf.build(params.is, mCmdBuffer[params.frameIndex], mCmdBufferSize, false);
					if (err)
						return err;

					err = newBuf.updateBuffer(params.is, v.data(), size, offset);
					if (err)
						return err;

					mCmdBuffer[params.frameIndex].destroy();

					mCmdBuffer[params.frameIndex] = std::move(newBuf);
				}

				mUploadedEntities[params.frameIndex][k] = { offset, offset + size };
			}
		}

		mEntitiesToAdd[params.frameIndex].clear();

		for (auto& k : mEntitiesToDelete[params.frameIndex])
		{
			auto uploadedEnity = mUploadedEntities[params.frameIndex].find(k);

			if (uploadedEnity != mUploadedEntities[params.frameIndex].end())
			{
				if (mCmdBuffer[params.frameIndex].getLoadedBytes() != uploadedEnity->second.second)
				{
					error err = mCmdBuffer[params.frameIndex].shiftData(params.is, uploadedEnity->second.first, uploadedEnity->second.second);
					if (err)
						return err;
				}
				else
					mCmdBuffer[params.frameIndex].markBytesAsDead(uploadedEnity->second.second - uploadedEnity->second.first);

				size_t deletedSize = uploadedEnity->second.second - uploadedEnity->second.first;

				for (auto& [_, v] : mUploadedEntities[params.frameIndex])
				{
					if (v.first >= uploadedEnity->second.second)
					{
						v.first -= deletedSize;
						v.second -= deletedSize;
					}
				}

				mUploadedEntities[params.frameIndex].erase(k);
			}
		}

		mEntitiesToDelete[params.frameIndex].clear();

		return {};
	}

	pipelineData::pipelineRenderData pipelineData::getPipelineRenderData(uint32_t frameIndex) const
	{
		auto pipe = mPipeline.getPipeline();

		return pipelineData::pipelineRenderData{
			.pipeline = pipe.first,
			.pipelineLayout = pipe.second,
			.cmdBufferCount = uint32_t(mCmdBuffer[frameIndex].getLoadedBytes() / sizeof(meshletShaderCMD)),
		};
	}

	bool pipelineData::meshIsUsed(uint32_t id, uint32_t frameIndex) const
	{
		if (auto found = mMeshCount[frameIndex].find(id); found != mMeshCount[frameIndex].end() && found->second != 0)
			return true;

		return false;
	}

	bool pipelineData::instanceExists(uint32_t id, uint32_t frameIndex) const
	{
		if (mUploadedEntities[frameIndex].find(id) != mUploadedEntities[frameIndex].end())
			return true;

		return false;
	}
	
	std::vector<VkDescriptorBufferInfo> pipelineData::getBufferInfo()
	{
		mBufferInfo.clear();

		for (auto& b : mCmdBuffer)
			mBufferInfo.push_back(VkDescriptorBufferInfo{ .buffer = b.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE });

		return mBufferInfo;
	}

	bool pipelineData::needDescriptorUpdate() const
	{
		return mNeedDescriptorUpdate;
	}

	void pipelineData::setUpdated()
	{
		mNeedDescriptorUpdate = false;
	}

	vulkanBuffer pipelineData::getBuffer(uint32_t frameIndex) const
	{
		return mCmdBuffer[frameIndex];
	}

	uint32_t pipelineData::getCommandBufferLoadedSize(uint32_t frameIndex) const
	{
		return uint32_t(mCmdBuffer[frameIndex].getLoadedBytes());
	}
}