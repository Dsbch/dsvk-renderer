#pragma once
#include <pch.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "descriptorSet.h"
#include "swapChain.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	struct vulkanUI
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
		error onRender(VkCommandBuffer cmd, profilingInfo profInfo);
		void updateDescriptorSets(swapChain sChain);
	private:
		VkDevice mDevice;
		std::vector<VkDescriptorSet> mImGuiDescroptorSets;
		VkSampler mSampler;
		graphicsPreset mPreset;
		
		void renderProfilingInfo(profilingInfo profInfo);
		void renderAccumAndRevealImages();
	};
}