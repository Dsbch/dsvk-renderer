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
#include "ui.h"

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

		uint32_t materialArrayBinding;
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
		error uploadGeometryData(const model& m, uint32_t albedoIndex, uint32_t normalIndex, uint32_t metalicRoughnesIndex);

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

		materialRegistry mMaterialRegistry;

		vulkanUI mUi;
	};
}