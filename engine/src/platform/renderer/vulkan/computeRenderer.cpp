#include <pch.h>
#include "computeRenderer.h"

namespace engine
{
	error computeRenderer::init(
		std::shared_ptr<context> ctx,
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VmaAllocator allocator,
		submit& is,
		deviceLimits limits,
		graphicsPreset preset,
		VkBuffer UBObuffer,
		const swapChain& sChain
	)
	{

		mCtx = ctx;

		mBindings = computeBindings{
			.descriptorSet = 0,
			.totalDescriptorsCount = 8,

			.orignalZBufferBinding = 0,
			.hzbBinding = 1,

			.cmdOpaqueBufferBinding = 2,
			.cmdAccumilationBufferBinding = 3,

			.perMeshBufferBinding = 4,
			.meshletBufferBinding = 5,
			.perInstanceBufferBinding = 6,

			.perDrawDataBufferBinding = 7,
		};

		mDeletionQueue.init(device);

		mPreset = preset;

		error err = initDescriptors(device, physicalDevice, UBObuffer, limits);
		if (err)
			return err;

		err = initComputePipeline(device);
		if (err)
			return err;

		err = updateSwapchainDependentDescriptors(sChain);
		if (err)
			return err;

		return {};
	}

	error computeRenderer::destroy()
	{
		mDeletionQueue.flushDeletonQueue();

		return {};
	}

