#pragma once

#include <pch.h>
#include <vk_mem_alloc.h>

#include "platform/renderer/renderer.h"

#include "swapChain.h"
#include "pipeline.h"
#include "descriptorSet.h"
#include "submit.h"
#include "shader.h"
#include "texture.h"
#include "registry.h"

namespace engine
{
	enum handleType
	{
		allocator,
		iSub,
		sChain,
		descPool,
		descSet,
		computePipe,
		buffRegistry,
		texRegistry,
		pipelineReg,
		sampler,
		vulkanBuf,
		vulkanTex,
	};

	struct destroyTask
	{
		handleType type;
		union
		{
			VmaAllocator allocator;
			submit* iSubmit;
			swapChain* sChain;
			descriptorSet* descSet;
			computePipeline* computePipe;
			bufferRegistry* buffRegistry;
			textureRegistry* texRegistry;
			VkSampler* sampler;
			vulkanBuffer* vulkanBuf;
			pipelineRegistry* pipelineReg;
			vulkanTexture* texture;
		};
	};

	struct computePipelineBindings
	{
		uint32_t descriptorSet;
		uint32_t totalDescriptorsCount;

		uint32_t colorAttachment;
	};

	struct geometryPipelineBindings
	{
		uint32_t descriptorSet;
		uint32_t totalDescriptorsCount;

		uint32_t vertexBinding;
		uint32_t perInstanceBinding;
		uint32_t meshletCmdBinding;
		uint32_t indexBinding;
		uint32_t primitiveBinding;
		uint32_t meshletBinding;

		uint32_t perDrawBufferUboBinding;

		uint32_t albedoBinding;
		uint32_t normalBinding;
		uint32_t metalicRoughnesBinding;
	};

	struct limits
	{
		uint32_t maxUniformBuffers;
		uint32_t maxStorageBuffers;
		uint32_t maxCombinedImageSamplers;
		uint32_t maxImage;
		float maxFiltering;
	};

	class vulkanRenderer : public renderer
	{
	public:
		vulkanRenderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window);
		~vulkanRenderer();
		std::string getVersion() const;
		std::string getGpuName() const;
		error checkError() const;
		error changeViewPort(uint32_t width, uint32_t height);
		
		error addToRender(const model& m);
		void removeFromRender(const model& m);
		
		error render(renderer::renderCallIn in);
		
		withError<std::shared_ptr<shader>> makeShader(const std::vector<uint32_t>& src);
		withError<std::shared_ptr<texture>> makeTexture(uint8_t* data, int width, int heigth, imageChannel channel);
	private:
		withError<std::array<uint32_t, 3>> uploadMaterialData(const model& m);
		error uploadGeometryData(const model& m, const std::array<uint32_t, 3> materialMappings);

		error geometryPass(VkCommandBuffer cmd, renderer::renderCallIn in);
		void clear(VkCommandBuffer cmd);

		error initVulkan();
		error setLimits();
		error setDefaultBindings();
		error initImmediateSubmit();
		error initSwapchain(uint32_t width, uint32_t height);
		error initRegistry();
		error initDescriptors();
		error initPipelines();
		
		error loadExtensions();
		error setBackgroundDescriptors();
		error setGeometryDescriptors();
		error updateGeometryDescriptorsPerFrame(renderer::renderCallIn in);
		error updateCommandBuffer();
		error updateUboBuffers(renderer::renderCallIn in);
		error initBackgroundPipeline();

		bool mWindowMinimized;

		VkDevice mDevice;
		VmaAllocator mAllocator;
		VkInstance mInstance;
		VkPhysicalDevice mPhysicalDevice;
		limits mPhysicalDeviceLimits;
		VkDebugUtilsMessengerEXT mDebugMessenger;

		VkSurfaceKHR mSurface;
		swapChain mSwapChain;

		VkQueue mGraphicsQueue;
		uint32_t mGraphicsQueueFamily;
		
		submit mSubmit;

		computePipelineBindings mComputeBinding;
		computePipeline mComputePipeline;

		descriptorSet mDescriptorSetCompute;

		void flushDeletonQueue();
		std::deque<destroyTask> mDeletionQueue;

		// Below stuff for geometry pass.
		VkSampler mSampler;
		geometryPipelineBindings mGeometryBinding;
		descriptorSet mDescriptorSetMesh;

		bufferRegistry mVertexRegistry;
		bufferRegistry mIndexRegistry;
		bufferRegistry mPrimitiveRegistry;
		bufferRegistry mMeshletRegistry;
		bufferRegistry mPerInstanceRegistry;
		pipelineRegistry mPipelineRegistry;
		vulkanBuffer mUniformBuffer;

		textureRegistry mAlbedoRegistry;
		textureRegistry mNormalRegistry;
		textureRegistry mMetalicRoughnesRegistry;
	};

	inline VkRenderingAttachmentInfo depthAttachmentInfo(
		VkImageView view, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/)
	{
		VkRenderingAttachmentInfo depthAttachment{};
		depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachment.pNext = nullptr;

		depthAttachment.imageView = view;
		depthAttachment.imageLayout = layout;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.clearValue.depthStencil.depth = 0.f;

		return depthAttachment;
	}

	inline VkRenderingAttachmentInfo attachmentInfo(
		VkImageView view, VkClearValue* clear, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/)
	{
		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.pNext = nullptr;

		colorAttachment.imageView = view;
		colorAttachment.imageLayout = layout;
		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		if (clear) {
			colorAttachment.clearValue = *clear;
		}

		return colorAttachment;
	}

	inline VkRenderingInfo renderingInfo(VkExtent3D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment)
	{
		VkRenderingInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderInfo.pNext = nullptr;

		renderInfo.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, VkExtent2D{.width = renderExtent.width, .height = renderExtent.height} };
		renderInfo.layerCount = 1;
		renderInfo.colorAttachmentCount = 1;
		renderInfo.pColorAttachments = colorAttachment;
		renderInfo.pDepthAttachment = depthAttachment;
		renderInfo.pStencilAttachment = nullptr;

		return renderInfo;
	}
}