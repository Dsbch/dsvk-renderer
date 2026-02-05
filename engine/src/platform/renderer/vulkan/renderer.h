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
#include "deletionQueue.h"
#include "meshletRenderer.h"
#include "lineRenderer.h"

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
		void removeFromRender(const model& m);
		
		error render(renderer::renderCallIn in);

		withError<std::shared_ptr<shader>> makeShader(const std::vector<uint32_t>& src);
		withError<std::shared_ptr<texture>> makeTexture(uint8_t* data, int width, int heigth, imageChannel channel);
	private:
		std::shared_ptr<std::mutex> mRenderMutex;

		bool mWindowMinimized;
		VkDebugUtilsMessengerEXT mDebugMessenger;
		VkDevice mDevice;
		VmaAllocator mAllocator;
		VkInstance mInstance;
		VkPhysicalDevice mPhysicalDevice;
		deviceLimits mDeviceLimits;
		
		VkSurfaceKHR mSurface;
		swapChain mSwapChain;
		
		VkQueue mGraphicsQueue;
		uint32_t mGraphicsQueueFamily;
		submit mSubmit;

		deletionQueue mDeletionQueue;

		// UBO generic data per drawCall.
		vulkanBuffer mUboPerDrawBuffer;

		// Imgui wrapper.
		vulkanUI mUi;

		// Geometry pass.
		meshletRenderer mMeshletRenderer;
		// Line renderer.
		lineRenderer mLineRenderer;

		PFN_vkCmdDrawMeshTasksEXT mVkCmdDrawMeshTasksEXT;

		error initVulkan();
		error setLimits();
		error initImmediateSubmit();
		error initSwapchain(uint32_t width, uint32_t height);
		error loadExtensions();

		error initRenderers();

		error updatePerDrawBuffer(renderer::renderCallIn in);
		void chooseGraphicsPreset();
	};
}