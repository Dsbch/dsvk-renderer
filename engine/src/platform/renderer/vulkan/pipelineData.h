#pragma once
#include <pch.h>

#include "vulkanPipeline.h"
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
			VmaAllocator allocator,
			immediateSubmit immSubmit,
			std::shared_ptr<shader> pixelShader,
			std::shared_ptr<shader> meshShader,
			std::shared_ptr<shader> taskShader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			VkFormat depthFormat,
			VkFormat colorAttachmentFormat,
			uint32_t defaultMeshletToInstanceBuffSize = 2 << 21
		);
		bool instanceExists(uint32_t id) const;
		error addInstance(uint32_t id, uint32_t meshID, bufferHandle meshletHandle, uint32_t meshletCount, perInstanceAttr attr);
		error updateMeshletToInstanceBuffer();
		void removeInstance(uint32_t id, uint32_t meshID);
		uint32_t getMeshInstanceCount(uint32_t meshID) const;
		bool needPerInstanceDecriptorUpdate() const;
		void setPerInstanceDecriptorUpdated();
		bool needMeshletToInstanceDescriptorUpdate() const;
		void setMeshletToInstanceDescriptorUpdated();
		std::vector<VkWriteDescriptorSet> getPerInstanceWriteInfo(uint32_t binding);
		std::vector<VkWriteDescriptorSet> getMeshletToInstanceWriteInfo(uint32_t binding);
		uint32_t getTaskShaderCount();
		void destroy();
		std::pair<VkPipeline, VkPipelineLayout> getPipeline();
	private:
		VkDevice mDevice;
		VmaAllocator mAllocator;
		immediateSubmit mImmediateSubmit;

		classicGraphicPipeline mPipeline;

		bufferRegistry mPerInstanceRegistry;

		bool mUpdateMeshletToInstanceBuffer;
		bool mUpdateMeshletToInstanceDescriptor;
		vulkanBuffer mMeshletToInstanceBuffer;
		uint32_t mNewMeshletToInstanceSize;
		std::map<uint32_t, std::vector<meshletToInstance>> mMeshletToInstanceData;
		std::map<uint32_t, uint32_t> mInstanceMeshCount;

		std::vector<VkDescriptorBufferInfo> mMeshletToInstanceBufferInfo;
	};
}