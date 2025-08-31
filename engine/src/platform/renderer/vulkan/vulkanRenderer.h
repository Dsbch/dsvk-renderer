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
		};
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
		void loadExtensions();
		void initImmediateSubmit();
		void initSwapchain(uint32_t width, uint32_t height);
		void initDescriptors();
		void setGraphicsDescriptorBindings();
		void setBackgroundDescriptors();
		void initPipelines();
		void initBackgroundPipeline();
		void initGraphicsPipeline();

		VkDevice mDevice;
		VmaAllocator mAllocator;

		VkInstance mInstance;
		VkDebugUtilsMessengerEXT mDebugMessenger;
		VkPhysicalDevice mPhysicalDevice;
		VkSurfaceKHR mSurface;
		swapChain mSwapChain;
		VkQueue mGraphicsQueue;
		uint32_t mGraphicsQueueFamily;
		immediateSubmit mImmediateSubmit;
		computePipeline mComputePipeline;
		classicGraphicPipeline mGraphicsPipeline;
		descriptorSet mDescriptorSetCompute;
		descriptorSet mDescriptorSetPixel;
		descriptorSet mDescriptorSetMesh;

		void flushDeletonQueue();
		std::deque<destroyTask> mDeletionQueue;
	};
}