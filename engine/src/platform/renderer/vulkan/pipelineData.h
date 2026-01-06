#pragma once
#include <pch.h>

#include "pipeline.h"
#include "registry.h"
#include <platform/renderer/shader.h>
#include <platform/renderer/vertex.h>

namespace engine
{
	struct pipelineData
	{
	public:
		error init(
			VkDevice device,
			std::shared_ptr<shader> pixelShader,
			std::shared_ptr<shader> meshShader,
			std::shared_ptr<shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			VkFormat colorAttachmentFormat
		);
		bool instanceExists(uint32_t id) const;
		error addInstance(uint32_t id, uint32_t meshID, bufferHandle meshletHandle, bufferHandle perInstanceHandle, const dataWithLodLevels<meshlet>& mesh, perInstanceAttr attr);
		void removeInstance(uint32_t id, uint32_t meshID);
		uint32_t getMeshInstanceCount(uint32_t meshID) const;
		uint32_t getMeshletCount() const;
		void destroy();
		std::pair<VkPipeline, VkPipelineLayout> getPipeline();

		std::vector<meshletShaderCMD> getPipelineCMD() const;

		bool needUpdate() const;
		void setUpdated();
	private:
		VkDevice mDevice;
		classicGraphicPipeline mPipeline;

		std::map<uint32_t, std::vector<meshletShaderCMD>> mMeshletShaderCMD;
		std::map<uint32_t, uint32_t> mInstanceMeshCount;

		bool mNeedUpdate;
	};
}