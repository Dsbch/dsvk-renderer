#include <pch.h>
#include "meshletRenderer.h"

namespace engine
{
	error meshletRenderer::init(
		std::shared_ptr<context> ctx,
		PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT,
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VmaAllocator allocator,
		submit& is,
		deviceLimits limits,
		graphicsPreset preset,
		VkBuffer UBObuffer
	)
	{
		if (!vkCmdDrawMeshTasksEXT)
			return { "vkCmdDrawMeshTasksEXT nullptr" };

		mVkCmdDrawMeshTasksEXT = vkCmdDrawMeshTasksEXT;

		mCtx = ctx;

		mBindings = meshletBindings{
			.descriptorSet = 0,
			.totalDescriptorsCount = 8,

			.vertexBinding = 0,
			.perInstanceBinding = 1,
			.meshletCmdBinding = 2,
			.indexBinding = 3,
			.primitiveBinding = 4,
			.meshletBinding = 5,

			.perDrawBufferUboBinding = 6,

			.materialArrayBinding = 7,
		};

		mDeletionQueue.init(device);

		mPreset = preset;

		error err = initRegistry(device, allocator, is);
		if (err)
			return err;

		err = initDescriptors(device, physicalDevice, limits, UBObuffer);
		if (err)
			return err;

		return {};
	}

	error meshletRenderer::destroy()
	{
		mDeletionQueue.flushDeletonQueue();

		return {};
	}

