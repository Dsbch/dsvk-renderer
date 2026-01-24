#include <pch.h>
#include "ui.h"
#include "helper.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace engine
{
	error vulkanUI::init(GLFWwindow* wnd, VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance, uint32_t queueFamily, VkQueue queue, VkFormat colorAttachmentFormat)
	{
		mColorAttachmentFormat = colorAttachmentFormat;

		auto logResult = [](VkResult err)
			{
				if (err != VK_SUCCESS)
					LOGERROR(vkResultToStr(err));
			};

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(wnd, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.

		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = instance;
		initInfo.PhysicalDevice = physicalDevice;
		initInfo.Device = device;
		initInfo.QueueFamily = queueFamily;
		initInfo.Queue = queue;
		initInfo.DescriptorPoolSize = 8;
		initInfo.MinImageCount = 2;
		initInfo.ImageCount = 3;
		initInfo.PipelineInfoMain.Subpass = 0;
		initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, .colorAttachmentCount = 1, .pColorAttachmentFormats = &mColorAttachmentFormat };;
		initInfo.CheckVkResultFn = logResult;
		initInfo.UseDynamicRendering = true;

		bool result = ImGui_ImplVulkan_Init(&initInfo);
		if (!result)
			return error{ "can't init UI" };

		return error();
	}

	error vulkanUI::destroy()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		return {};
	}

	error vulkanUI::onRender(VkCommandBuffer cmd, VkImageView drawImageView, VkExtent3D renderExtent)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Add calls to imgui here.

		ImGui::Render();

		//begin a render pass connected to our draw image and depth buffer.
		VkRenderingAttachmentInfo colorAttachment = attachmentInfo(drawImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = renderingInfo(renderExtent, &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		vkCmdEndRendering(cmd);

		return {};
	}
}