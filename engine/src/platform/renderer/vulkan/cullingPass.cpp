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

		auto hzbBuf = mResourceManager->getCurrentFrameHZB();
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

			pushConstants pc{
				.frameIndex = frameIndex,
				.hzbMipLevel = i,
				.mipWidth = mipWidth,
				.mipHeight = mipHeight,
			};

			vkCmdPushConstants(cmd, mBuildHzbPipeline.getPipeline().second, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pc);

			uint32_t groupCountX = (mipWidth) / 32 + 1;
			uint32_t groupCountY = (mipHeight) / 32 + 1;

			vkCmdDispatch(cmd, groupCountX, groupCountY, 1);
		}

		return {};
	}

	error cullingPass::cullMeshes(VkCommandBuffer cmd, renderer::renderParams in, cullMeshletsParams params, uint32_t frameIndex)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mCullingMeshPipeline.getPipeline().first);

		mResourceManager->bindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			mCullingMeshPipeline.getPipeline().second
		);

		pushConstants pc{
			.frameIndex = frameIndex,
			.cullingPassFlagBit = params.cullStage,
			.cmdOpaqueBufferIndex = params.opaqueCmdBufferIndex,
			.meshCount = params.meshCount,
			.hzbLength = params.hzbLength,
			.compactCapacity = params.compactCapacity,
		};

		vkCmdPushConstants(cmd, mCullingMeshPipeline.getPipeline().second, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pc);

		vkCmdDispatch(
			cmd,
			params.meshCount / mCtx->config.inner.render.cullingThreadCount + 1,
			1,
			1
		);

		return {};
	}

	error cullingPass::cullMeshlets(VkCommandBuffer cmd, renderer::renderParams in, cullingPass::cullMeshletsParams params, uint32_t frameIndex)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mCullingMeshletPipeline.getPipeline().first);

		mResourceManager->bindDescriptorSets(
			cmd,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			mCullingMeshletPipeline.getPipeline().second
		);

		pushConstants pc{
			.frameIndex = frameIndex,
			.cullingPassFlagBit = params.cullStage,
			.cmdOpaqueBufferIndex = params.opaqueCmdBufferIndex,
			.meshCount = params.meshCount,
			.hzbLength = params.hzbLength,
			.compactCapacity = params.compactCapacity,
		};

		vkCmdPushConstants(cmd, mCullingMeshletPipeline.getPipeline().second, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pc);

		vkCmdDispatchIndirect(
			cmd,
			params.dispatchBuffer.getBuffer().buffer,
			sizeof(uint32_t)
		);

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
		pc.size = sizeof(pushConstants);
		pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		error buildErr = mBuildHzbPipeline.build(
			&pc,
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second }
		);
		if (buildErr)
			return buildErr;

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::computePipe, .computePipe = &mBuildHzbPipeline });

		// Init culling meshlet pipeline.
		VkShaderModule cullingMeshletModule;
		auto csCullingMeshletShader = mCtx->mAmanager->getCullingMeshletShader();
		if (!csCullingMeshletShader)
			return csCullingMeshletShader.err();

		cullingMeshletModule = static_cast<vulkanShader*>(const_cast<shader*>(csCullingMeshletShader.value().get()))->mShaderModule;

		mCullingMeshletPipeline.init(mVulkanCtx->device);
		mCullingMeshletPipeline.setShader(cullingMeshletModule);

		pc.offset = 0;
		pc.size = sizeof(pushConstants);
		pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		buildErr = mCullingMeshletPipeline.build(
			&pc,
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second }
		);
		if (buildErr)
			return buildErr;

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::computePipe, .computePipe = &mCullingMeshletPipeline });

		// Init culling mesh pipeline.
		VkShaderModule cullingMeshModule;
		auto csCullingMeshShader = mCtx->mAmanager->getCullingMeshShader();
		if (!csCullingMeshShader)
			return csCullingMeshShader.err();

		cullingMeshModule = static_cast<vulkanShader*>(const_cast<shader*>(csCullingMeshShader.value().get()))->mShaderModule;

		mCullingMeshPipeline.init(mVulkanCtx->device);
		mCullingMeshPipeline.setShader(cullingMeshModule);

		pc.offset = 0;
		pc.size = sizeof(pushConstants);
		pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		buildErr = mCullingMeshPipeline.build(
			&pc,
			{ mResourceManager->getBufferDescriptorSet().second, mResourceManager->getTextureDescriptorSet().second }
		);
		if (buildErr)
			return buildErr;

		mVulkanCtx->delQueue.addDestroyTask(destroyTask{ .type = handleType::computePipe, .computePipe = &mCullingMeshPipeline });

		return {};
	}
}