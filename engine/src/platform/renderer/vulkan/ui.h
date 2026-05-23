#pragma once
#include <pch.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "descriptorSet.h"
#include "platform/renderer/renderer.h"

namespace engine
{
	struct vulkanUI
	{
		error init(GLFWwindow* wnd, VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance, uint32_t queueFamily, VkQueue queue, VkFormat colorAttachmentFormat);
		error destroy();
		error onRender(VkCommandBuffer cmd, profilingInfo profInfo);
	private:
		void renderProfilingInfo(profilingInfo profInfo);

		VkFormat mColorAttachmentFormat;
	};
}