	error computeRenderer::buildHZB(VkCommandBuffer cmd, renderer::renderCallIn in, const swapChain& sChain)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mBuildHzbPipeline.getPipeline().first);

		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mBuildHzbPipeline.getPipeline().second, mBindings.descriptorSet, 1, &set, 0, nullptr);

		std::vector<vulkanImage> hzbBuf = sChain.getHZB();
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

	error computeRenderer::cullMeshlets(VkCommandBuffer cmd, renderer::renderCallIn in, computeRenderer::cullMeshletsParams params)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mCullingPipeline.getPipeline().first);

		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mCullingPipeline.getPipeline().second, mBindings.descriptorSet, 1, &set, 0, nullptr);

		computePushConstants pc{
			.cullingPassFlagBit = params.cullStage,
			.opaqueCmdBufferIndex = params.opaqueCmdBufferIndex,
			.meshletCount = params.meshletCount,
			.hzbLength = params.hzbLength,
		};

		vkCmdPushConstants(cmd, mCullingPipeline.getPipeline().second, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(computePushConstants), &pc);

		vkCmdDispatch(cmd, uint32_t(pc.meshletCount) / mCtx->config.inner.render.shaderWorkGroup + 1, 1, 1);

		return {};
	}

	error computeRenderer::updateSwapchainDependentDescriptors(const swapChain& sChain)
	{
		std::vector<VkDescriptorImageInfo> originalZInfo{ VkDescriptorImageInfo{} };
		originalZInfo.front().sampler = mSampler;
		originalZInfo.front().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		originalZInfo.front().imageView = sChain.getDepthImageView(mPreset.msaa > 1);

		std::vector<VkWriteDescriptorSet> wSet = descriptorSet::getWriteInfo(mBindings.orignalZBufferBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, originalZInfo);

		mDescriptorSet.updateWrite(wSet);

		std::vector<vulkanImage> hzb = sChain.getHZB();
		std::vector<VkDescriptorImageInfo> hzbInfo{};

		for (auto& h : hzb)
		{
			VkDescriptorImageInfo imgInfo{
				.sampler = mSampler,
				.imageView = h.img.view,
				.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
			};

			hzbInfo.push_back(std::move(imgInfo));
		}

		wSet = descriptorSet::getWriteInfo(mBindings.hzbBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, hzbInfo);

		mDescriptorSet.updateWrite(wSet);

		return {};
	}

	error computeRenderer::updateOpaqueCmdBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info)
	{
		auto writeInfo = descriptorSet::getWriteInfo(mBindings.cmdOpaqueBufferBinding, info);

		mDescriptorSet.updateWrite(writeInfo);

		return {};
	}

	error computeRenderer::updateAccumilationCmdBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info)
	{
		auto writeInfo = descriptorSet::getWriteInfo(mBindings.cmdAccumilationBufferBinding, info);

		mDescriptorSet.updateWrite(writeInfo);

		return {};
	}

	error computeRenderer::updatePerMeshBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info)
	{
		auto writeInfo = descriptorSet::getWriteInfo(mBindings.perMeshBufferBinding, info);

		mDescriptorSet.updateWrite(writeInfo);

		return {};
	}

	error computeRenderer::updateMeshletBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info)
	{
		auto writeInfo = descriptorSet::getWriteInfo(mBindings.meshletBufferBinding, info);

		mDescriptorSet.updateWrite(writeInfo);

		return {};
	}

	error computeRenderer::updatePerInstancetBufferDescriptors(std::vector<VkDescriptorBufferInfo>& info)
	{
		auto writeInfo = descriptorSet::getWriteInfo(mBindings.perInstanceBufferBinding, info);

		mDescriptorSet.updateWrite(writeInfo);

		return {};
	}

	error computeRenderer::initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer UBObuffer, deviceLimits limits)
	{
		error err = mDescriptorSet.init(
			device,
			physicalDevice,
			poolConstraints{
				.maxImageDescriptors = limits.maxImage,
				.maxCombinedImageDescriptors = limits.maxCombinedImageSamplers,
				.maxBuffersDescriptors = limits.maxStorageBuffers,
				.maxUniformBuffersDescriptors = limits.maxUniformBuffers,
			}
			);
		if (err)
			return err;

		const uint32_t imageStorage = 1;
		const uint32_t combinedImageSamplers = 1;
		const uint32_t bufferObjects = 5;
		const uint32_t uniformBufferObjects = 1;

		// add bindings for hzb.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.orignalZBufferBinding, limits.maxCombinedImageSamplers / combinedImageSamplers, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.hzbBinding, limits.maxImage / imageStorage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
			)
		);

		// add bindings for cmd buffers.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.cmdOpaqueBufferBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.cmdAccumilationBufferBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		// add bindings for perMesh and meshlet buffers.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perMeshBufferBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.meshletBufferBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perInstanceBufferBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perDrawDataBufferBinding, limits.maxStorageBuffers / uniformBufferObjects, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			)
		);

		err = mDescriptorSet.build(VK_SHADER_STAGE_ALL, mBindings.totalDescriptorsCount);
		if (err)
			return err;

		// Set descriptor for ubo buffer write right away.
		std::vector<VkDescriptorBufferInfo> bufferInfo{
			VkDescriptorBufferInfo{.buffer = UBObuffer, .offset = 0, .range = VK_WHOLE_SIZE }
		};

		auto writeInfo = descriptorSet::getWriteInfo(mBindings.perDrawDataBufferBinding, bufferInfo, true);
		mDescriptorSet.updateWrite(writeInfo);

		mDeletionQueue.addDestroyTask(destroyTask{ .type = descSet, .descSet = &mDescriptorSet });

		auto samp = descriptorSet::createSampler(device, float(mPreset.anisotropicFiltering));
		if (!samp)
			return samp.err();

		mSampler = samp.value();

		mDeletionQueue.addDestroyTask(destroyTask{ .type = sampler, .sampler = &mSampler });

		return {};
	}

	error computeRenderer::initComputePipeline(VkDevice device)
	{
		// Init build HZB pipeline.
		VkShaderModule chHZBmodule;
		auto csHZBShder = mCtx->mAmanager->getHzbGenShader();
		if (!csHZBShder)
			return csHZBShder.err();

		chHZBmodule = static_cast<vulkanShader*>(const_cast<shader*>(csHZBShder.value().get()))->mShaderModule;

		mBuildHzbPipeline.init(device);
		mBuildHzbPipeline.setShader(chHZBmodule);

		VkPushConstantRange pc{};
		pc.offset = 0;
		pc.size = sizeof(computePushConstants);
		pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		error buildErr = mBuildHzbPipeline.build(&pc, { mDescriptorSet.getDescriptorSet().second });
		if (buildErr)
			return buildErr;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = computePipe, .computePipe = &mBuildHzbPipeline });

		// Init culling pipeline.
		VkShaderModule cullingModule;
		auto csCullingShader = mCtx->mAmanager->getCullingShader();
		if (!csCullingShader)
			return csCullingShader.err();

		cullingModule = static_cast<vulkanShader*>(const_cast<shader*>(csCullingShader.value().get()))->mShaderModule;

		mCullingPipeline.init(device);
		mCullingPipeline.setShader(cullingModule);

		pc.offset = 0;
		pc.size = sizeof(computePushConstants);
		pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		buildErr = mCullingPipeline.build(&pc, { mDescriptorSet.getDescriptorSet().second });
		if (buildErr)
			return buildErr;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = computePipe, .computePipe = &mCullingPipeline });

		return {};
	}
}