	error meshletRenderer::initRegistry(VkDevice device, VmaAllocator allocator, submit& is)
	{
		mVertexRegistry.init(device, allocator);

		mIndexRegistry.init(device, allocator);

		mPrimitiveRegistry.init(device, allocator);

		mMeshletRegistry.init(device, allocator);

		mPerInstanceRegistry.init(device, allocator);

		error err = mPipelineRegistry.init(device, allocator, is);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mVertexRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mIndexRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mPrimitiveRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mMeshletRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = buffRegistry, .buffRegistry = &mPerInstanceRegistry });

		mDeletionQueue.addDestroyTask(destroyTask{ .type = pipelineReg, .pipelineReg = &mPipelineRegistry });

		auto samp = descriptorSet::createSampler(device, float(mPreset.anisotropicFiltering));
		if (!samp)
		{
			return samp.err();
		}

		mSampler = samp.value();

		mDeletionQueue.addDestroyTask(destroyTask{ .type = sampler, .sampler = &mSampler });

		mMaterialRegistry.init(mSampler);

		return {};
	}

	error meshletRenderer::initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, deviceLimits limits, VkBuffer UBObuffer)
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

		const uint32_t combinedImageSamplers = 1;
		const uint32_t bufferObjects = 6;
		const uint32_t uniformObjects = 1;

		// add bindings for buffers.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.vertexBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perInstanceBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.meshletCmdBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.indexBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.primitiveBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.meshletBinding, limits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perDrawBufferUboBinding, limits.maxUniformBuffers / uniformObjects, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			)
		);

		// add bindings for materials.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.materialArrayBinding, limits.maxCombinedImageSamplers / combinedImageSamplers, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		err = mDescriptorSet.build(VK_SHADER_STAGE_ALL, mBindings.totalDescriptorsCount);
		if (err)
			return err;

		// Set ubo buffer write right away.
		std::vector<VkDescriptorBufferInfo> bufferInfo{
			VkDescriptorBufferInfo{.buffer = UBObuffer, .offset = 0, .range = VK_WHOLE_SIZE }
		};

		auto writeInfo = descriptorSet::getWriteInfo(mBindings.perDrawBufferUboBinding, bufferInfo, true);
		mDescriptorSet.updateWrite(writeInfo);

		mDeletionQueue.addDestroyTask(destroyTask{ .type = descSet, .descSet = &mDescriptorSet });

		return {};
	}

	error meshletRenderer::geometryPass(VkCommandBuffer cmd, renderer::renderCallIn in)
	{
		auto pipelines = mPipelineRegistry.getPipelines();

		for (auto& [_, v] : pipelines)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, v.pipeline);

			pushConstants pc{
				.commandBufferOffset = v.cmdPipelineStartOffset,
				.meshletCount = v.cmdPipelineEndOffset - v.cmdPipelineStartOffset,
			};

			vkCmdPushConstants(cmd, v.layout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

			// bind the descriptor set.
			auto set = mDescriptorSet.getDescriptorSet().first;
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, v.layout, mBindings.descriptorSet, 1, &set, 0, nullptr);

			mVkCmdDrawMeshTasksEXT(cmd, uint32_t(pc.meshletCount) / mCtx->config.inner.render.shaderWorkGroup + 1, 1, 1);
		}

		return {};
	}

	error meshletRenderer::addToRender(VkDevice device, submit& is, VkFormat depthFormat, VkFormat drawFormat, const model& m)
	{
		auto meshShader = mCtx->mAmanager->getDefaultMeshShader();
		if (!meshShader)
			return meshShader.err();

		auto taskShader = mCtx->mAmanager->getDefaultTaskShader();
		if (!taskShader)
			return taskShader.err();

		error err = mPipelineRegistry.createPipeline(
			device,
			m.mat.pixelShader,
			meshShader.value(),
			taskShader.value(),
			{ mDescriptorSet.getDescriptorSet().second },
			depthFormat,
			drawFormat,
			mPreset
		);
		if (err)
			return err;

		if (mPipelineRegistry.instanceExists(m.id))
			return {};

		// Upload material.
		materialRegistry::materialOffsets materialOffsets = mMaterialRegistry.addMaterial(m.mat.textures);

		perInstanceAttr attr = m.instanceAttributes;
		attr.albedoIndex = materialOffsets.albedo;
		attr.normalIndex = materialOffsets.normal;
		attr.metallicRoughnesIndex = materialOffsets.metalicRoughnes;

		auto handle = mVertexRegistry.addBlock(
			m.meshData.getHash(),
			m.meshData.vertex->data(),
			m.meshData.vertex->size() * sizeof(vertex),
			is
		);
		if (!handle)
			return handle.err();

		// Upload geometry.
		std::vector<meshlet> meshlets = *m.meshData.mesh.data.get();

		for (auto& m : meshlets)
		{
			m.vertexBufferOffset += handle.value().offset / uint32_t(sizeof(vertex));
			m.vertexBufferIndex = handle.value().bufferIndex;
		}

		handle = mIndexRegistry.addBlock(
			m.meshData.getHash(),
			m.meshData.index.data->data(),
			m.meshData.index.data->size() * sizeof(uint32_t),
			is
		);
		if (!handle)
			return handle.err();

		for (auto& m : meshlets)
		{
			m.indexBufferOffset += handle.value().offset / uint32_t(sizeof(uint32_t));
			m.indexBufferIndex = handle.value().bufferIndex;
		}

		handle = mPrimitiveRegistry.addBlock(
			m.meshData.getHash(),
			m.meshData.primitive.data->data(),
			m.meshData.primitive.data->size() * sizeof(uint32_t),
			is
		);
		if (!handle)
			return handle.err();

		for (auto& m : meshlets)
		{
			m.triangleBufferOffset += handle.value().offset / uint32_t(sizeof(uint32_t));
			m.triangleBufferIndex = handle.value().bufferIndex;
		}

		handle = mMeshletRegistry.addBlock(
			m.meshData.getHash(),
			meshlets.data(),
			meshlets.size() * sizeof(meshlet),
			is
		);
		if (!handle)
			return handle.err();

		auto perInstanceHandle = mPerInstanceRegistry.addBlock(
			m.id,
			&attr,
			sizeof(perInstanceAttr),
			is
		);
		if (!perInstanceHandle)
			return perInstanceHandle.err();

		err = mPipelineRegistry.addInstance(
			m.mat.pixelShader->hash(),
			m.id,
			m.meshData.getHash(),
			handle.value(),
			perInstanceHandle.value(),
			m.meshData.mesh
		);
		if (err)
			return err;

		return {};
	}

	error meshletRenderer::updateInstance(const model& m, submit& is)
	{
		// Upload/get material.
		materialRegistry::materialOffsets materialOffsets = mMaterialRegistry.addMaterial(m.mat.textures);

		// Form new instance attrs.
		perInstanceAttr attr = m.instanceAttributes;
		attr.albedoIndex = materialOffsets.albedo;
		attr.normalIndex = materialOffsets.normal;
		attr.metallicRoughnesIndex = materialOffsets.metalicRoughnes;

		return mPerInstanceRegistry.updateBlock(m.id, &attr, sizeof(perInstanceAttr), is);
	}

	void meshletRenderer::removeFromRender(const model& m)
	{
		mPerInstanceRegistry.deleteBlock(m.id);

		// Remove instance.
		mPipelineRegistry.removeInstance(m.mat.pixelShader->hash(), m.id, m.meshData.getHash());

		// Mesh isn't used.
		if (!mPipelineRegistry.meshIsUsed(m.meshData.getHash()))
		{
			mVertexRegistry.deleteBlock(m.meshData.getHash());

			mIndexRegistry.deleteBlock(m.meshData.getHash());

			mPrimitiveRegistry.deleteBlock(m.meshData.getHash());

			mMeshletRegistry.deleteBlock(m.meshData.getHash());
		}

		mMaterialRegistry.deleteMaterial(m.mat.textures);
	}

	error meshletRenderer::updateDescriptors(renderer::renderCallIn in, submit& is)
	{
		// Update command buffer for mesh pipeline.
		error err = mPipelineRegistry.updateCommandBuffer(is);
		if (err)
			return err;

		// update buffers.
		if (mPipelineRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPipelineRegistry.getWriteInfo(mBindings.meshletCmdBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mPipelineRegistry.setUpdated();
		}

		if (mVertexRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mVertexRegistry.getWriteInfo(mBindings.vertexBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mVertexRegistry.setUpdated();
		}

		if (mIndexRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mIndexRegistry.getWriteInfo(mBindings.indexBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mIndexRegistry.setUpdated();
		}

		if (mPrimitiveRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPrimitiveRegistry.getWriteInfo(mBindings.primitiveBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mPrimitiveRegistry.setUpdated();
		}

		if (mMeshletRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mMeshletRegistry.getWriteInfo(mBindings.meshletBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mMeshletRegistry.setUpdated();
		}

		if (mPerInstanceRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPerInstanceRegistry.getWriteInfo(mBindings.perInstanceBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mPerInstanceRegistry.setUpdated();
		}

		// Update materials.
		if (mMaterialRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mMaterialRegistry.getWriteInfo(mBindings.materialArrayBinding);
			mDescriptorSet.updateWrite(writeInfo);
			mMaterialRegistry.setUpdated();
		}

		return {};
	}

	error meshletRenderer::updateGraphicsPreset()
	{
		return {};
	}
}
