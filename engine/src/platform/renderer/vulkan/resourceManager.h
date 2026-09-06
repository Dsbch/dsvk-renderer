#pragma once

#include <pch.h>

#include "base/include.h"

namespace engine
{
	struct resourceManager
	{
	public:
		resourceManager() = default;
		resourceManager(const resourceManager&) = delete;

		void init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanCtx);
		error build(uint32_t width, uint32_t height);
		void destroy();
		
		std::pair<VkDescriptorSet, VkDescriptorSetLayout> getBufferDescriptorSet() const;
		std::pair<VkDescriptorSet, VkDescriptorSetLayout> getTextureDescriptorSet() const;
		VkSampler getSampler() const;

		// Should be called each frame before render.
		// Will update descriptors for all managed resources.
		error updateDescriptors(uint32_t frameIndex);
		
		error addToRender(const model& m, uint32_t frameIndex);
		error updateInstance(const model& m, uint32_t frameIndex);
		error updateAnimations(const model& m, uint32_t frameIndex);
		void removeFromRender(const model& m, uint32_t frameIndex);
		error changeViewPort(uint32_t width, uint32_t height);

		error addLine(line l);
		error updatePerDrawBuffer(perDrawData data, uint32_t frameIndex);
	
		void bindDescriptorSets(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint, VkPipelineLayout layout);

		void transitionColorAttachmentImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionDepthImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionAccumImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionRevealImage(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;
		void transitionHzbChainImages(
			VkCommandBuffer cmd,
			VkImageLayout current,
			VkImageLayout newLayout,
			VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			VkAccessFlags2 dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT
		) const;

		vulkanImage getColorAttachmentImage(bool needResolve) const;
		vulkanImage getDepthImage(bool needResolve) const;
		vulkanImage getAccumImage(bool needResolve) const;
		vulkanImage getRevealImage(bool needResolve) const;
		vulkanImage getClipMap() const;
		std::span<vulkanImage> getHZB();
		const std::unordered_map<uint32_t, commandBuffer>& getOpaqueCmdBuffers() const;
		const commandBuffer* getOpaqueCmdBuffer(uint32_t pixelShaderID) const;
		const commandBuffer& getAccumilationCmdBuffer() const;
		const vulkanBuffer& getVisabilityBuffer(uint32_t frameIndex) const;
		const vulkanBuffer& getLinebuffer() const;
	private:
		void destroyViewPortDependantResources();
		error buildResources(uint32_t width, uint32_t height);
		error buildViewPortDependantResources(uint32_t width, uint32_t height);
		error buildDescriptors();
		void updateWriteAfterViewPortChange();

		uint32_t getMaxCmdBufferSize(uint32_t frameIndex) const;

		// Control fields.
		std::shared_ptr<context> mCtx;
		std::shared_ptr<vulkanContext> mVulkanCtx;
		
		struct descriptorsBindings
		{
			uint32_t storageBufferBindings = 15;
			uint32_t uniformBufferBindings = 1;
			
			uint32_t storageImageBindings = 2;
			uint32_t combinedSampledImageBindings = 4;

			// Buffers bindings.
			uint32_t positionsBinding = 0;
			uint32_t normalBinding = 1;
			uint32_t tangentBinding = 2;
			uint32_t jointIndexBinding = 3;
			uint32_t weightBinding = 4;
			uint32_t cmdOpaqueBufferBinding = 5;
			uint32_t cmdAccumilationBufferBinding = 6;
			uint32_t perInstanceBinding = 7;
			uint32_t indexBinding = 8;
			uint32_t primitiveBinding = 9;
			uint32_t meshletBinding = 10;
			uint32_t jointsBinding = 11;
			uint32_t perMeshBinding = 12;
			uint32_t perDrawBufferUboBinding = 13;
			uint32_t visabilityBuffer = 14;
			uint32_t lineBuffer = 15;

			// Texture bindings.
			uint32_t materialArrayBinding = 0;
			uint32_t clipMapBinding = 1;
			uint32_t accumBinding = 2;
			uint32_t revealBinding = 3;
			uint32_t orignalZBufferBinding = 4;
			uint32_t hzbBinding = 5;
		};

		descriptorsBindings mBindings;
		descriptorSet mBufferDescriptorSet;
		descriptorSet mTextureDescriptorSet;

		// Managed resources.
		VkSampler mSampler;

		// Buffers.
		commandBuffer mAccumilationCommandBuffer;
		std::unordered_map<uint32_t, commandBuffer> mOpaqueCommandBuffers;
		bufferRegistry mPositionRegistry;
		bufferRegistry mNormalRegistry;
		bufferRegistry mTangentRegistry;
		bufferRegistry mJointIndexRegistry;
		bufferRegistry mWeightRegistry;
		bufferRegistry mIndexRegistry;
		bufferRegistry mPrimitiveRegistry;
		bufferRegistry mMeshletRegistry;
		bufferRegistry mPerMeshRegistry;
		bufferRegistry mPerInstanceRegistry;
		bufferRegistry mJointRegistry;
		vulkanBuffer mLineBuffer;
		std::vector<vulkanBuffer> mUboPerDrawBuffer;
		std::vector<vulkanBuffer> mVisabilityBuffer;

		// Texture buffers.
		materialRegistry mMaterialRegistry;
		vulkanImage mClipMap;
		vulkanImage mAccumImage;
		vulkanImage mRevealImage;
		vulkanImage mAccumResolveImage;
		vulkanImage mRevealResolveImage;
		vulkanImage mColorAttachmentImage;
		vulkanImage mColorAttachmentResolveImage;
		vulkanImage mDepthImage;
		vulkanImage mDepthResolveImage;
		std::vector<vulkanImage> mHZBImages;
	};
}