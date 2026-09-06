#include <pch.h>
#include "cullingPass.h"

namespace engine
{
	error cullingPass::init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanContext, std::shared_ptr<resourceManager> resourceManager)
	{
		mCtx = ctx;
		mVulkanCtx = vulkanContext;
		mResourceManager = resourceManager;

		error err = initComputePipeline();
		if (err)
			return err;

		return {};
	}

	error cullingPass::destroy()
	{
		return {};
	}

	error cullingPass::buildHZB(VkCommandBuffer cmd, renderer::renderParams in, uint32_t frameIndex)
	{
		if (in.useDebugCamera)
			return {};

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mBuildHzbPipeline.getPipeline().first);

		mResourceManager->bindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			mBuildHzbPipeline.getPipeline().second
		);

		auto hzbBuf = mResourceManager->getHZB();
		for (uint32_t i = 0; i < hzbBuf.size(); i++)
		{
			if (i > 0)
			{
				pipelineImageBarrier(
					cmd,
					hzbBuf[i - 1].img.image,
					hzbBuf[i - 1].img.format,
					VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_READ_BIT
				);
			}

			uint32_t mipWidth = std::max(1u, in.width >> (i + 1));
			uint32_t mipHeight = std::max(1u, in.height >> (i + 1));

			computePushConstants pc{
				.frameIndex = frameIndex,
				.hzbMipLevel = i,
				.mipWidth = mipWidth,
				.mipHeight = mipHeight,
			};

			vkCmdPushConstants(cmd, mBuildHzbPipeline.getPipeline().second, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(computePushConstants), &pc);

			uint32_t groupCountX = (mipWidth) / 32 + 1;
			uint32_t groupCountY = (mipHeight) / 32 + 1;

			vkCmdDispatch(cmd, groupCountX, groupCountY, 1);
		}

		return {};
	}

	error cullingPass::cullMeshlets(VkCommandBuffer cmd, renderer::renderParams in, cullingPass::cullMeshletsParams params, uint32_t frameIndex)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mCullingPipeline.getPipeline().first);

		mResourceManager->bindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			mBuildHzbPipeline.getPipeline().second
		);

		computePushConstants pc{
			.frameIndex = frameIndex,
			.cullingPassFlagBit = params.cullStage,
			.cmdOpaqueBufferIndex = params.opaqueCmdBufferIndex,
			.cmdBufferCount = params.cmdBufferCount,
			.hzbLength = params.hzbLength,
		};

		vkCmdPushConstants(cmd, mCullingPipeline.getPipeline().second, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(computePushConstants), &pc);

		// For accumilation pass we do not use indirect for culling.
		vkCmdDispatch(cmd, uint32_t(pc.cmdBufferCount) / mCtx->config.inner.render.shaderWorkGroup + 1, 1, 1);

		return {};
	}

	error cullingPass::compactCommandBuffer(VkCommandBuffer cmd, renderer::renderParams in, compactCommandBufferParams params, uint32_t frameIndex)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mCompactCommandsPipeline.getPipeline().first);

		mResourceManager->bindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			mBuildHzbPipeline.getPipeline().second
		);

		computePushConstants pc{
			.frameIndex = frameIndex,
			.cullingPassFlagBit = params.stage,
			.cmdOpaqueBufferIndex = params.opaqueCmdBufferIndex,
			.cmdBufferCount = params.cmdBufferCount,
			.compactRule = params.compactRule,
		};

		vkCmdPushConstants(cmd, mCompactCommandsPipeline.getPipeline().second, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(computePushConstants), &pc);

		vkCmdDispatch(cmd, uint32_t(pc.cmdBufferCount) / mCtx->config.inner.render.compactWorkGroup + 1, 1, 1);

		return {};
	}

	error cullingPass::initComputePipeline()
	{
		// Init build HZB pipeline.
		VkShaderModule chHZBmodule;
		auto csHZBShder = mCtx->mAmanager->getHzbGenShader();
		if (!csHZBShder)
			return csHZBShder.err();

		chHZBmodule = static_cast<vulkanShader*>(const_cast<shader*>(csHZBShder.value().get()))->mShaderModule;

		mBuildHzbPipeline.init(mVulkanCtx->device);
		mBuildHzbPipeline.setShader(chHZBmodule);

		VkPushConstantRange pc{};
		pc.offset = 0;
		pc.size = sizeof(computePushConstants);
		pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		error buildErr = mBuildHzbPipeline.build(
			&pc, 
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second }
		);
		if (buildErr)
			return buildErr;

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::computePipe, .computePipe = &mBuildHzbPipeline });

		// Init culling pipeline.
		VkShaderModule cullingModule;
		auto csCullingShader = mCtx->mAmanager->getCullingShader();
		if (!csCullingShader)
			return csCullingShader.err();

		cullingModule = static_cast<vulkanShader*>(const_cast<shader*>(csCullingShader.value().get()))->mShaderModule;

		mCullingPipeline.init(mVulkanCtx->device);
		mCullingPipeline.setShader(cullingModule);

		pc.offset = 0;
		pc.size = sizeof(computePushConstants);
		pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		buildErr = mCullingPipeline.build(
			&pc, 
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second }
		);
		if (buildErr)
			return buildErr;

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::computePipe, .computePipe = &mCullingPipeline });

		// Init compact pipeline.
		VkShaderModule compactModule;
		auto compactShader = mCtx->mAmanager->getDefaultComputeCompactShader();
		if (!compactShader)
			return compactShader.err();

		compactModule = static_cast<vulkanShader*>(const_cast<shader*>(compactShader.value().get()))->mShaderModule;

		mCompactCommandsPipeline.init(mVulkanCtx->device);
		mCompactCommandsPipeline.setShader(compactModule);

		pc.offset = 0;
		pc.size = sizeof(computePushConstants);
		pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		buildErr = mCompactCommandsPipeline.build(
			&pc, 
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second }
		);
		if (buildErr)
			return buildErr;

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::computePipe, .computePipe = &mCompactCommandsPipeline });

		return {};
	}
}