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
		mUpdateMeshletToInstanceBuffer = false;
		mUpdateMeshletToInstanceDescriptor = true;

		// init buffers, registry.
		mPerInstanceRegistry.init(mDevice, mAllocator, mImmediateSubmit);

		mMeshletToInstanceBuffer.init(mDevice, mAllocator);

		auto err = mMeshletToInstanceBuffer.build(mImmediateSubmit, nullptr, mNewMeshletToInstanceSize, 0);
		if (err)
			return err;

		mMeshletToInstanceBufferInfo = { VkDescriptorBufferInfo{.buffer = mMeshletToInstanceBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE } };

		VkPushConstantRange pc{};
		pc.offset = 0;
		pc.size = sizeof(pushConstants);
		pc.stageFlags = VK_SHADER_STAGE_ALL;

		// init pipeline.
		mPipeline.init(mDevice);

		//connecting the vertex and pixel shaders to the pipeline
		mPipeline.setShaders(
			static_cast<vulkanShader*>(taskShader.get())->mShaderModule,
			static_cast<vulkanShader*>(meshShader.get())->mShaderModule,
			static_cast<vulkanShader*>(pixelShader.get())->mShaderModule
		);
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

		err = mPipeline.build(&pc, descriptorSets, true);
		if (err)
			return err;

		return {};
	}

	bool pipelineData::instanceExists(uint32_t id) const
	{
		return mMeshletToInstanceData.find(id) != mMeshletToInstanceData.end();
	}

	void pipelineData::destroy()
	{
		mPipeline.destroy();
		mPerInstanceRegistry.destroy();
		mMeshletToInstanceBuffer.destroy();
		mMeshletToInstanceData.clear();
		mUpdateMeshletToInstanceBuffer = false;
		mUpdateMeshletToInstanceDescriptor = false;
	}

	std::pair<VkPipeline, VkPipelineLayout> pipelineData::getPipeline()
	{
		return mPipeline.getPipeline();
	}

	error pipelineData::addInstance(uint32_t id, uint32_t meshID, bufferHandle meshletHandle, const dataWithLodLevels<meshlet>& mesh, perInstanceAttr attr)
	{
		if (mMeshletToInstanceData.find(id) != mMeshletToInstanceData.end())
			return {};

		mInstanceMeshCount[meshID]++;

		auto perIsntanceHandle = mPerInstanceRegistry.addBlock(id, &attr, sizeof(perInstanceAttr));
		if (!perIsntanceHandle)
			return perIsntanceHandle.err();

		mUpdateMeshletToInstanceBuffer = true;

		std::vector<meshletToInstance> meshToInstance;

		uint32_t baseOffset = meshletHandle.offset / uint32_t(sizeof(meshlet));

		for (uint32_t i = 0; i < mesh.second; i++)
		{
			meshToInstance.push_back(
				meshletToInstance{
					.instanceIndex = perIsntanceHandle.value().bufferIndex,
					.instanceOffset = perIsntanceHandle.value().offset / uint32_t(sizeof(perInstanceAttr)),

					.meshletIndex = meshletHandle.bufferIndex,
					.meshletOffset1 = baseOffset + i,
					.meshletOffset2 = i < mesh.third - mesh.second ? baseOffset + mesh.second + i : std::numeric_limits<uint32_t>::max(),
					.meshletOffset3 = i < mesh.fourth - mesh.third ? baseOffset + mesh.third + i : std::numeric_limits<uint32_t>::max(),
					.meshletOffset4 = i < mesh.data->size() - mesh.fourth ? baseOffset + mesh.fourth + i : std::numeric_limits<uint32_t>::max()
				}
			);
		}

		mMeshletToInstanceData[id] = meshToInstance;

		return {};
	}

	void pipelineData::removeInstance(uint32_t id, uint32_t meshID)
	{
		if (mMeshletToInstanceData.find(id) == mMeshletToInstanceData.end())
			return;

		if (auto found = mInstanceMeshCount.find(meshID); found != mInstanceMeshCount.end() && found->second != 0)
			found->second--;

		mMeshletToInstanceData.erase(id);
		mPerInstanceRegistry.deleteBlock(id);
		mUpdateMeshletToInstanceBuffer = true;
	}

	uint32_t pipelineData::getMeshInstanceCount(uint32_t meshID) const
	{
		if (auto found = mInstanceMeshCount.find(meshID); found != mInstanceMeshCount.end())
			return found->second;

		return 0;
	}

	bool pipelineData::needPerInstanceDecriptorUpdate() const
	{
		return mPerInstanceRegistry.needDecriptorUpdate();
	}

	void pipelineData::setPerInstanceDecriptorUpdated()
	{
		return mPerInstanceRegistry.setUpdated();
	}

	bool pipelineData::needMeshletToInstanceDescriptorUpdate() const
	{
		return mUpdateMeshletToInstanceDescriptor;
	}

	void pipelineData::setMeshletToInstanceDescriptorUpdated()
	{
		mUpdateMeshletToInstanceDescriptor = false;

	}
	error pipelineData::updateMeshletToInstanceBuffer()
	{
		if (mMeshletToInstanceData.size() == 0)
			return {};

		if (!mUpdateMeshletToInstanceBuffer)
			return {};

		std::vector<meshletToInstance> meshToInstance;

		for (auto& [_, v] : mMeshletToInstanceData)
		{
			for (auto& val : v)
				meshToInstance.push_back(val);
		}

		auto err = mMeshletToInstanceBuffer.updateBuffer(mImmediateSubmit, meshToInstance.data(), meshToInstance.size() * sizeof(meshletToInstance), 0);
		if (err && err.err() == "buffer overflow")
		{
			mMeshletToInstanceBuffer.destroy();

			mNewMeshletToInstanceSize *= 2;
			if (mNewMeshletToInstanceSize < uint32_t(meshToInstance.size() * sizeof(meshletToInstance)))
			{
				mNewMeshletToInstanceSize = uint32_t(meshToInstance.size() * sizeof(meshletToInstance) * 2);
			}

			err = mMeshletToInstanceBuffer.build(
				mImmediateSubmit, meshToInstance.data(),
				mNewMeshletToInstanceSize,
				meshToInstance.size() * sizeof(meshletToInstance)
			);
			if (err)
				return err;

			mUpdateMeshletToInstanceDescriptor = true;
		}
		if (err)
			return err;

		mUpdateMeshletToInstanceBuffer = false;

		return {};
	}

	std::vector<VkWriteDescriptorSet> pipelineData::getPerInstanceWriteInfo(uint32_t binding)
	{
		return mPerInstanceRegistry.getWriteInfo(binding);
	}

	std::vector<VkWriteDescriptorSet> pipelineData::getMeshletToInstanceWriteInfo(uint32_t binding)
	{
		mMeshletToInstanceBufferInfo = { VkDescriptorBufferInfo{.buffer = mMeshletToInstanceBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE } };

		return descriptorSet::getWriteInfo(
			binding,
			mMeshletToInstanceBufferInfo
		);
	}

	uint32_t pipelineData::getTaskShaderCount()
	{
		uint32_t result = 0;

		for (const auto [_, v] : mMeshletToInstanceData)
		{
			result += uint32_t(v.size());
		}

		return result;
	}
}