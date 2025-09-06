#pragma once

#include <pch.h>
#include <vma/vk_mem_alloc.h>

#include "platform/renderer/renderer.h"

#include "vulkanSwapChain.h"
#include "vulkanPipeline.h"
#include "vulkanDescriptorSet.h"
#include "vulkanImmediateSubmit.h"
#include "vulkanShader.h"
#include "vulkanTexture.h"
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
		sampler,
	};

	struct destroyTask
	{
		handleType type;
		union
		{
			VmaAllocator allocator;
			immediateSubmit* iSubmit;
			swapChain* sChain;
			descriptorSet* descSet;
			computePipeline* computePipe;
			bufferRegistry* buffRegistry;
			textureRegistry* texRegistry;
			VkSampler* sampler;
		};
	};

	struct computePipelineBindings
	{
		uint32_t descriptorSet;

		uint32_t textureBinding;
	};

	struct geometryPipelineBindings
	{
		uint32_t descriptorSet;

		uint32_t vertexBinding;
		uint32_t indexBinding;
		uint32_t primitiveBinding;
		uint32_t meshletBinding;

		uint32_t albedoBinding;
		uint32_t normalBinding;
		uint32_t roughnessBinding;
		uint32_t metalicBinding;
		uint32_t aoBinding;
	};

	struct pipelineData
	{
		classicGraphicPipeline pipeline;
		bufferRegistry perInstanceRegistry;
	};

	struct limits
	{
		uint32_t maxStorageBuffers;
		uint32_t maxCombinedImageSamplers;
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
		void changeViewPort(uint32_t width, uint32_t height);
		void addToRender(const model& m);
		void render();

		withError<std::shared_ptr<shader>> makeShader(const std::vector<uint32_t>& src);
		withError<std::shared_ptr<texture>> makeTexture(uint8_t* data, int width, int heigth, imageChannel channel);
	private:
		void clear(VkCommandBuffer cmd);

		void initVulkan();
		void setDefaultBindings();
		void loadExtensions();
		void initImmediateSubmit();
		void initSwapchain(uint32_t width, uint32_t height);
		void initRegistry();
		void initDescriptors();
		void setBackgroundDescriptors();
		void setGeometryDescriptors();
		void updateGeometryDescriptors();
		void initPipelines();
		void initBackgroundPipeline();
		void setLimits();

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
		immediateSubmit mImmediateSubmit;

		computePipelineBindings mComputeBinding;
		computePipeline mComputePipeline;

		descriptorSet mDescriptorSetCompute;


		void flushDeletonQueue();
		std::deque<destroyTask> mDeletionQueue;

		// Below stuff for geometry pass.
		VkSampler mSampler;
		geometryPipelineBindings mGeometryBinding;
		descriptorSet mDescriptorSetPixel;
		descriptorSet mDescriptorSetMesh;

		bufferRegistry mVertexRegistry;
		bufferRegistry mIndexRegistry;
		bufferRegistry mPrimitiveRegistry;
		bufferRegistry mMeshletRegistry;

		textureRegistry mAlbedoRegistry;
		textureRegistry mRoughnessRegistry;
		textureRegistry mNormalRegistry;
		textureRegistry mMetalicRegistry;
		textureRegistry mAoRegistry;

		typedef vulkanShader pixelShader;
		std::map<pixelShader, pipelineData> mGeometryPipelines;
	};
}