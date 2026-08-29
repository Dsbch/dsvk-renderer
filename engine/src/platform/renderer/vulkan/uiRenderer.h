#pragma once
#include <pch.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "descriptorSet.h"
#include "resourceManager.h"
#include "platform/renderer/renderer.h"


namespace engine
{
	struct uiRenderer
	{
		error init(
			std::shared_ptr<context> ctx,
			GLFWwindow* wnd,	
			VkDevice device, 
			VkPhysicalDevice physicalDevice, 
			VkInstance instance, 
			uint32_t queueFamily, 
			VkQueue queue, 
			graphicsPreset preset,
			std::shared_ptr<resourceManager> manager
		);
		error destroy();
		error onRender(VkCommandBuffer cmd, const profilingInfo& profInfo);
		
		// Should be called when only viewport changed.
		void updateViewPortDependantDescriptors();
	private:
		VkDevice mDevice;
		std::vector<VkDescriptorSet> mImGuiDescroptorSets;
		VkSampler mSampler;
		graphicsPreset mPreset;
		std::shared_ptr<resourceManager> mResourceManager;

		struct passHistory {
			float history[128] = {};
			float lastValidMs = 0.0f;
			ImColor color;
		};
		std::map<std::string, passHistory> mPassHistories;
		int mGlobalOffset;
		
		void renderProfilingInfo(const profilingInfo& profInfo);
		void renderAccumAndRevealImages();
		void renderHzbImages();
	};
}