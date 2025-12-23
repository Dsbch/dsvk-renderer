#include <pch.h>

#include "pipelineData.h"
#include "vulkanDescriptorSet.h"

namespace engine
{
	error pipelineData::init(
		VkDevice device,
		std::shared_ptr<shader> pixelShader,
		std::shared_ptr<shader> meshShader,
		std::shared_ptr<shader> taskShader,
		const std::vector<VkDescriptorSetLayout>& descriptorSets,
		VkFormat depthFormat,
		VkFormat colorAttachmentFormat
	)
	{
		mNeedUpdate = false;

		mDevice = device;

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

		error err = mPipeline.build(&pc, descriptorSets, true);
		if (err)
			return err;

		return {};
	}

	bool pipelineData::instanceExists(uint32_t id) const
	{
		return mMeshletShaderCMD.find(id) != mMeshletShaderCMD.end();
	}

	void pipelineData::destroy()
	{
		mPipeline.destroy();
	}

	std::pair<VkPipeline, VkPipelineLayout> pipelineData::getPipeline()
	{
		return mPipeline.getPipeline();
	}

	std::vector<meshletShaderCMD> pipelineData::getPipelineCMD() const
	{
		std::vector<meshletShaderCMD> result;

		for (auto& [_, v] : mMeshletShaderCMD)
		{
			result.insert(result.end(), v.begin(), v.end());
		}

		return result;
	}

	error pipelineData::addInstance(uint32_t id, uint32_t meshID, bufferHandle meshletHandle, bufferHandle perInstanceHandle, const dataWithLodLevels<meshlet>& mesh, perInstanceAttr attr)
	{
		if (mMeshletShaderCMD.find(id) != mMeshletShaderCMD.end())
			return {};
		
		mNeedUpdate = true;

		mInstanceMeshCount[meshID]++;

		std::vector<meshletShaderCMD> meshCMD;

		uint32_t baseOffset = meshletHandle.offset / uint32_t(sizeof(meshlet));

		for (uint32_t i = 0; i < mesh.second; i++)
		{
			meshCMD.push_back(
				meshletShaderCMD{
					.instanceIndex = perInstanceHandle.bufferIndex,
					.instanceOffset = perInstanceHandle.offset / uint32_t(sizeof(perInstanceAttr)),
					.meshletIndex = meshletHandle.bufferIndex,
					.meshletOffset1 = baseOffset + i,
					.meshletOffset2 = i < mesh.third - mesh.second ? baseOffset + i + mesh.second : std::numeric_limits<uint32_t>::max(),
					.meshletOffset3 = i < mesh.fourth - mesh.third ? baseOffset + i + mesh.third : std::numeric_limits<uint32_t>::max(),
					.meshletOffset4 = i < mesh.data->size() - mesh.fourth ? baseOffset + i + mesh.fourth : std::numeric_limits<uint32_t>::max()
				}
			);
		}

		mMeshletShaderCMD[id] = meshCMD;

		return {};
	}

	void pipelineData::removeInstance(uint32_t id, uint32_t meshID)
	{
		if (mMeshletShaderCMD.find(id) == mMeshletShaderCMD.end())
			return;

		if (auto found = mInstanceMeshCount.find(meshID); found != mInstanceMeshCount.end() && found->second != 0)
			found->second--;

		mMeshletShaderCMD.erase(id);

		mNeedUpdate = true;
	}

	uint32_t pipelineData::getMeshInstanceCount(uint32_t meshID) const
	{
		if (auto found = mInstanceMeshCount.find(meshID); found != mInstanceMeshCount.end())
			return found->second;

		return 0;
	}

	uint32_t pipelineData::getTaskShaderCount()
	{
		uint32_t result = 0;

		for (const auto [_, v] : mMeshletShaderCMD)
		{
			result += uint32_t(v.size());
		}

		return result;
	}

	bool pipelineData::needUpdate() const
	{
		return mNeedUpdate;
	}

	void pipelineData::setUpdated()
	{
		mNeedUpdate = false;
	}
}