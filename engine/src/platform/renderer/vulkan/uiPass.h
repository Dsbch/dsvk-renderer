#pragma once

#include <pch.h>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "resourceManager.h"
#include "base/include.h"

namespace engine
{
	struct uiPass
	{
		error init(std::shared_ptr<context> ctx, std::shared_ptr<vulkanContext> vulkanCtx, std::shared_ptr<resourceManager> resourceManager, GLFWwindow* wnd);
		error destroy();
		error drawUI(VkCommandBuffer cmd, const profilingInfo& profInfo);
		
		// Should be called when only viewport changed.
		void updateViewPortDependantDescriptors();
	private:
		std::shared_ptr<context> mCtx;
		std::shared_ptr<vulkanContext> mVulkanCtx;
		std::shared_ptr<resourceManager> mResourceManager;
		std::vector<VkDescriptorSet> mImGuiDescroptorSets;

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