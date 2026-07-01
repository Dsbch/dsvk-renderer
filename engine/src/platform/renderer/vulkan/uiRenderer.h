#pragma once
#include <pch.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "descriptorSet.h"
#include "swapChain.h"
#include "platform/renderer/renderer.h"


namespace engine
{
	struct uiRenderer
	{
		error init(
			GLFWwindow* wnd,	
			VkDevice device, 
			VkPhysicalDevice physicalDevice, 
			VkInstance instance, 
			uint32_t queueFamily, 
			VkQueue queue, 
			swapChain sChain,
			graphicsPreset preset
		);
		error destroy();
		error onRender(VkCommandBuffer cmd, const swapChain& sChain, profilingInfo profInfo);
		void updateSwapchainDependentDescriptors(swapChain sChain);
	private:
		VkDevice mDevice;
		std::vector<VkDescriptorSet> mImGuiDescroptorSets;
		VkSampler mSampler;
		graphicsPreset mPreset;

		struct passHistory {
			float history[128] = {};
			int offset = 0;
			float lastValidMs = 0.0f;
			ImColor color;
		};
		std::map<std::string, passHistory> mPassHistories;
		
		void renderProfilingInfo(profilingInfo profInfo);
		void renderAccumAndRevealImages();
		void renderHzbImages();
	};
}