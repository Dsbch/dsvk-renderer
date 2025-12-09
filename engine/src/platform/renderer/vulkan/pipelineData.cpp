#include <pch.h>

#include "pipelineData.h"
#include "vulkanDescriptorSet.h"

namespace engine
{
	error pipelineData::init(
		VkDevice device,
		VmaAllocator allocator,
		immediateSubmit immSubmit,
		std::shared_ptr<shader> pixelShader,
		std::shared_ptr<shader> meshShader,
		std::shared_ptr<shader> taskShader,
		const std::vector<VkDescriptorSetLayout>& descriptorSets,
		VkFormat depthFormat,
		VkFormat colorAttachmentFormat,
		uint32_t defaultMeshletToInstanceBuffSize
	)
	{
		mDevice = device;
		mAllocator = allocator;
		mImmediateSubmit = immSubmit;
		mNewMeshletToInstanceSize = defaultMeshletToInstanceBuffSize;
		mUpdateMeshletPerInstanceBuffer = false;

		// init buffers, registry.
		mPerInstanceRegistry.init(mDevice, mAllocator, mImmediateSubmit);

		mMeshletToInstanceBuffer.init(mDevice, mAllocator);

		auto err = mMeshletToInstanceBuffer.build(mImmediateSubmit, nullptr, mNewMeshletToInstanceSize, 0);
		if (err)
			return err;

		auto pc = pixelShader->getPushConstant();
		VkPushConstantRange pushConstant{};
		pushConstant.offset = pc.offset;
		pushConstant.size = pc.size;
		pushConstant.stageFlags = VK_SHADER_STAGE_ALL;

		// init pipeline.
		mPipeline.init(mDevice);

		//connecting the vertex and pixel shaders to the pipeline
		mPipeline.setShaders(static_cast<vulkanShader*>(taskShader.get())->mShaderModule, static_cast<vulkanShader*>(meshShader.get())->mShaderModule, static_cast<vulkanShader*>(pixelShader.get())->mShaderModule);
		//it will draw triangles
		mPipeline.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		//filled triangles
		mPipeline.setPolygonMode(VK_POLYGON_MODE_FILL);
		//no backface culling
		mPipeline.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
		//no multisampling
		mPipeline.setMultisamplingNone();
		//no blending
		mPipeline.disableBlending();
		mPipeline.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

		//connect the image format we will draw into, from draw image
		mPipeline.setColorAttachmentFormat(colorAttachmentFormat);
		mPipeline.setDepthFormat(depthFormat);

		err = mPipeline.build(&pushConstant, descriptorSets, true);
		if (err)
			return err;

		return {};
	}

	void pipelineData::destroy()
	{
		mPipeline.destroy();
		mPerInstanceRegistry.destroy();
		mMeshletToInstanceBuffer.destroy();
		mMeshletToInstanceData.clear();
		mUpdateMeshletPerInstanceBuffer = false;
	}

	std::pair<VkPipeline, VkPipelineLayout> pipelineData::getPipeline()
	{
		return mPipeline.getPipeline();
	}

	error pipelineData::addInstance(uint32_t id, bufferHandle meshletHandle, uint32_t meshletCount, perInstanceAttr attr)
	{
		if (mMeshletToInstanceData.find(id) != mMeshletToInstanceData.end())
			return {};

		auto perIsntanceHandle = mPerInstanceRegistry.addBlock(id, &attr, sizeof(perInstanceAttr));
		if (!perIsntanceHandle)
			return perIsntanceHandle.err();

		mUpdateMeshletPerInstanceBuffer = true;

		std::vector<meshletToInstance> meshToInstance;
		meshToInstance.reserve(meshletCount);

		for (uint32_t i = 0; i < meshletCount; i++)
		{
			meshToInstance.push_back(
				meshletToInstance{
					.instanceIndex = perIsntanceHandle.value().bufferIndex,
					.instanceOffset = perIsntanceHandle.value().offset,
					.meshletIndex = meshletHandle.bufferIndex,
					.meshletOffset = meshletHandle.offset + i * uint32_t(sizeof(meshlet))
				}
			);
		}

		mMeshletToInstanceData[id] = meshToInstance;

		return {};
	}

	void pipelineData::removeInstance(uint32_t id)
	{
		if (mMeshletToInstanceData.find(id) == mMeshletToInstanceData.end())
			return;

		mMeshletToInstanceData.erase(id);
		mPerInstanceRegistry.deleteBlock(id);
		mUpdateMeshletPerInstanceBuffer = true;
	}

	error pipelineData::updateMeshletToInstanceBuffer()
	{
		if (!mUpdateMeshletPerInstanceBuffer)
			return {};

		std::vector<meshletToInstance> meshToInstance;
		meshToInstance.reserve(mMeshletToInstanceData.size());

		for (auto [_, v] : mMeshletToInstanceData)
		{
			for (auto& val : v)
				meshToInstance.push_back(val);
		}

		auto err = mMeshletToInstanceBuffer.updateBuffer(mImmediateSubmit, meshToInstance.data(), meshToInstance.size() * sizeof(meshletToInstance), 0);
		if (err && err.err() == "buffer overflow")
		{
			mMeshletToInstanceBuffer.destroy();

			mNewMeshletToInstanceSize *= 2;

			err = mMeshletToInstanceBuffer.build(
				mImmediateSubmit, meshToInstance.data(),
				mNewMeshletToInstanceSize,
				meshToInstance.size() * sizeof(meshletToInstance)
			);
			if (err)
				return err;
		}

		if (err)
			return err;

		mUpdateMeshletPerInstanceBuffer = false;

		return {};
	}

	void pipelineData::setPerInstanceDescriptorUpdated()
	{
		mPerInstanceRegistry.setUpdated();
	}

	bool pipelineData::needPerInstanceDescriptorUpdate()
	{
		return mPerInstanceRegistry.needDecriptorUpdate();
	}

	std::vector<VkWriteDescriptorSet> pipelineData::getPerInstanceWriteInfo(uint32_t binding)
	{
		return mPerInstanceRegistry.getWriteInfo(binding);
	}

	std::vector<VkWriteDescriptorSet> pipelineData::getMeshletToInstanceWriteInfo(uint32_t binding)
	{
		return descriptorSet::getWriteInfo(
			binding,
			{ VkDescriptorBufferInfo{.buffer = mMeshletToInstanceBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE } }
		);
	}

	uint32_t pipelineData::getTaskShaderCount()
	{
		uint32_t result = 0;

		for (auto [k, v] : mMeshletToInstanceData)
		{
			result += uint32_t(v.size());
		}

		return result;
	}
}