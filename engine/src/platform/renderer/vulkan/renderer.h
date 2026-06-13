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
#include "uiRenderer.h"
#include "deletionQueue.h"
#include "meshletRenderer.h"
#include "lineRenderer.h"
#include "gpuProfiler.h"
#include "computeRenderer.h"

namespace engine
{
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
		error updateInstance(const model& m);
		error updateAnimations(const model& m);
		void removeFromRender(const model& m);

		error render(renderer::renderCallIn in);

		profilingInfo getProfilingInfo();

		withError<std::shared_ptr<const texture>> makeTexture(const image& img);
		withError<std::shared_ptr<const shader>> makeShader(const std::vector<uint32_t>& src);
		withError<std::shared_ptr<const texture>> makeTextureWithMips(const imageWithMipLevels& img);
	private:
		bool mWindowMinimized;
		VkDebugUtilsMessengerEXT mDebugMessenger;
		VkDevice mDevice;
		VmaAllocator mAllocator;
		VkInstance mInstance;
		VkPhysicalDevice mPhysicalDevice;
		deviceLimits mDeviceLimits;
		VkPhysicalDeviceProperties mDeviceProps;

		profilingInfo mProfInfo;

		VkSurfaceKHR mSurface;
		swapChain mSwapChain;

		VkQueue mGraphicsQueue;
		uint32_t mGraphicsQueueFamily;
		submit mSubmit;

		deletionQueue mDeletionQueue;

		// UBO generic data per drawCall.
		vulkanBuffer mUboPerDrawBuffer;

		// Geometry pass.
		meshletRenderer mMeshletRenderer;
		// Line renderer.
		lineRenderer mLineRenderer;
		// UI renderer.
		uiRenderer mUiRenderer;
		// Compute renderer.
		computeRenderer mComputeRenderer;

		gpuProfiler mGpuProfiler;

		PFN_vkCmdDrawMeshTasksEXT mVkCmdDrawMeshTasksEXT;
		
		error initVulkan();
		error setLimits();
		error initImmediateSubmit();
		error initSwapchain(uint32_t width, uint32_t height);
		error loadExtensions();

		error initRenderers(std::shared_ptr<window> window);

		error updatePerDrawBuffer(renderer::renderCallIn in);
		void chooseGraphicsPreset();
		void setViewportAndSciccors(VkCommandBuffer cmd) const;
		error drawOpaque(VkCommandBuffer cmd, renderer::renderCallIn in);
		error buildHZB(VkCommandBuffer cmd, renderer::renderCallIn in);
		error drawTransperent(VkCommandBuffer cmd, renderer::renderCallIn in);
		error compositeOpaqueAndTransperent(VkCommandBuffer cmd, renderer::renderCallIn in);
		error drawUI(VkCommandBuffer cmd);

		void updateProfInfo(float deltaTime);
		void registerSceneMetrics(const model& m, bool isDeleted = false);
		void visualizeNormals(const model& m);
	};
}